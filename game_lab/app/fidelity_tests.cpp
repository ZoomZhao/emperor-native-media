// SPDX-License-Identifier: MIT
// Fidelity check for the normal App Play path. Finds two original saves of the
// same city where the later one can only be the earlier one after a forward run,
// derives the update distance from the saved scheduler, runs AppCityPlayback
// (experiment off, the same load hypotheses as play) for that many updates, and
// compares the aggregates the public playback view exposes.
//
// Automatic discovery scores only unchanged terrain/building pairs. Explicit
// pairs are compared even when edits or recorded events make attribution
// uncertain; only aggregate counts are printed. A pair blocked before its
// target calendar is never scored. Exit 0 with SKIP when the data root is unset.
#include "city_playback.hpp"
#include "../original_building_update.hpp"
#include "../original_evidence_fields.hpp"
#include "../asset_bytes.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>

namespace {
struct SaveView {
    std::string name;
    game_lab::OriginalSaveResult save;
    std::vector<uint8_t> payload;
    game_lab::EvidenceClock clock;
    bool clockOk = false;
    int32_t population = 0, treasury = 0, year = 0;
    bool cityOk = false,buildingInputsOk = false;
    std::optional<game_lab::OriginalBuildingUpdateContext::Accounting> accounting;
    std::optional<game_lab::OriginalBuildingUpdateContext::Labor> labor;
    std::optional<game_lab::OriginalBuildingCityEmployment> employment;
    std::optional<int32_t> scenarioMonths;
    std::map<size_t, uint16_t> liveSlots; // slot -> type, alive 1 or 3
    size_t terrainDiffersFrom(const SaveView& later) const {
        size_t n = 0;
        for (size_t i = 0; i < save.terrain.flags.size(); ++i)
            if (save.terrain.flags[i] != later.save.terrain.flags[i]) ++n;
        return n;
    }
    bool sameMap(const SaveView& other) const {
        const auto& a = save.layers; const auto& b = other.save.layers;
        return save.terrain.activeSide == other.save.terrain.activeSide &&
            a.mapperRandom == b.mapperRandom && a.mapperFertile == b.mapperFertile && a.mapperStone == b.mapperStone;
    }
};

// Updates separating two clocks, or empty when the clocks are not exactly one
// forward run. The ported scheduler executes one phase per update and wraps at
// 50; the phase-50 tail is the only cycle and month-part increment. The
// documented valid month part is 0..15, so a month is 16 parts. That wrap is
// the service the port refuses, so a distance that crosses it is derived, not
// observed.
std::optional<unsigned> forwardDistance(const game_lab::EvidenceClock& early, const game_lab::EvidenceClock& late) {
    if (!early.rangesValid() || !late.rangesValid()) return std::nullopt;
    const unsigned tails = late.cycle - early.cycle;
    if(tails>20000)return std::nullopt;
    const unsigned wrapped = early.monthPart + tails;
    if ((early.month + wrapped / 16) % 12 != late.month || wrapped % 16 != late.monthPart) return std::nullopt;
    if (!tails) return late.phase >= early.phase ? std::optional<unsigned>(late.phase - early.phase) : std::nullopt;
    // First tail executes phase 50: (51 - phase) updates from here. Each later
    // tail is another 51, and the tail leaves phase 0, so the target phase
    // remains.
    const uint64_t updates = uint64_t(51 - early.phase) + uint64_t(tails - 1) * 51 + late.phase;
    if (updates > std::numeric_limits<unsigned>::max()) return std::nullopt;
    return unsigned(updates);
}

std::vector<SaveView> inventory(const game_lab::ResourceRoot& root) {
    using namespace game_lab;
    std::vector<SaveView> out;
    const auto catalog = enumerateOriginalMapSources(root);
    if (!catalog.complete) throw std::runtime_error("source catalog incomplete");
    for (const auto& source : catalog.sources) {
        if (source.kind != OriginalMapSourceKind::SaveFile) continue;
        SaveView view; view.name = source.relativePath;
        view.save = loadOriginalSave(root.resolve(source.relativePath));
        if (view.save.status != OriginalSaveStatus::LoadedSnapshot) { out.push_back(std::move(view)); continue; }
        std::vector<uint8_t> raw;
        if (!detail::readFile(root.resolve(source.relativePath), 8u * 1024u * 1024u, raw)) continue;
        view.payload = decodeOriginalMapContainer(raw).payload;
        if (auto clock = evidenceSavedClock(view.payload, view.save.terrainOffset, view.save.sizeOffset)) {
            view.clock = *clock; view.clockOk = true;
        }
        OriginalBuildingUpdateContext city(OriginalBuildingUpdateScope::AdmittedTicks, true);
        if (originalBuildingSavedCityContext(view.payload, view.save.sizeOffset, city) && city.populationTotal && city.treasury && city.elapsedYears) {
            view.cityOk = true; view.population = *city.populationTotal; view.treasury = *city.treasury; view.year = *city.elapsedYears;
            view.accounting=city.accounting;view.labor=city.labor;view.employment=city.cityEmployment;view.scenarioMonths=city.scenarioMonths;
        }
        if (auto inputs = originalBuildingUpdateInputs(view.payload, view.save.buildings)){
            view.buildingInputsOk=true;
            for (const auto& [slot, input] : *inputs)
                if (input.alive == 1 || input.alive == 3) view.liveSlots.emplace(slot, view.save.buildings.slotTypes[slot]);}
        out.push_back(std::move(view));
    }
    return out;
}

// A slot that is live earlier and gone later was removed. A type change of a
// slot that survives can be the simulation evolving a house, so it is not
// treated as the player's action.
size_t removedBuildings(const SaveView& early, const SaveView& late) {
    size_t removed = 0;
    for (const auto& [slot, type] : early.liveSlots) if (!late.liveSlots.count(slot)) ++removed;
    return removed;
}

SaveView explicitSave(const game_lab::ResourceRoot& root,const std::string& name){
    using namespace game_lab;
    SaveView view;view.name=name;const auto path=root.resolve(name);
    if(path.empty())throw std::runtime_error("pair source is not under the data root");
    view.save=loadOriginalSave(path);
    if(view.save.status!=OriginalSaveStatus::LoadedSnapshot)throw std::runtime_error("pair save did not load");
    std::vector<uint8_t> raw;if(!detail::readFile(path,8u*1024u*1024u,raw))throw std::runtime_error("pair save unreadable");
    view.payload=decodeOriginalMapContainer(raw).payload;
    if(auto clock=evidenceSavedClock(view.payload,view.save.terrainOffset,view.save.sizeOffset)){view.clock=*clock;view.clockOk=true;}
    OriginalBuildingUpdateContext c(OriginalBuildingUpdateScope::AdmittedTicks,true);
    if(originalBuildingSavedCityContext(view.payload,view.save.sizeOffset,c)&&c.populationTotal&&c.treasury&&c.elapsedYears){
        view.cityOk=true;view.population=*c.populationTotal;view.treasury=*c.treasury;view.year=*c.elapsedYears;
        view.accounting=c.accounting;view.labor=c.labor;view.employment=c.cityEmployment;view.scenarioMonths=c.scenarioMonths;}
    if(auto inputs=originalBuildingUpdateInputs(view.payload,view.save.buildings)){view.buildingInputsOk=true;
        for(const auto& [slot,input]:*inputs)if(input.alive==1||input.alive==3)view.liveSlots.emplace(slot,view.save.buildings.slotTypes[slot]);}
    return view;
}

struct Agreement {size_t equal=0,total=0;};
std::optional<int> randomAdvanceDifference(uint32_t first,uint32_t second,uint32_t targetFirst,uint32_t targetSecond){
    const auto next=[](uint32_t value){for(unsigned n=0;n<31;++n)value=(value>>1)|((((value>>4)^value)&1u)<<30);return value;};
    uint32_t a=first,b=second,c=targetFirst,d=targetSecond;
    for(int n=0;n<=100000;++n){if(a==targetFirst&&b==targetSecond)return n;if(c==first&&d==second)return -n;
        a=next(a);b=next(b);c=next(c);d=next(d);}return std::nullopt;
}
template<class A,class B> void compare(std::map<std::string,Agreement>& groups,const char* group,const A& a,const B& b){
    auto& row=groups[group];++row.total;if(a==b)++row.equal;
}
template<class T> void compareOptional(std::map<std::string,Agreement>& groups,const char* group,
    const std::optional<T>& a,const std::optional<T>& b){
    if(a&&b)compare(groups,group,*a,*b);else if(bool(a)!=bool(b))compare(groups,group,bool(a),bool(b));
}
void compareCommitted(const game_lab::OriginalCombinedState& city,const SaveView& late,std::map<std::string,Agreement>& groups){
    using namespace game_lab;
    const auto& c=*city.buildingContext;
    compare(groups,"calendar",city.phase,late.clock.phase);
    if(c.cycle)compare(groups,"calendar",*c.cycle,late.clock.cycle);
    if(c.monthPart)compare(groups,"calendar",*c.monthPart,late.clock.monthPart);
    if(c.month)compare(groups,"calendar",*c.month,late.clock.month);
    if(late.cityOk){if(c.treasury)compare(groups,"treasury",*c.treasury,late.treasury);
        if(c.elapsedYears)compare(groups,"year",*c.elapsedYears,late.year);
        if(city.figures.services&&city.figures.services->population)compare(groups,"population",city.figures.services->population->total,late.population);}
    if(c.scenarioMonths&&late.scenarioMonths)compare(groups,"calendar",*c.scenarioMonths,*late.scenarioMonths);
    if(city.figures.terrain.size()==late.save.terrain.flags.size())
        for(size_t n=0;n<city.figures.terrain.size();++n)compare(groups,"terrain",city.figures.terrain[n],late.save.terrain.flags[n]);
    if(city.figures.services&&late.save.terrainOffset<=late.payload.size()&&
       original_save19::randomSecond+4<=late.payload.size()-late.save.terrainOffset){const auto* p=late.payload.data()+late.save.terrainOffset;
        compare(groups,"random",city.figures.services->randomFirst,detail::u32(p+original_save19::randomFirst));
        compare(groups,"random",city.figures.services->randomSecond,detail::u32(p+original_save19::randomSecond));}
    if(c.accounting&&late.accounting){const auto& x=*c.accounting;const auto& y=*late.accounting;
#define ACCOUNT_FIELD(field) compare(groups,"accounting",x.field,y.field)
        ACCOUNT_FIELD(taxRate);ACCOUNT_FIELD(taxReceived);ACCOUNT_FIELD(taxOther);ACCOUNT_FIELD(wages);ACCOUNT_FIELD(wagesPaid);
        ACCOUNT_FIELD(incomeOther);ACCOUNT_FIELD(expensesOther);ACCOUNT_FIELD(treasuryStart);ACCOUNT_FIELD(incomeReceived);
        ACCOUNT_FIELD(expensesPaid);ACCOUNT_FIELD(festivalCount);
#undef ACCOUNT_FIELD
        compareOptional(groups,"accounting",x.tributeIncomeComponent,y.tributeIncomeComponent);
        if(x.buckets&&y.buckets){const auto& a=*x.buckets;const auto& b=*y.buckets;
#define ANNUAL_FIELD(field) compare(groups,"annual_finance",a.field,b.field)
            ANNUAL_FIELD(priorTax);ANNUAL_FIELD(priorWages);ANNUAL_FIELD(priorExports);ANNUAL_FIELD(priorImports);
            ANNUAL_FIELD(priorExpense10514);ANNUAL_FIELD(priorExpense11400);ANNUAL_FIELD(priorExpense11161);ANNUAL_FIELD(priorExpense10534);
            ANNUAL_FIELD(priorIncome11440);ANNUAL_FIELD(priorIncome11061);ANNUAL_FIELD(priorTribute);ANNUAL_FIELD(priorMisc11121);
#undef ANNUAL_FIELD
        }}
    {OriginalBuildingUpdateContext saved(OriginalBuildingUpdateScope::AdmittedTicks,true);
        if(originalBuildingSavedCityContext(late.payload,late.save.sizeOffset,saved)){
            if(c.cycleTail&&saved.cycleTail)compare(groups,"monthly_arrivals",c.cycleTail->previousMonthArrivals,saved.cycleTail->previousMonthArrivals);
            for(size_t n=0;n<c.previousYearProduction.size();++n)compare(groups,"annual_production",c.previousYearProduction[n],saved.previousYearProduction[n]);
            if(late.save.layers.figures&&originalBuildingSavedEmpireStates(late.payload,late.save.layers.figures->end,&saved)&&c.trade&&saved.trade)
                for(size_t p=0;p<c.trade->partners.size();++p)for(size_t n=1;n<=28;++n)
                    compare(groups,"annual_quotas",c.trade->partners[p].quotaUsed[n],saved.trade->partners[p].quotaUsed[n]);}}
    if(c.labor&&late.labor){const auto& x=*c.labor;const auto& y=*late.labor;
#define LABOR_FIELD(field) do {compare(groups,"labor",x.field,y.field);compare(groups,"labor_detail." #field,x.field,y.field);} while(false)
        compare(groups,"sentiment",x.sentiment,y.sentiment);
        compareOptional(groups,"sentiment",x.previousSentiment,y.previousSentiment);
        compareOptional(groups,"sentiment",x.sentimentReached350,y.sentimentReached350);
        LABOR_FIELD(health);LABOR_FIELD(migration);LABOR_FIELD(incoming);LABOR_FIELD(outgoing);
        LABOR_FIELD(inRemainder);LABOR_FIELD(outRemainder);LABOR_FIELD(inDelay);LABOR_FIELD(outDelay);LABOR_FIELD(arrivals);
        LABOR_FIELD(departures);LABOR_FIELD(unplaced);LABOR_FIELD(yearArrivals);LABOR_FIELD(statisticsPending);LABOR_FIELD(populationSum);
        LABOR_FIELD(sampleCount);LABOR_FIELD(averagePopulation);LABOR_FIELD(growthCursor);LABOR_FIELD(deathCursor);
        LABOR_FIELD(lastPopulationDelta);LABOR_FIELD(excessDepartureTotal);
#undef LABOR_FIELD
    }
    if(c.cityEmployment&&late.employment){const auto& x=*c.cityEmployment;const auto& y=*late.employment;
        compare(groups,"employment",x.available,y.available);compare(groups,"employment",x.employed,y.employed);
        compare(groups,"employment",x.unemployed,y.unemployed);compare(groups,"employment",x.unemploymentPercent,y.unemploymentPercent);
        compare(groups,"employment",x.shortage,y.shortage);
        for(size_t n=0;n<x.rows.size();++n){const auto& a=x.rows[n];const auto& b=y.rows[n];
            compare(groups,"employment",a.demand,b.demand);compare(groups,"employment",a.allocated,b.allocated);
            compare(groups,"employment",a.preferred,b.preferred);compare(groups,"employment",a.preferredAllocated,b.preferredAllocated);
            compare(groups,"employment",a.priority,b.priority);}}
    if(c.formations){const auto saved=originalBuildingSavedFormations(late.payload,late.save.terrainOffset);
        if(saved)for(size_t n=0;n<c.formations->size();++n){const auto& x=c.formations->at(n);const auto& y=saved->at(n);
            compare(groups,"formation_identity",bool(x.alive),bool(y.alive));if(!x.alive||!y.alive)continue;
            compare(groups,"formation_state",x.count,y.count);compare(groups,"formation_detail.count",x.count,y.count);
            compare(groups,"formation_state",x.limit,y.limit);compare(groups,"formation_detail.limit",x.limit,y.limit);
            compare(groups,"formation_state",x.kind,y.kind);compare(groups,"formation_detail.kind",x.kind,y.kind);
            compare(groups,"formation_state",x.spawnTimer,y.spawnTimer);compare(groups,"formation_detail.spawnTimer",x.spawnTimer,y.spawnTimer);
            for(size_t k=0;k<x.members.size();++k){compare(groups,"formation_state",x.members[k],y.members[k]);compare(groups,"formation_detail.members",x.members[k],y.members[k]);}}}
    const auto inputs=originalBuildingUpdateInputs(late.payload,late.save.buildings);
    if(inputs){std::set<size_t> slots;for(const auto& e:*inputs)if(e.second.alive==1||e.second.alive==3)slots.insert(e.first);
        for(const auto& e:city.buildings)if(e.second.state.inputs.alive==1||e.second.state.inputs.alive==3)slots.insert(e.first);
        for(size_t slot:slots){const auto a=city.buildings.find(slot);const auto b=inputs->find(slot);
            const bool presentA=a!=city.buildings.end()&&(a->second.state.inputs.alive==1||a->second.state.inputs.alive==3);
            const bool presentB=b!=inputs->end()&&(b->second.alive==1||b->second.alive==3);
            compare(groups,"building_identity",presentA,presentB);if(!presentA||!presentB)continue;
            const auto& x=a->second.state.inputs;const auto& y=b->second;
            compare(groups,"building_identity",a->second.state.building.type,late.save.buildings.slotTypes[slot]);
            if(a->second.state.building.type!=late.save.buildings.slotTypes[slot])continue;
#define BUILDING_FIELD(field) compare(groups,"building_common",x.field,y.field)
            BUILDING_FIELD(alive);BUILDING_FIELD(output);BUILDING_FIELD(byte52);BUILDING_FIELD(upgrade);BUILDING_FIELD(identity);
            BUILDING_FIELD(model);BUILDING_FIELD(residents);BUILDING_FIELD(free);BUILDING_FIELD(distance);BUILDING_FIELD(roadX);BUILDING_FIELD(roadY);
            BUILDING_FIELD(migrant);BUILDING_FIELD(workers);BUILDING_FIELD(available);BUILDING_FIELD(random);BUILDING_FIELD(network);BUILDING_FIELD(laborCountdown);
            BUILDING_FIELD(operational);BUILDING_FIELD(parent);BUILDING_FIELD(suppression);BUILDING_FIELD(laborDisabled);BUILDING_FIELD(enabled);
            BUILDING_FIELD(employmentCategory);BUILDING_FIELD(houseWater);BUILDING_FIELD(houseService);BUILDING_FIELD(spawnCounter);BUILDING_FIELD(spawnAux);
            BUILDING_FIELD(roadFailure);BUILDING_FIELD(primaryFigure);BUILDING_FIELD(word30);BUILDING_FIELD(word34);BUILDING_FIELD(stock);BUILDING_FIELD(faction);
            BUILDING_FIELD(direction);BUILDING_FIELD(houseInUse);BUILDING_FIELD(desirability);BUILDING_FIELD(desirabilityBonus);BUILDING_FIELD(fireRisk);
            BUILDING_FIELD(damageRisk);BUILDING_FIELD(burning);BUILDING_FIELD(word92);BUILDING_FIELD(fireproof);BUILDING_FIELD(fireDirection);
            BUILDING_FIELD(fireSuppressed);BUILDING_FIELD(watchRisk);
#undef BUILDING_FIELD
            compareOptional(groups,"building_common",x.modelDesirability,y.modelDesirability);
            compareOptional(groups,"building_common",x.modelUpgrade,y.modelUpgrade);
            compareOptional(groups,"building_common",x.modelDesirabilityBonus,y.modelDesirabilityBonus);
            compareOptional(groups,"building_common",x.parentSlot,y.parentSlot);
            compareOptional(groups,"building_common",x.stallResourceIndex,y.stallResourceIndex);
            compareOptional(groups,"building_common",x.supplierStatus,y.supplierStatus);
            compareOptional(groups,"building_common",x.nextPart,y.nextPart);
            if(x.cropWorkers&&y.cropWorkers)for(size_t n=0;n<2;++n)compare(groups,"crop_industry",x.cropWorkers->at(n),y.cropWorkers->at(n));
            else if(bool(x.cropWorkers)!=bool(y.cropWorkers))compare(groups,"crop_industry",bool(x.cropWorkers),bool(y.cropWorkers));
            if(x.cropBays&&y.cropBays)for(size_t n=0;n<x.cropBays->size();++n){const auto& p=x.cropBays->at(n);const auto& q=y.cropBays->at(n);
                compare(groups,"crop_storage",p.resource,q.resource);compare(groups,"crop_storage",p.amount,q.amount);compare(groups,"crop_storage",p.capacity,q.capacity);}
            else if(bool(x.cropBays)!=bool(y.cropBays))compare(groups,"crop_storage",bool(x.cropBays),bool(y.cropBays));
            if(x.house&&y.house){const auto goodsA=originalHouseGoodsFromPayload(*x.house),goodsB=originalHouseGoodsFromPayload(*y.house);
                if(goodsA&&goodsB)for(size_t n=0;n<goodsA->size();++n)compare(groups,"house_goods",goodsA->at(n),goodsB->at(n));
                compare(groups,"house_goods",x.house->at(53-4),y.house->at(53-4));
                for(unsigned wire:{13u,14u,15u,16u,41u,42u,43u,44u,45u,49u,50u,51u,54u,55u,56u})
                    compare(groups,"house_services",x.house->at(wire-4),y.house->at(wire-4));
                compare(groups,"house_services",x.house->at(55),y.house->at(55));}
            if(a->second.state.building.type>=194&&a->second.state.building.type<=199&&x.industry&&y.industry){compare(groups,"crop_industry",originalBuildingCropFlags(x),originalBuildingCropFlags(y));
                compare(groups,"crop_industry",originalBuildingCropProgress(x),originalBuildingCropProgress(y));}
        }}
    if(late.save.layers.figures&&late.save.layers.figures->status==OriginalFigureStatus::Complete&&late.save.layers.figures->slots.size()==2000){
        for(size_t slot=1;slot<2000;++slot){const auto found=city.figures.figures.find(slot);if(found==city.figures.figures.end())continue;
            const auto& x=found->second;const auto& y=late.save.layers.figures->slots[slot];
            compare(groups,"figure_identity",bool(x.alive),bool(y.alive));if(!x.alive||!y.alive)continue;
            compare(groups,"figure_identity",x.type,y.type);compare(groups,"figure_identity",x.runtimeClass,y.runtimeClass);
            if(x.type!=y.type||x.runtimeClass!=y.runtimeClass)continue;
            compare(groups,"figure_images",x.image,y.image);compare(groups,"figure_images",x.secondaryImage,y.secondaryImage);
            const auto figureType="figure_type."+std::to_string(x.type);
            for(const auto& pair:std::array<std::pair<int32_t,int32_t>,5>{{{x.x,y.x},{x.y,y.y},{x.action,y.action},{x.direction,y.direction},{x.progress,y.progress}}})
                compare(groups,figureType.c_str(),pair.first,pair.second);
#define FIGURE_FIELD(field) do {compare(groups,"figure_state",x.field,y.field);compare(groups,"figure_detail." #field,x.field,y.field);} while(false)
            FIGURE_FIELD(alive);FIGURE_FIELD(cell);FIGURE_FIELD(x);FIGURE_FIELD(y);FIGURE_FIELD(subX);FIGURE_FIELD(subY);FIGURE_FIELD(next);
            FIGURE_FIELD(action);FIGURE_FIELD(progress);FIGURE_FIELD(direction);FIGURE_FIELD(previousDirection);FIGURE_FIELD(frame);
            FIGURE_FIELD(subtype);FIGURE_FIELD(resource);FIGURE_FIELD(counter);FIGURE_FIELD(owner);FIGURE_FIELD(amount);FIGURE_FIELD(target);
            FIGURE_FIELD(leaderLink);FIGURE_FIELD(follower);FIGURE_FIELD(formation);FIGURE_FIELD(damage);FIGURE_FIELD(route);
            FIGURE_FIELD(routeCursor);FIGURE_FIELD(routeLength);FIGURE_FIELD(destinationX);FIGURE_FIELD(destinationY);FIGURE_FIELD(destinationCell);
            FIGURE_FIELD(age);FIGURE_FIELD(home);FIGURE_FIELD(delivery);FIGURE_FIELD(linkedBuilding);
            FIGURE_FIELD(extraX);FIGURE_FIELD(extraY);FIGURE_FIELD(freeMovement);FIGURE_FIELD(height);FIGURE_FIELD(hidden);FIGURE_FIELD(drawClass);
            FIGURE_FIELD(state85);FIGURE_FIELD(raise);FIGURE_FIELD(attackDirection);FIGURE_FIELD(attackFrame);FIGURE_FIELD(cartStyle);FIGURE_FIELD(stationary);
            FIGURE_FIELD(variant);FIGURE_FIELD(faction);FIGURE_FIELD(employmentAmount);FIGURE_FIELD(watchCoverage);FIGURE_FIELD(effectKind);
            FIGURE_FIELD(effectVariant);FIGURE_FIELD(effectFlags);FIGURE_FIELD(effectX);FIGURE_FIELD(effectY);FIGURE_FIELD(effectVelocityX);
            FIGURE_FIELD(effectVelocityY);FIGURE_FIELD(effectHeight);FIGURE_FIELD(effectVelocityZ);FIGURE_FIELD(effectDraw);
            // effectFrame and progress both decode Figure7+41. Only flame
            // animation writes effectFrame; movement writes progress instead.
            if(x.type==2)FIGURE_FIELD(effectFrame);
            FIGURE_FIELD(updateOwner);FIGURE_FIELD(threat);FIGURE_FIELD(threatDistance);FIGURE_FIELD(herdTarget);FIGURE_FIELD(herdDistance);
            FIGURE_FIELD(idleCounter);FIGURE_FIELD(movementMode);FIGURE_FIELD(attackerCount);FIGURE_FIELD(routePhase);FIGURE_FIELD(speed);
            FIGURE_FIELD(priorProgress);FIGURE_FIELD(reciprocalTarget);FIGURE_FIELD(priorX);FIGURE_FIELD(priorY);FIGURE_FIELD(priorCell);
            FIGURE_FIELD(homeX);FIGURE_FIELD(homeY);FIGURE_FIELD(flags120);FIGURE_FIELD(water);FIGURE_FIELD(roamLimit);FIGURE_FIELD(roamSteps);
            FIGURE_FIELD(roamState);FIGURE_FIELD(roamTurns);FIGURE_FIELD(roamTurn);FIGURE_FIELD(roamCountdown);FIGURE_FIELD(searchCounter);
            FIGURE_FIELD(roamOriginX);FIGURE_FIELD(roamOriginY);FIGURE_FIELD(freeTargetX);FIGURE_FIELD(freeTargetY);FIGURE_FIELD(freeRemainingX);
            FIGURE_FIELD(freeRemainingY);FIGURE_FIELD(freeError);FIGURE_FIELD(freeAxis);FIGURE_FIELD(cellMovement);FIGURE_FIELD(previousAction);
            FIGURE_FIELD(allocationSerial);FIGURE_FIELD(allocationPhase);FIGURE_FIELD(allocationPhaseCopy);FIGURE_FIELD(projectile);
            FIGURE_FIELD(projectileDistance);FIGURE_FIELD(projectileStartHeight);FIGURE_FIELD(projectileTargetHeight);FIGURE_FIELD(projectileEndHeight);
            FIGURE_FIELD(projectileHits);FIGURE_FIELD(formationOrdinal);FIGURE_FIELD(formationStationary);
#undef FIGURE_FIELD
            for(size_t n=0;n<x.cooldowns.size();++n)compare(groups,"figure_state",x.cooldowns[n],y.cooldowns[n]);
            for(size_t n=0;n<x.attackers.size();++n)compare(groups,"figure_state",x.attackers[n],y.attackers[n]);
        }}
}

int selfTest(){
    using namespace game_lab;
    const EvidenceClock before{50,3,15,11},after{0,4,0,0};
    if(forwardDistance(before,after)!=1||forwardDistance(before,EvidenceClock{0,4,1,0}))
        throw std::runtime_error("calendar alignment across year");
    OriginalCombinedState city;city.phase=0;city.buildingContext.emplace(OriginalBuildingUpdateScope::AdmittedTicks,true);
    auto& c=*city.buildingContext;c.cycle=4;c.monthPart=0;c.month=0;c.treasury=99;c.elapsedYears=7;
    city.figures.services.emplace();city.figures.services->population.emplace();city.figures.services->population->total=12;
    SaveView late;late.clock=after;late.cityOk=true;late.treasury=98;late.year=7;late.population=12;
    std::map<std::string,Agreement> groups;compareCommitted(city,late,groups);
    if(groups["calendar"].equal!=4||groups["calendar"].total!=4||groups["treasury"].equal!=0||groups["treasury"].total!=1||
       groups["year"].equal!=1||groups["population"].equal!=1)
        throw std::runtime_error("field group agreement counts");
    std::cout<<"fidelity self-test passed\n";return 0;
}

int explicitPairs(const game_lab::ResourceRoot& root,const game_lab::AppCityPlaybackPreferences& preferences,int argc,char** argv){
    using namespace game_lab;
    if((argc-1)%3)throw std::runtime_error("pairs require --pair EARLY LATE or --event-pair EARLY LATE");
    unsigned reachedPairs=0,blockedPairs=0;
    for(int arg=1;arg<argc;arg+=3){const std::string kind=argv[arg];
        if(kind!="--pair"&&kind!="--event-pair")throw std::runtime_error("unknown pair option");
        const auto early=explicitSave(root,argv[arg+1]),late=explicitSave(root,argv[arg+2]);
        const auto distance=early.clockOk&&late.clockOk?forwardDistance(early.clock,late.clock):std::nullopt;
        std::cout<<"pair "<<early.name<<" -> "<<late.name<<" event_class="<<(kind=="--event-pair"?"unmet_request_and_rating_notice":"unclassified")
                 <<" same_map="<<early.sameMap(late)<<" terrain_differences="<<early.terrainDiffersFrom(late)<<" removed_buildings=";
        if(early.buildingInputsOk&&late.buildingInputsOk)std::cout<<removedBuildings(early,late);else std::cout<<"unavailable";
        if(!distance){++blockedPairs;std::cout<<" blocked=calendar_alignment\n";continue;}
        AppMapSurface surface(root);AppCitySessionState session;session.crtRandom=1;
        AppCityPlayback playback(surface,early.save,early.payload,preferences,1,0,false,&session);
        if(!playback.prepared()){++blockedPairs;std::cout<<" blocked=not_prepared detail="<<playback.status().error<<'\n';continue;}
        const unsigned interval=std::max(1u,playback.status().intervalMs);
        for(unsigned n=1;n<=*distance;++n)if(!playback.step(uint64_t(n)*interval))break;
        std::cout<<" updates="<<playback.status().updates<<'/'<<*distance;
        if(playback.status().updates!=*distance||!playback.status().refusal.empty()){
            ++blockedPairs;std::cout<<" blocked="<<(playback.status().refusal.empty()?"not_admitted":playback.status().refusal);
            if(const auto* stopped=surface.cityPlaybackState();stopped&&stopped->buildingContext){const auto& c=*stopped->buildingContext;
                std::cout<<" phase="<<stopped->phase;if(c.monthPart)std::cout<<" part="<<*c.monthPart;}
            std::cout<<'\n';continue;}
        const auto* city=surface.cityPlaybackState();if(!city||!city->buildingContext){++blockedPairs;std::cout<<" blocked=city_state_unavailable\n";continue;}
        const auto& calendar=*city->buildingContext;
        if(city->phase!=late.clock.phase||!calendar.cycle||*calendar.cycle!=late.clock.cycle||
           !calendar.monthPart||*calendar.monthPart!=late.clock.monthPart||!calendar.month||*calendar.month!=late.clock.month){
            ++blockedPairs;std::cout<<" blocked=calendar_diverged\n";continue;}
        ++reachedPairs;std::map<std::string,Agreement> groups;
        for(const char* name:{"calendar","treasury","year","population","sentiment","random","accounting","labor","employment","formation_identity","formation_state","building_identity","building_common","house_goods","house_services","crop_industry","crop_storage","figure_identity","figure_images","figure_state"})groups[name]={};
        compareCommitted(*city,late,groups);
        const auto liveSimulated=std::count_if(city->buildings.begin(),city->buildings.end(),[](const auto& e){return e.second.state.inputs.alive==1||e.second.state.inputs.alive==3;});
        std::cout<<" attribution=unresolved live_buildings="<<early.liveSlots.size()<<'/'<<late.liveSlots.size()
                 <<" simulated_live_buildings="<<liveSimulated<<" simulated_records="<<city->buildings.size()<<"\n";
        {size_t simulatedRuins=0,savedRuins=0;
            for(const auto& e:city->buildings)if(e.second.state.building.type==161&&(e.second.state.inputs.alive==1||e.second.state.inputs.alive==3))++simulatedRuins;
            for(const auto& e:late.liveSlots)if(e.second==161)++savedRuins;
            std::cout<<"  live_ruins simulated="<<simulatedRuins<<" original="<<savedRuins;
            if(calendar.disasters)std::cout<<" simulated_destructions="<<calendar.disasters->destructions;
            std::cout<<'\n';}
        if(city->figures.services&&late.save.terrainOffset<=late.payload.size()&&
           original_save19::randomSecond+4<=late.payload.size()-late.save.terrainOffset){const auto* p=late.payload.data()+late.save.terrainOffset;
            const auto delta=randomAdvanceDifference(city->figures.services->randomFirst,city->figures.services->randomSecond,
                detail::u32(p+original_save19::randomFirst),detail::u32(p+original_save19::randomSecond));
            std::cout<<"  random_draw_delta=";if(delta)std::cout<<*delta;else std::cout<<"outside_100000_or_different_state";std::cout<<'\n';}
        for(const auto& [name,row]:groups){if((name.rfind("figure_detail.",0)==0||name.rfind("labor_detail.",0)==0||name.rfind("formation_detail.",0)==0)&&row.equal==row.total)continue;
            std::cout<<"  "<<name<<' '<<row.equal<<'/'<<row.total<<" agree\n";}
    }
    std::cout<<"FIDELITY pairs_reached="<<reachedPairs<<" blocked="<<blockedPairs<<'\n';return 0;
}
}

int main(int argc,char** argv) {
    using namespace game_lab;
    if(argc==2&&std::string(argv[1])=="--self-test"){try{return selfTest();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
    const char* rootPath = std::getenv("EMPEROR_GAMEDATA_ROOT");
    if (!rootPath || !*rootPath) { std::cout << "SKIP EMPEROR_GAMEDATA_ROOT unset\n"; return 0; }
    const auto root = ResourceRoot::fromAbsolute(rootPath);
    if (!root.configured()) { std::cerr << "data root not configured\n"; return 1; }
    AppCityPlaybackPreferences preferences; std::string error;
    if (!readAppCityPlaybackPreferences(root, preferences, error)) { std::cerr << error << '\n'; return 1; }
    if(argc>1){try{return explicitPairs(root,preferences,argc,argv);}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

    const auto saves = inventory(root);
    std::cout << "FIDELITY saves " << saves.size() << "\n";
    for (const auto& s : saves) {
        std::cout << "save " << s.name << " buildings=" << s.liveSlots.size()
                  << " figures=" << (s.save.layers.figures ? s.save.layers.figures->live.size() : 0) << "\n";
    }

    unsigned compared = 0, matched = 0, blocked = 0, rejected = 0;
    for (size_t i = 0; i < saves.size(); ++i) for (size_t j = 0; j < saves.size(); ++j) {
        if (i == j) continue;
        const auto& early = saves[i]; const auto& late = saves[j];
        if (!early.clockOk || !late.clockOk || !early.sameMap(late)) continue;
        const auto distance = forwardDistance(early.clock, late.clock);
        if (!distance || !*distance) continue; // not a forward run, or the same instant
        const size_t terrain = early.terrainDiffersFrom(late), removed = removedBuildings(early, late);
        std::cout << "pair " << early.name << " -> " << late.name
                  << " terrainDiff=" << terrain << " removedBuildings=" << removed
                  << " distance=" << *distance;
        if (terrain || removed) {
            ++rejected;
            std::cout << " rejected player-action:";
            if (terrain) std::cout << " terrain";
            if (removed) std::cout << " demolition";
            std::cout << "\n";
            continue;
        }
        AppMapSurface surface(root); AppCitySessionState session; session.crtRandom = 1;
        AppCityPlayback playback(surface, early.save, early.payload, preferences, 1, 0, false, &session);
        if (!playback.prepared()) { ++blocked; std::cout << " blocked not-prepared " << playback.status().error << "\n"; continue; }
        const unsigned interval = std::max(1u, playback.status().intervalMs);
        for (unsigned n = 1; n <= *distance; ++n) if (!playback.step(uint64_t(n) * interval)) break;
        const bool reached = playback.status().updates == *distance && playback.status().refusal.empty();
        std::cout << " reached=" << playback.status().updates << "/" << *distance
                  << " refusal=" << (playback.status().refusal.empty() ? "none" : playback.status().refusal);
        if (!reached) { ++blocked; std::cout << " blocked\n"; continue; }
        const auto finances = playback.tributeFinances();
        const auto calendar = playback.status().calendar;
        auto compare = [&](const std::string& field, auto got, auto want) {
            const bool same = got == want; ++compared; if (same) ++matched;
            std::cout << "  " << field << (same ? " match" : " MISMATCH") << "\n";
        };
        std::cout << "\n";
        if (finances && late.cityOk) compare("treasury", finances->treasury, late.treasury);
        if (calendar) { compare("month", calendar->first, late.clock.month); compare("monthPart", calendar->second, late.clock.monthPart); }
    }
    std::cout << "FIDELITY " << matched << "/" << compared
              << " blocked=" << blocked << " rejected=" << rejected << "\n";
    return 0;
}
