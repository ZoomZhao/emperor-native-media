// SPDX-License-Identifier: MIT
#pragma once
// One admitted native-scoped strict update over a full copied
// OriginalCombinedState. Depends only on the strict timeline/figure code and the
// pure house plan; never on native session/source DTOs. Pure: no IO, no asset
// loading (the caller supplies already-loaded archive references).
//
// Order mirrors the existing timeline update (original_scene_timeline.hpp
// originalCombinedBeforeFigures -> ordered figure pass -> sync):
//   1. building phase via originalCombinedBuildingPhase (sync links -> strict
//      phase -> SynchronizePool -> link publish), only for supported phases;
//   2. phase advance (50 wraps only under an explicit caller policy);
//   3. ordered figure pass (originalFigureOrderedNextPass), live Well walkers only;
//   4. originalCombinedSyncBuildings.
// This is a native adaptation: unrelated scenario/event/UI prefixes, the
// post-figure pass and every unsupported phase are skipped with explicit
// events. It does not claim original timing or a complete original clock.
//
// Named isolations (persisted by the caller, never silent):
//  * well_walker_return_destination_unported: a type28 walker that reaches the
//    unported return branch is rolled back (the whole pass restarts from the
//    untouched input) and then frozen in place through the existing
//    updateOwner skip (original_figure_update.hpp ordered pass), restored to 0
//    afterwards. Its owner keeps a live primary walker, so no respawn occurs.
//  * house_target_selector_unported / house_type_evolution_path_unported:
//    phase39 houses whose strict step refuses its target (or whose type has no
//    ported evolution path) are excluded for that phase with an exact restore
//    of their state, context and canonical link; other houses continue.
// Everything else that refuses is a whole-update refusal with no candidate.
#include "native_house_evolution_plan.hpp"
#include "native_observed_dry_downgrade.hpp"
#include "original_scene_timeline.hpp"
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "native_immigrant_arrival.hpp"
namespace game_lab {
enum class NativePhaseWrapPolicy {
    RefuseAtCycleTail,          // strict: the ported phase50 tail cannot run natively
    WrapPreserveMonthPart       // adapted: 50 -> 0, tail skipped, monthPart/cycle unchanged
};
struct NativeHouseIsolation {
    std::map<size_t,std::string> heldWalkers;    // figure slot -> reason
    std::map<size_t,std::string> blockedHouses;  // building slot -> reason (phase39)
};
struct NativePhaseEvent { uint32_t phase=0; size_t slot=0; std::string kind; };
enum class NativeWellReturnPolicy { Hold, ObservedHomeRoad };
struct NativePhaseStepInputs {
    NativePhaseWrapPolicy wrap=NativePhaseWrapPolicy::RefuseAtCycleTail;
    const std::map<uint32_t,const SG3Archive*>* archives=nullptr;
    NativeWellReturnPolicy wellReturnPolicy=NativeWellReturnPolicy::Hold;
    std::optional<std::set<std::pair<uint16_t,int16_t>>> admittedHouseGrades; // caller projection contract; absent retains strict plan admission
    bool observedDryDowngrade=false;
    bool observedRewatering=false; // explicit R4; legacy R1-R3 retain their sticky guard
    bool nonemptyMigrantRehousing=false;
    bool immigration=false; // explicit type11 population authority; never enabled by old archives
    bool inspection=false;
    bool cityContextContract=false; // C1: persisted context + scheduled jobs, not full calendar
    std::map<uint64_t,uint32_t> pendingExcess;
    bool retainNativeHunterEconomy=false;
    bool retainNativeHouseFootprints=false; // native constructed collision contract; never an original type4 virtual claim
};
inline bool nativeHouseRewateringUnverified(const OriginalBuildingUpdateState& source,const NativePhaseStepInputs& in){
    const auto& i=source.inputs;
    return in.observedDryDowngrade&&source.building.type==4&&i.model==1&&i.house&&i.house->at(52)>0&&
        (i.houseWater||i.house->at(45)||i.house->at(47))&&
        !(in.observedRewatering&&i.houseWater<=1&&i.house->at(45)>0&&i.house->at(47)==0);
}
inline NativeHouseEvolutionPlan planNativePhaseHouseEvolution(const OriginalBuildingUpdateState& source,
    const OriginalBuildingUpdateContext& context,const NativePhaseStepInputs& in,const std::string& retainedReason={}){
    if(source.building.type==4&&in.observedDryDowngrade){
        const auto& i=source.inputs;
        const bool knownRecovery=in.observedRewatering&&i.house&&i.houseWater==1&&i.house->at(45)>0&&i.house->at(47)==0;
        if(nativeHouseRewateringUnverified(source,in)||
           (retainedReason=="observed_dry_downgrade_rewatered_during_delay_unverified"&&!knownRecovery)){
            NativeHouseEvolutionPlan out;out.error="observed_dry_downgrade_rewatered_during_delay_unverified";return out;
        }
        return planNativeObservedDryDowngrade(source,context,in.observedRewatering?
            NativeObservedDryDowngradePolicy::WaterOnlyType4To3AndRecoveryV2:NativeObservedDryDowngradePolicy::WaterOnlyType4To3V1);
    }
    auto out=planNativeHouseEvolution(source,context,source.inputs.houseWater);
    // C1 still has an explicitly frozen calendar. A strict phase39 candidate
    // that would debit materials at part0/7 cannot be replayed every51 ticks
    // as if a new month had elapsed. Keep that house unchanged until the
    // material/calendar contract is implemented; water-only paths still run.
    if(in.cityContextContract&&out.candidate&&source.inputs.house&&out.candidate->inputs.house&&
       originalHouseGoodsFromPayload(*source.inputs.house)!=originalHouseGoodsFromPayload(*out.candidate->inputs.house)){
        out.candidate.reset();out.candidateContext.reset();out.error="native_city_material_schedule_unmapped";
    }
    if(out.candidate&&out.retainsHouseIdentityAndGeometry&&in.admittedHouseGrades&&
       !in.admittedHouseGrades->count({out.candidate->building.type,out.candidate->inputs.model})){
        out.candidate.reset();out.candidateContext.reset();out.error="house_target_native_grade_unsupported";
    }
    return out;
}
struct NativeMigrantArrival {size_t figureSlot=0,houseSlot=0;uint32_t residents=0;uint16_t figureType=13;};
struct NativeImmigrantOffer {size_t figureSlot=0,houseSlot=0;uint16_t allocationSerial=0;uint32_t residents=0;};
struct NativePhaseStepResult {
    std::optional<OriginalCombinedState> candidate;   // whole runtime; absent on refusal
    NativeHouseIsolation isolation;                   // retained + newly recorded
    std::string refusal;
    uint32_t executedPhase=0;
    bool phaseExecuted=false;
    std::vector<NativePhaseEvent> events;
    unsigned conversions=0,houseRedraws=0,allocations=0,figuresComputed=0;
    std::map<size_t,uint32_t> departedResidents;
    std::vector<NativeMigrantArrival> arrivedResidents;
    std::vector<NativeImmigrantOffer> immigrantOffers;
};

namespace native_phase_detail {
// Strictly-ported phases admitted natively. 0/2/9/14/15/19/26/34/41/42/46/49
// are proven-empty dispatch targets in originalCombinedBuildingPhase.
inline bool supportedPhase(uint32_t p){
    switch(p){case 21:case 22:case 28:case 31:case 35:case 36:case 37:case 38:case 39:case 40:
    case 0:case 2:case 9:case 14:case 15:case 19:case 26:case 34:case 41:case 42:case 46:case 49:return true;
    default:return false;}
}
inline bool cityContextPhase(uint32_t p){return p==12||p==17||p==25;}
inline bool liveBuilding(uint8_t alive){return alive==1||alive==3;}
struct Geometry{uint16_t x,y;uint8_t size,width,height;std::optional<uint32_t> grid;};
inline Geometry geometry(const OriginalBuildingFootprint& f){
    return {f.x,f.y,f.size,f.width,f.height,f.imageInputs?std::make_optional(f.imageInputs->gridIndex):std::nullopt};}
inline bool sameGeometry(const Geometry& a,const Geometry& b){
    return a.x==b.x&&a.y==b.y&&a.size==b.size&&a.width==b.width&&a.height==b.height&&a.grid==b.grid;}
// Live figures must all be Well walkers owned by a live Well link.
inline std::string checkZeroLink(const OriginalFigureUpdateContext& f){
    if(!f.buildings||!f.buildings->count(0))return "native_migrant_zero_link_missing";
    const auto& z=f.buildings->at(0);
    if(z.type||z.alive||z.worker||z.primaryFigure||z.residents||z.freeSlots)return "native_migrant_zero_link_changed";
    return {};
}
inline std::string checkWalkers(const OriginalCombinedState& city,bool observedDry=false,bool nonemptyRehousing=false,bool immigration=false,bool inspection=false){
    const auto& f=city.figures;
    if(f.figures.size()!=2000)return "native_figure_pool_required";
    if(nonemptyRehousing){if(!observedDry)return "native_rehousing_requires_dry_runtime";if(auto e=checkZeroLink(f);!e.empty())return e;}
    for(const auto& e:f.figures){const auto& w=e.second;if(!w.alive)continue;
        if(w.type==11&&immigration){
            if(w.alive!=1||w.owner||w.action<6||w.action>8||!w.employmentAmount||w.home<=0||!f.buildings||!f.buildings->count(size_t(w.home)))return "native_immigrant_identity_invalid";
            const auto& h=f.buildings->at(size_t(w.home));
            if(h.type<2||h.type>=18||!liveBuilding(h.alive)||h.worker!=int(w.slot)||h.residents<0)return "native_immigrant_target_invalid";
            continue;
        }
        if(w.type==13&&observedDry){
            // Only the observed ownerless emigrant and the existing exit route
            // are admitted. Rehousing is held before reserving another house.
            if(w.alive!=1||w.owner||w.action<6||w.action>9||w.employmentAmount==0)return "native_migrant_identity_unsupported_slot_"+std::to_string(e.first);
            if(!nonemptyRehousing){if(w.home)return "native_migrant_identity_unsupported_slot_"+std::to_string(e.first);}
            else if(w.home){
                if(w.home<0||(w.action!=7&&w.action!=8)||!f.buildings||!f.buildings->count(size_t(w.home)))return "native_migrant_home_invalid_slot_"+std::to_string(e.first);
                const auto& h=f.buildings->at(size_t(w.home));
                if(h.type<2||h.type>=18||!liveBuilding(h.alive)||h.worker!=int(e.first)||h.residents<0)return "native_migrant_home_reservation_invalid_slot_"+std::to_string(e.first);
            }else if(w.action!=6&&w.action!=9)return "native_migrant_unreserved_action_slot_"+std::to_string(e.first);
            continue;
        }
        if(w.type==39&&inspection){
            if(!f.buildings||w.owner<=0||w.alive!=1||w.action<6||w.action>9)return "inspection_owner_reference_invalid";
            const auto owner=f.buildings->find(size_t(w.owner));
            if(owner==f.buildings->end()||owner->second.type!=124||owner->second.alive!=1)return "inspection_owner_reference_invalid";
            const auto& b=owner->second;
            if(b.primaryFigure!=int(w.slot)&&(!b.inspectorSecondary||b.secondaryFigures[1]!=int(w.slot)))return "inspection_auxiliary_reference_invalid";
            continue;
        }
        if(w.type!=28)return "native_live_figure_type_"+std::to_string(w.type)+"_unsupported_slot_"+std::to_string(e.first);
        if(!f.buildings)return "native_building_links_required";
        const auto owner=f.buildings->find(size_t(w.owner));
        if(w.owner<=0||owner==f.buildings->end()||owner->second.type!=0x48||owner->second.alive!=1)
            return "well_walker_owner_not_live_well_cleanup_unported_slot_"+std::to_string(e.first);
        // Strict membership (originalBuildingHookFigure / originalServiceFirstUpdate):
        // a Well owns its walker only through primaryFigure. Without it phase31
        // would respawn while this record (possibly held) stays alive.
        if(owner->second.primaryFigure!=int(e.first))
            return (owner->second.primaryFigure?"well_walker_owner_primary_other_slot_":"well_walker_owner_primary_missing_slot_")+std::to_string(e.first);}
    if(nonemptyRehousing)for(const auto& [slot,h]:*f.buildings)if(slot&&h.type>=2&&h.type<18&&h.worker){
        const auto worker=f.figures.find(size_t(h.worker));
        if(h.worker<0||worker==f.figures.end()||worker->second.alive!=1||(worker->second.type!=13&&!(immigration&&worker->second.type==11))||worker->second.home!=int(slot)||(worker->second.action!=7&&worker->second.action!=8&&!(immigration&&worker->second.type==11&&worker->second.action==6)))return "native_house_migrant_reservation_dangling_"+std::to_string(slot);
    }
    return {};
}
// Explicit native cancellation when a constructed destination is removed.
// Keeps the already-departed people alive in the same cell/FIFO position;
// only the reservation/owned route is released, then wait/search resumes.
inline std::string checkRetainedIsolation(const OriginalCombinedState& city,const NativeHouseIsolation& retained,bool observedDry){
    for(const auto& h:retained.heldWalkers){const auto f=city.figures.figures.find(h.first);
        if(f==city.figures.figures.end()||f->second.alive!=1||(f->second.type!=28&&!(observedDry&&f->second.type==13)))return "held_walker_not_live_supported_slot_"+std::to_string(h.first);
        if(f->second.updateOwner)return "held_walker_update_owner_nonzero_slot_"+std::to_string(h.first);}
    for(const auto& h:retained.blockedHouses)if(!city.buildings.count(h.first))return "blocked_house_missing_slot_"+std::to_string(h.first);
    return {};
}
inline std::string cancelMigrantReservation(OriginalCombinedState& value,size_t slot,bool immigration=false){
    if(auto e=checkWalkers(value,true,true,immigration);!e.empty())return e;
    auto next=value;auto& f=next.figures;
    const auto it=f.figures.find(slot);if(it==f.figures.end())return "native_migrant_cancel_slot_missing";
    auto& w=it->second;
    if(w.alive!=1||w.type!=13||w.owner||w.home<=0||(w.action!=7&&w.action!=8)||!f.services||!f.buildings)return "native_migrant_cancel_not_reserved";
    auto& house=f.buildings->at(size_t(w.home));if(house.worker!=int(slot))return "native_migrant_cancel_reservation_mismatch";
    if(w.route<0||w.route>=1000)return "native_migrant_cancel_route_invalid";
    if(w.route){if(f.services->routeOwners[size_t(w.route)]!=int(slot))return "native_migrant_cancel_route_not_owned";f.services->routeOwners[size_t(w.route)]=0;}
    house.worker=0;w.home=0;w.action=6;w.counter=0;w.route=w.routeCursor=w.routeLength=w.routePhase=0;
    w.freeMovement=0;w.hidden=0;w.freeRemainingX=w.freeRemainingY=w.freeError=w.freeAxis=0;w.roamSteps=0;
    w.destinationX=w.x;w.destinationY=w.y;w.destinationCell=w.cell;
    if(auto e=originalCombinedSyncBuildings(next);!e.empty())return e;
    if(next.buildingContext){next.buildingContext->buildingLinks=f.buildings;
        if(next.buildingContext->buildingInputs)for(const auto& [id,b]:next.buildings)next.buildingContext->buildingInputs->at(id)=b.state.inputs;}
    if(auto e=checkWalkers(next,true,true,immigration);!e.empty())return e;
    value=std::move(next);return {};
}
// Fields that originalCombinedBuildingPhase/originalCombinedSyncBuildings
// dereference or index with at(): verified before any strict call, so a
// shaped-but-incomplete state is a named atomic refusal, never UB/throw.
inline std::string checkBuildingPhaseInputs(const OriginalCombinedState& city,uint32_t phase){
    const auto& c=*city.buildingContext;const auto& f=city.figures;
    if(!c.buildingInputs||!c.buildings)return "native_building_pool_required";
    if(!f.services)return "native_figure_services_required";
    if(!f.buildings)return "native_building_links_required";
    for(size_t n=0;n<f.figures.size();++n){const auto found=f.figures.find(n);
        if(found==f.figures.end()||found->second.slot!=n)return "native_figure_pool_slot_"+std::to_string(n);}
    if(c.buildingInputs->size()!=city.buildings.size()||c.buildings->size()!=city.buildings.size())
        return "native_building_pool_coverage";
    for(const auto& e:city.buildings){const size_t slot=e.first;const auto& s=e.second.state;
        const auto in=c.buildingInputs->find(slot);const auto fp=c.buildings->find(slot);
        if(in==c.buildingInputs->end()||fp==c.buildings->end())return "native_building_pool_slot_missing_"+std::to_string(slot);
        if(s.building.slot!=slot||fp->second.slot!=slot)return "native_building_slot_identity_"+std::to_string(slot);
        if(!f.buildings->count(slot))return "combined_building_link_required";
        if(s.production&&(!c.productionInputs||!c.productionInputs->count(slot)))return "native_production_pool_slot_missing_"+std::to_string(slot);
        // Phase31 crop hooks dereference month/monthPart for these types only.
        if(phase==31&&(s.building.type==194||s.building.type==196)&&(!c.monthPart||!c.month))return "native_crop_hook_calendar_required";
        if(phase==39&&originalBuildingIsHouse(s.building.type)&&!c.monthPart)return "house_cycle_unmapped";}
    return {};
}
inline void advanceContexts(OriginalCombinedState& city,uint32_t next,bool fullSync=false){
    city.phase=next;if(city.figures.goods)city.figures.goods->savedPhase=next;
    if(fullSync&&city.buildingContext){city.buildingContext->phase=next;city.buildingContext->goods=city.figures.goods;}
    for(auto& e:city.buildings)e.second.context.phase=next;
}
} // namespace native_phase_detail

inline NativePhaseStepResult stepNativeHousePhase(const OriginalCombinedState& admitted,
    const NativeHouseIsolation& retained,const NativePhaseStepInputs& in){
    using namespace native_phase_detail;
    NativePhaseStepResult out;out.isolation=retained;
    // A refusal publishes nothing: no candidate, no newly recorded isolation,
    // no counts. Events remain only as diagnostics of the refused attempt.
    const auto refuse=[&](std::string why){out.refusal=std::move(why);out.candidate.reset();out.isolation=retained;
        out.conversions=out.houseRedraws=out.allocations=out.figuresComputed=0;out.departedResidents.clear();out.arrivedResidents.clear();out.immigrantOffers.clear();out.phaseExecuted=false;return out;};
    if(in.observedRewatering&&!in.observedDryDowngrade)return refuse("water_recovery_requires_observed_dry_runtime");
    if(!in.archives)return refuse("native_figure_archives_required");
    if(admitted.phase>50)return refuse("invalid_city_phase");
    if(!admitted.buildingContext)return refuse("native_building_context_required");
    if(auto w=checkWalkers(admitted,in.observedDryDowngrade,in.nonemptyMigrantRehousing,in.immigration,in.inspection);!w.empty())return refuse(w);
    if(auto e=checkRetainedIsolation(admitted,retained,in.observedDryDowngrade);!e.empty())return refuse(e);

    const auto retainUnverifiedRewatering=[&](const OriginalCombinedState& value){
        if(!in.observedDryDowngrade)return;
        for(const auto& [slot,b]:value.buildings)if(nativeHouseRewateringUnverified(b.state,in))
            out.isolation.blockedHouses[slot]="observed_dry_downgrade_rewatered_during_delay_unverified";
    };
    retainUnverifiedRewatering(admitted);
    auto city=admitted;city.figures.captureInspectionVisits=in.inspection;city.figures.inspectionVisits.clear();out.executedPhase=city.phase;const uint32_t phase=city.phase;
    std::map<size_t,Geometry> before;
    for(const auto& e:city.buildings)before.emplace(e.first,geometry(e.second.state.building));
    const auto repairsBefore=city.buildingContext->ownershipRepairs;
    const auto conversionsBefore=city.buildingContext->houseConversions,redrawsBefore=city.buildingContext->houseRedraws;
    const auto allocationsBefore=city.figures.allocationCount;
    std::set<size_t> liveBefore;for(const auto& e:city.figures.figures)if(e.second.alive)liveBefore.insert(e.first);

    // ---- 1. building phase ----
    if(phase==50){
        if(in.wrap!=NativePhaseWrapPolicy::WrapPreserveMonthPart)return refuse("native_phase50_cycle_tail_unsupported");
        out.events.push_back({50,0,"native_cycle_tail_skipped_month_part_preserved"});
    }else if(!((in.inspection&&(phase==43||phase==44||phase==45))||supportedPhase(phase)||(phase==24&&in.observedDryDowngrade)||(phase==23&&in.immigration)||(in.cityContextContract&&cityContextPhase(phase)))){
        out.events.push_back({phase,0,"native_phase_skipped"});
    }else{
        if(auto p=checkBuildingPhaseInputs(city,phase);!p.empty())return refuse(p);
        std::map<size_t,std::string> blocked;
        std::map<size_t,NativeHouseEvolutionPlan> dryPlans;
        if(phase==23&&in.immigration){
            const auto& c=*city.buildingContext;
            if(!c.labor||!c.cityEmployment||!c.formations)return refuse("native_immigration_labor_inputs_required");
            // Full phase23 runs transactionally for the supported native subset.
            // The positive-health birth/death statistics and military domains
            // are separate unported writers, never silently discarded.
            if(c.labor->statisticsPending)return refuse("native_population_statistics_pending_unported");
            for(const auto& f:*c.formations)if(f.alive)return refuse("native_population_military_unported");
            for(const auto n:c.labor->formationLabor)if(n)return refuse("native_population_formation_labor_unported");
            city.process.migrantSpacing=c.labor->spacing;
        }
        if(phase==24){
            if(!in.observedDryDowngrade)return refuse("native_excess_policy_required");
            for(const auto& [slot,b]:city.buildings)if(b.state.inputs.house&&b.state.inputs.free<0){
                const auto& i=b.state.inputs;const auto pending=in.pendingExcess.find(slot);
                if(b.state.building.type!=3||i.model!=0||pending==in.pendingExcess.end()||pending->second!=unsigned(-int(i.free))||pending->second>255||i.residents<=int(pending->second))return refuse("native_excess_not_observed_pending_slot_"+std::to_string(slot));
            }
        }
        if(phase==39){
            // Screen every house on copies with the pure plan (strict step).
            auto probe=city;if(auto s=originalCombinedSyncBuildings(probe);!s.empty())return refuse(s);
            auto& pc=*probe.buildingContext;
            if(!pc.buildingInputs||!pc.buildings)return refuse("native_building_pool_required");
            if(!pc.monthPart)return refuse("house_cycle_unmapped");
            for(const auto& e:probe.buildings){pc.buildingInputs->at(e.first)=e.second.state.inputs;pc.buildings->at(e.first)=e.second.state.building;}
            for(const auto& e:probe.buildings){const auto& s=e.second.state;
                if(!originalBuildingIsHouse(s.building.type)||!liveBuilding(s.inputs.alive))continue;
                const auto held=out.isolation.blockedHouses.find(e.first);
                const auto plan=planNativePhaseHouseEvolution(s,pc,in,held==out.isolation.blockedHouses.end()?std::string{}:held->second);
                if(s.building.type==4&&in.observedDryDowngrade){
                    if(plan.candidate){dryPlans.emplace(e.first,plan);continue;}
                    blocked[e.first]=plan.error.empty()?"observed_dry_type4_not_admitted":plan.error;continue;
                }
                if(plan.candidate){
                    if(!plan.retainsHouseIdentityAndGeometry)return refuse(plan.invariantError+"_slot_"+std::to_string(e.first));
                    continue;}
                if(plan.error=="native_city_material_schedule_unmapped"){blocked[e.first]=plan.error;continue;}
                if(plan.error=="house_target_native_grade_unsupported"){blocked[e.first]=plan.error;continue;}
                if(plan.error=="house_type_unsupported"){blocked[e.first]="house_type_"+std::to_string(s.building.type)+"_evolution_path_unported";continue;}
                if(plan.error=="house_evolution_target_unsupported"){blocked[e.first]="house_target_selector_unported";continue;}
                return refuse(plan.error+"_slot_"+std::to_string(e.first));}
            // Mask excluded houses in the canonical links for this phase only.
            if(!city.figures.buildings)return refuse("native_building_links_required");
            for(const auto& b:blocked)city.figures.buildings->at(b.first).type=0;
            for(const auto& b:dryPlans)city.figures.buildings->at(b.first).type=0;
        }
        std::set<size_t> excludedHooks;
        if(phase==31&&in.retainNativeHunterEconomy)for(const auto& e:city.buildings)if(e.second.state.building.type==33){
            excludedHooks.insert(e.first);out.events.push_back({31,e.first,"native_v6_hunter_dispatch_retained"});}
        // The dispatch hook's BeginRoam examines even non-road neighbouring
        // owners. Retain the same solid 2x2 native footprint view used by the
        // movement/road adapter. Both type2 and type4 house hooks are no-ops
        // in phase31; restore every canonical type before any figure pass or
        // publication. No strict whitelist or grade/evolution rule is changed.
        std::set<size_t> retainedHouseTypes;
        if(phase==31&&in.retainNativeHouseFootprints&&city.figures.buildings)
            for(auto& [slot,link]:*city.figures.buildings)if(link.type==4){
                retainedHouseTypes.insert(slot);link.type=2;
                out.events.push_back({31,slot,"native_house_type4_initial_roam_footprint_retained"});}
        if(in.inspection&&phase==44){
            // Probe the exact copied update/RNG with disasters unavailable.
            // A positive hazard is an explicit unsupported outcome, never a
            // half-committed building replacement or a silently zeroed risk.
            auto probe=*city.buildingContext;probe.figureServices=city.figures.services;probe.disasters.reset();
            const auto hazard=originalBuildingRiskStep(probe);
            if(hazard==OriginalBuildingUpdateBlocker::RiskAndFire)return refuse("inspection_disaster_trigger_unported");
            if(hazard!=OriginalBuildingUpdateBlocker::None)return refuse(std::string("inspection_risk_inputs:")+originalBuildingUpdateBlockerName(hazard));
        }
        if(auto r=originalCombinedBuildingPhase(city,excludedHooks);!r.empty())return refuse(r);
        for(size_t slot:retainedHouseTypes){
            auto& c=*city.buildingContext;
            if(!city.figures.buildings||!city.figures.buildings->count(slot)||!city.buildings.count(slot)||
               !c.buildings||!c.buildings->count(slot)||!c.buildingLinks||!c.buildingLinks->count(slot))
                return refuse("native_retained_house_link_missing");
            city.figures.buildings->at(slot).type=4;city.buildings.at(slot).state.building.type=4;
            c.buildings->at(slot).type=4;c.buildingLinks->at(slot).type=4;
            if(c.houses&&c.houses->count(slot))c.houses->at(slot).type=4;
        }
        out.phaseExecuted=true;
        if(phase==24)for(const auto& [slot,b]:city.buildings){
            const int beforeResidents=admitted.buildings.at(slot).state.inputs.residents;
            const int afterResidents=b.state.inputs.residents;
            if(afterResidents<beforeResidents)out.departedResidents[slot]=uint32_t(beforeResidents-afterResidents);
        }
        if(phase==39){
            auto& c=*city.buildingContext;
            for(const auto& [slot,plan]:dryPlans){
                const auto& pc=*plan.candidateContext;const auto& base=*admitted.buildingContext;
                city.buildings.at(slot).state=*plan.candidate;
                c.buildingInputs->at(slot)=plan.candidate->inputs;c.buildings->at(slot)=plan.candidate->building;
                c.buildingLinks->at(slot)=pc.buildingLinks->at(slot);city.figures.buildings->at(slot)=pc.buildingLinks->at(slot);
                c.houseConversions+=pc.houseConversions-base.houseConversions;
                c.houseRedraws+=pc.houseRedraws-base.houseRedraws;
                c.houseEvolutionCalls+=pc.houseEvolutionCalls-base.houseEvolutionCalls;
                if(pc.buildingRedraws.count(slot))c.buildingRedraws.insert(slot);
                out.events.push_back({39,slot,plan.direction==0?"observed_water_only_delay_reset":plan.candidate->building.type==3?"observed_dry_type4_to3":"observed_dry_type4_delay"});
            }
            for(const auto& b:blocked){const size_t slot=b.first;
                const auto& original=admitted.buildings.at(slot);
                city.buildings.at(slot).state=original.state;
                const auto link=admitted.figures.buildings->at(slot);
                city.figures.buildings->at(slot)=link;
                if(c.buildingLinks)c.buildingLinks->at(slot)=link;
                c.buildingInputs->at(slot)=original.state.inputs;c.buildings->at(slot)=original.state.building;
                if(!retained.blockedHouses.count(slot)||retained.blockedHouses.at(slot)!=b.second)
                    out.events.push_back({39,slot,b.second});
                out.isolation.blockedHouses[slot]=b.second;}
            for(auto it=out.isolation.blockedHouses.begin();it!=out.isolation.blockedHouses.end();)
                if(!blocked.count(it->first)){out.events.push_back({39,it->first,"house_isolation_cleared"});it=out.isolation.blockedHouses.erase(it);}else ++it;
        }
    }
    if(in.immigration&&phase==23)city.process.migrantSpacing=city.buildingContext->labor->spacing;
    if(in.inspection&&phase==40){
        // Explicit native lifecycle policy: when a tower loses its upgraded
        // model, retire only its auxiliary patrol while its owner still exists.
        // The base owner gate cannot update that patrol on a later call.
        bool retired=false;
        for(const auto& [id,b]:city.buildings)if(b.state.building.type==124&&
            b.state.inputs.model<=0&&!b.state.inputs.modelUpgrade.value_or(0)){
            const auto secondary=city.figures.buildings->at(id).secondaryFigures[1];if(!secondary)continue;
            const auto found=city.figures.figures.find(size_t(secondary));
            if(secondary<0||found==city.figures.figures.end()||found->second.alive!=1||found->second.type!=39||found->second.owner!=int(id))return refuse("inspection_auxiliary_reference_invalid");
            auto dead=found->second;dead.alive=2;
            if(!originalFigureRemoveArrivedWorker(dead,city.figures))return refuse("inspection_downgrade_cleanup_refused");
            retired=true;out.events.push_back({40,id,"native_inspector_secondary_cancelled_on_downgrade_v1"});
        }
        if(retired){if(auto error=originalCombinedSyncBuildings(city);!error.empty())return refuse(error);
            city.buildingContext->buildingLinks=city.figures.buildings;
            for(const auto& [id,b]:city.buildings)city.buildingContext->buildingInputs->at(id)=b.state.inputs;}
    }
    // Identity/geometry/ownership invariants: any anomaly refuses the update.
    if(city.buildingContext->ownershipRepairs!=repairsBefore)return refuse("native_ownership_repair_refused");
    if(city.buildings.size()!=before.size())return refuse("native_building_identity_changed");
    for(const auto& e:city.buildings){const auto b=before.find(e.first);
        if(b==before.end()||!sameGeometry(b->second,geometry(e.second.state.building)))return refuse("native_building_geometry_changed_slot_"+std::to_string(e.first));
        if(!liveBuilding(e.second.state.inputs.alive))return refuse("native_building_retired_slot_"+std::to_string(e.first));}
    for(const auto& e:city.figures.figures)if(e.second.alive&&!liveBefore.count(e.first)&&e.second.type!=28&&!(in.inspection&&phase==31&&e.second.type==39)&&!(in.observedDryDowngrade&&phase==24&&e.second.type==13)&&!(in.immigration&&phase==23&&e.second.type==11))
        return refuse("native_non_well_allocation_type_"+std::to_string(e.second.type));

    if(in.immigration&&phase==23)for(const auto& w:city.buildingContext->createdFigureRecords)if(w.type==11){
        if(liveBefore.count(w.slot)||!w.alive||w.home<=0||!w.employmentAmount)return refuse("native_immigrant_offer_invalid");
        out.immigrantOffers.push_back({w.slot,size_t(w.home),w.allocationSerial,w.employmentAmount});
    }
    // ---- 2. phase advance (timeline order: before the figure pass) ----
    advanceContexts(city,phase==50?0:phase+1,in.cityContextContract);

    // An obsolete road-distance record can allocate an arrival in the same
    // pass in which a freshly rebuilt native road graph has no entrance.
    // Cancel this native-owned offer normally; never leave a dead reservation.
    if(in.immigration)for(auto& [slot,w]:city.figures.figures)if(w.alive==1&&w.type==11&&w.action==6&&w.counter<=1){
        const auto road=city.figures.workerRoadExits.find(size_t(w.home));
        if(road==city.figures.workerRoadExits.end()||!road->second){const size_t home=size_t(w.home);auto dead=w;dead.alive=2;
            if(!originalFigureRemoveArrivedWorker(dead,city.figures))return refuse("native_immigrant_unreachable_cleanup_failed");
            city.figures.buildings->at(home).worker=0;out.events.push_back({phase,slot,"native_immigrant_unreachable_offer_cancelled"});}
    }
    // Native reservation-capacity policy: strict type11 only clips an empty
    // house. A later capacity decrease must not overfill an occupied target.
    // Excess is accounted as unplaced by the owning bridge, not as residents.
    if(in.immigration)for(auto& [slot,w]:city.figures.figures)if(w.alive==1&&w.type==11&&w.action==8){
        auto& h=city.figures.buildings->at(size_t(w.home));const auto cap=city.figures.houseCapacities.find(h.type==11?11:h.houseModel);
        if(cap==city.figures.houseCapacities.end())return refuse("native_immigrant_capacity_missing");
        const auto amount=std::min<int>(w.employmentAmount,std::max(0,cap->second-h.residents));
        if(!amount){auto dead=w;dead.alive=2;if(!originalFigureRemoveArrivedWorker(dead,city.figures))return refuse("native_immigrant_full_cleanup_failed");h.worker=0;out.events.push_back({phase,slot,"native_immigrant_full_target_cancelled"});}
        else w.employmentAmount=uint8_t(amount);
    }
    // ---- 3. ordered figure pass with named holds ----
    if(auto w=checkWalkers(city,in.observedDryDowngrade,in.nonemptyMigrantRehousing,in.immigration,in.inspection);!w.empty())return refuse(w);
    auto held=out.isolation.heldWalkers;
    if(in.observedDryDowngrade)for(const auto& [slot,w]:city.figures.figures)if(w.alive==1&&w.type==13&&!held.count(slot)){
        const bool search=(w.action==6&&w.counter>=51)||(w.action==9&&w.counter>=30);
        if(search){const auto home=originalFigureNearestHouse(w,city.figures);
            if(!home)return refuse("native_migrant_house_registry_required");
            if(*home&&(!in.nonemptyMigrantRehousing||city.figures.buildings->at(*home).residents==0)){
                held[slot]=in.nonemptyMigrantRehousing?"native_migrant_empty_house_conversion_unported":"native_migrant_rehousing_unverified";out.events.push_back({phase,slot,held[slot]});}
        }
    }
    if(in.nonemptyMigrantRehousing){
        for(auto it=held.begin();it!=held.end();){
            if(it->second!="native_migrant_empty_house_conversion_unported"){++it;continue;}
            const auto& w=city.figures.figures.at(it->first);bool ready=false;
            if(w.home>0)ready=city.figures.buildings->at(size_t(w.home)).residents>0;
            else{const auto target=originalFigureNearestHouse(w,city.figures);ready=target&&(!*target||city.figures.buildings->at(*target).residents>0);}
            if(ready){out.events.push_back({phase,it->first,"native_migrant_nonempty_target_available"});it=held.erase(it);}else ++it;
        }
        for(const auto& [slot,w]:city.figures.figures)if(w.alive==1&&w.type==13&&w.action==8&&w.home>0&&!held.count(slot)){
            const auto& h=city.figures.buildings->at(size_t(w.home));
            if(h.residents==0&&h.laborDisabled&&!*h.laborDisabled&&int(w.freeRemainingX)+w.freeRemainingY<1){held[slot]="native_migrant_empty_house_conversion_unported";out.events.push_back({phase,slot,held[slot]});}
        }
    }
    // Previously held native observed cases may recover only when the exact
    // same narrow guard is proved again (e.g. the original road is restored).
    if(in.wellReturnPolicy==NativeWellReturnPolicy::ObservedHomeRoad){
        auto probe=city.figures;probe.wellReturnObservedHomePolicy=true;
        for(auto it=held.begin();it!=held.end();){
            const auto& w=probe.figures.at(it->first);
            bool recover=it->second.rfind("native_well_return_observed:",0)==0&&
                originalFigureObservedWellReturnBlocker(w,probe,w.action==2).empty();
            if(recover&&it->second.rfind("native_well_return_observed:route_step_",0)==0){
                const auto step=originalServiceFirstUpdate(w,*in.archives,&probe,probe.terrain.at(w.cell));
                recover=step.state&&step.status==OriginalFigureUpdateStatus::Computed;
            }
            if(recover){
                out.events.push_back({phase,it->first,"native_well_return_guard_recovered"});it=held.erase(it);
            }else ++it;
        }
        for(const auto& [slot,w]:probe.figures)if(w.alive==1&&w.type==28&&w.action==2&&!held.count(slot)){
            const auto reason=originalFigureObservedWellReturnBlocker(w,probe,true);
            if(!reason.empty()){held[slot]="native_well_return_observed:"+reason;out.events.push_back({phase,slot,held[slot]});}
        }
    }
    for(unsigned attempt=0;;++attempt){
        if(attempt>2000)return refuse("native_hold_retry_limit");
        auto context=city.figures;
        context.wellReturnObservedHomePolicy=in.wellReturnPolicy==NativeWellReturnPolicy::ObservedHomeRoad?std::make_optional(true):std::nullopt;
        for(const auto& h:held){auto found=context.figures.find(h.first);
            if(found==context.figures.end()||found->second.alive!=1||(found->second.type!=28&&!(in.observedDryDowngrade&&found->second.type==13))||found->second.updateOwner)
                return refuse("held_walker_not_live_well_slot_"+std::to_string(h.first));
            found->second.updateOwner=1;}
        auto pass=originalFigureOrderedNextPass(context,*in.archives);
        if(pass.complete&&pass.context){
            if(in.immigration)for(const auto& [slot,w]:city.figures.figures)if(w.alive==1&&w.type==11&&!pass.context->figures.at(slot).alive){
                auto& h=pass.context->buildings->at(size_t(w.home));const auto& prior=city.figures.buildings->at(size_t(w.home));
                if(h.worker==int(slot)){
                    if(h.residents!=prior.residents||h.type!=prior.type||h.houseModel!=prior.houseModel)return refuse("native_immigrant_cancel_changed_house");
                    h.worker=0;out.events.push_back({phase,slot,"native_immigrant_cancel_reservation_released"});}
            }
            if(in.nonemptyMigrantRehousing)for(const auto& [houseId,prior]:*city.figures.buildings){
                if(!houseId||prior.type<2||prior.type>=18)continue;
                const auto& after=pass.context->buildings->at(houseId);
                if(after.residents<prior.residents)return refuse("native_figure_residents_decreased");
                const uint32_t delta=uint32_t(after.residents-prior.residents);if(!delta)continue;
                if(prior.worker<=0||!city.figures.figures.count(size_t(prior.worker)))return refuse("native_arrival_without_reservation");
                if(in.immigration&&city.figures.figures.at(size_t(prior.worker)).type==11){
                    std::string error;const auto proof=proveNativeImmigrantArrival(city.figures,*pass.context,houseId,error);
                    if(!proof)return refuse("native_immigrant_arrival:"+error);
                    out.arrivedResidents.push_back({proof->figureSlot,houseId,proof->arrived,11});
                    out.events.push_back({phase,proof->figureSlot,proof->convertedEmpty?"native_immigrant_first_arrival_type3":"native_immigrant_arrived"});continue;
                }
                if(prior.residents<=0)return refuse("native_type13_empty_conversion_unported");
                const auto& w=city.figures.figures.at(size_t(prior.worker));
                const int key=prior.type==11?11:prior.houseModel;const auto cap=city.figures.houseCapacities.find(key);
                if(w.type!=13||w.action!=8||w.home!=int(houseId)||held.count(w.slot)||int(w.freeRemainingX)+w.freeRemainingY>=1||!prior.laborDisabled||*prior.laborDisabled||cap==city.figures.houseCapacities.end()||delta!=uint32_t(std::min<int>(w.employmentAmount,std::max(0,cap->second-prior.residents))))return refuse("native_arrival_delta_unproved");
                const auto& removed=pass.context->figures.at(w.slot);
                if(removed.alive||std::count(pass.context->freeSlots.begin(),pass.context->freeSlots.end(),w.slot)!=1)return refuse("native_arrival_cleanup_unproved");
                out.arrivedResidents.push_back({w.slot,houseId,delta});out.events.push_back({phase,w.slot,"native_migrant_arrived_nonempty_house"});
            }
            if(in.immigration){uint64_t sum=0;for(const auto& a:out.arrivedResidents)sum+=a.residents;
                if(!city.figures.services||!city.figures.services->population||!pass.context->services||!pass.context->services->population||
                   int64_t(pass.context->services->population->total)-city.figures.services->population->total!=int64_t(sum))return refuse("native_arrival_pass_population_mismatch");}
            // Policy is an ephemeral caller input, not hidden authoritative
            // state. The versioned native policy tag is what is persisted.
            pass.context->wellReturnObservedHomePolicy=city.figures.wellReturnObservedHomePolicy;
            for(const auto& [slot,w]:city.figures.figures)if(w.alive==1&&w.type==28&&w.action==1){
                const auto next=pass.context->figures.find(slot);
                if(next!=pass.context->figures.end()&&next->second.alive==1&&next->second.action==2)
                    out.events.push_back({phase,slot,"native_well_return_observed_home_road"});
            }
            for(const auto& h:held){pass.context->figures.at(h.first).updateOwner=0;
                const auto prior=city.figures.primaryImageInputs.find(h.first);
                if(prior==city.figures.primaryImageInputs.end())pass.context->primaryImageInputs.erase(h.first);
                else pass.context->primaryImageInputs[h.first]=prior->second;}
            out.figuresComputed=unsigned(pass.computed);city.figures=std::move(*pass.context);break;}
        const size_t slot=pass.firstBlockedSlot;
        const auto status=slot&&pass.figures.count(slot)?pass.figures.at(slot).status:OriginalFigureUpdateStatus::OrderedPoolPass;
        if(slot&&status==OriginalFigureUpdateStatus::WellReturnDestination&&!held.count(slot)&&
           city.figures.figures.at(slot).type==28){
            held[slot]="well_walker_return_destination_unported";
            if(in.wellReturnPolicy==NativeWellReturnPolicy::ObservedHomeRoad){
                const auto why=originalFigureObservedWellReturnBlocker(city.figures.figures.at(slot),context);
                held[slot]="native_well_return_observed:"+(why.empty()?std::string("transition_refused"):why);
            }
            out.events.push_back({phase,slot,held[slot]});continue;}
        if(slot&&in.wellReturnPolicy==NativeWellReturnPolicy::ObservedHomeRoad&&!held.count(slot)&&
           city.figures.figures.at(slot).type==28&&city.figures.figures.at(slot).action==2&&
           (status==OriginalFigureUpdateStatus::RouteGeneration||status==OriginalFigureUpdateStatus::RouteBoundary||status==OriginalFigureUpdateStatus::NextDirection)){
            held[slot]="native_well_return_observed:route_step_"+std::string(originalFigureUpdateStatusName(status));
            out.events.push_back({phase,slot,held[slot]});continue;
        }
        if(in.inspection&&status==OriginalFigureUpdateStatus::InspectorFireSearch)return refuse("inspection_fire_target_unported");
        return refuse(std::string("figure_pass_")+originalFigureUpdateStatusName(status)+"_slot_"+std::to_string(slot));
    }
    out.isolation.heldWalkers=held;
    if(auto w=checkWalkers(city,in.observedDryDowngrade,in.nonemptyMigrantRehousing,in.immigration,in.inspection);!w.empty())return refuse(w);
    // Canonical shared figure services are city.figures.services. Republish the
    // mirrors the next building phase/timeline would otherwise read stale.
    if(city.figures.services){city.buildingContext->figureServices=city.figures.services;
        city.process.randomHistory=city.figures.services->randomHistory;
        if(in.nonemptyMigrantRehousing&&city.figures.services->population){
            city.buildingContext->populationTotal=city.figures.services->population->total;
            city.buildingContext->populationPeak=city.figures.services->population->peak;
        }}
    if(in.cityContextContract){city.buildingContext->goods=city.figures.goods;city.buildingContext->phase=city.phase;}
    if(in.inspection)city.process.effectDecayCounter=city.buildingContext->effectDecayCounter;

    // ---- 4. canonical sync after figures ----
    if(auto s=originalCombinedSyncBuildings(city);!s.empty())return refuse(s);
    // The capacity registry is a projection of the same committed building
    // inputs, not a second authoritative model/resident store. Keep its peak.
    if(city.buildingContext->houses)for(auto& e:*city.buildingContext->houses){
        const auto& b=city.buildings.at(e.first).state;const auto& i=b.inputs;
        e.second.type=b.building.type;e.second.model=i.model;e.second.residents=i.residents;e.second.free=i.free;e.second.distance=i.distance;
        if(i.house)e.second.laborDisabled=i.house->at(55);
    }
    retainUnverifiedRewatering(city);
    out.events.push_back({phase,0,"native_post_figure_pass_skipped"});
    out.conversions=unsigned(city.buildingContext->houseConversions-conversionsBefore);
    out.houseRedraws=unsigned(city.buildingContext->houseRedraws-redrawsBefore);
    out.allocations=unsigned(city.figures.allocationCount-allocationsBefore);
    out.candidate=std::move(city);return out;
}
} // namespace game_lab
