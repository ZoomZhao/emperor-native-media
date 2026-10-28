// SPDX-License-Identifier: MIT
// Authored fixtures only for the strict read-only house query. Real data is
// never printed; --real reports aggregate categories or skips when unset.
#include "native_house_evolution.hpp"
#include "native_session_presentation.hpp"
#include "native_start_catalog.hpp"
#include "original_pak.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace game_lab;
void need(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}

struct HouseFixture {
    int32_t down=-99,up=100,currentWater=0,currentGoods=0;
    int32_t targetWater=0,targetGoods=0;
};

std::string fields(size_t count,const std::vector<std::pair<size_t,int32_t>>& values){
    std::ostringstream out;out<<"{,";
    for(size_t n=0;n<count;++n){if(n)out<<",";
        int32_t v=0;for(const auto& e:values)if(e.first==n)v=e.second;out<<v;}
    return out.str();
}
std::string modelText(const HouseFixture& f,bool withTarget=true){
    std::ostringstream out;out<<"BUILDING MODS\n";
    for(int n=0;n<5;++n)out<<"BM"<<n<<fields(13,{})<<"\n";
    out<<"ALL BUILDINGS\n";
    for(int n=0;n<269;++n)out<<"B"<<n<<fields(13,{})<<"\n";
    out<<"HOUSE MODS\n";
    for(int n=0;n<5;++n)out<<"HM"<<n<<fields(24,{})<<"\n";
    out<<"ALL HOUSES\n";
    for(int n=0;n<15;++n){
        std::vector<std::pair<size_t,int32_t>> v;
        if(n==0)v={{0,f.down},{1,f.up},{3,f.currentWater},{10,f.currentGoods}};
        if(n==1)v={{3,f.targetWater},{10,f.targetGoods}};
        out<<"H"<<n<<fields(24,v)<<"\n";
    }
    (void)withTarget;return out.str();
}
NativeHouseModels parseModels(const HouseFixture& f){
    const auto text=modelText(f);
    const auto building=originalBuildingModels(text,2);
    const auto house=originalBuildingHouseModels(text,2);
    need(building&&building->size()==269,"authored building rows parse");
    need(house&&house->size()==15,"authored house rows parse");
    NativeHouseModels out;out.sourceRevision=7;out.modelRevision=42;out.difficulty=2;
    out.building=*building;out.house=*house;return out;
}
struct Env {NativeSessionSource source;NativeSessionState state;};
constexpr size_t cells=OriginalTerrainState::cells;
Env makeEnv(uint32_t rules=1,uint32_t residents=10){
    const uint32_t side=24;
    NativeSessionSeed seed;seed.source="Fixture.City.SAV";seed.sourceRevision=7;seed.modelRevision=42;
    seed.side=side;seed.roadCost=1;seed.houseCost=2;seed.houseCapacity=100;
    seed.wellCost=3;seed.hunterCost=4;seed.initialFunds=1000;
    seed.buildable.assign(size_t(side)*side,1);seed.sourceRoad.assign(size_t(side)*side,0);
    seed.laborPercent=60;seed.hunterWorkers=1;seed.wellWorkers=1;seed.laborPriority.fill(1);
    NativeSessionState state;state.rulesVersion=rules;state.nextId=3;state.ticks=100000;state.revision=100000;
    NativeSessionObject road;road.id=1;road.kind=NativeObjectKind::Road;road.x=7;road.y=5;
    NativeSessionObject house;house.id=2;house.kind=NativeObjectKind::House;house.x=5;house.y=5;house.residents=residents;
    state.objects={road,house};state.funds=1000-1-2;
    const uint64_t population=residents;
    if(rules>=3){state.foodProduced=population;state.foodConsumed=population;state.foodStock=0;
        state.lastFood.demand=uint32_t(population);state.lastFood.fed=uint32_t(population);}
    if(rules>=5){state.taxCycles=state.ticks/4;state.taxReceived=0;state.lastTax.eligible=0;state.lastTax.income=0;}
    OriginalMapTerrain terrain;terrain.activeSide=side;terrain.flags.assign(cells,0);
    OriginalSavedGround ground;
    ground.state.terrain.assign(cells,0);ground.state.feature.assign(cells,0);
    ground.state.subTile.assign(cells,0);ground.state.randomByte.assign(cells,0);
    ground.material.assign(cells,0);ground.owner.assign(cells,0);
    ground.state.roadDistance.assign(cells,0);
    ground.activeSide=side;ground.cropStorageBase=((228-side)/2)*229;ground.savedView=0;ground.waterMode=0;
    OriginalSaveResult save;save.snapshotRevision=7;save.status=OriginalSaveStatus::LoadedSnapshot;
    save.terrain=std::move(terrain);save.layers.ground=std::move(ground);
    return Env{NativeSessionSource{std::move(seed),std::move(save)},std::move(state)};
}
const NativeHouseEvolutionView& onlyHouse(const NativeHouseEvolutionReport& report){
    need(report.error.empty(),"query error "+report.error);
    need(report.houses.size()==1,"one authored house");
    return report.houses.front();
}
bool has(const std::vector<std::string>& values,const std::string& name){
    for(const auto& v:values)if(v==name)return true;return false;
}
size_t waterBinaryCases=0,branchStepCases=0,planEquivalenceCases=0;
// Independent 519120/51A3F0 probe on one authored house at the same site.
struct BranchProbe {std::optional<bool> current,next;int direction=0;bool contextValid=true;};
BranchProbe probeBranch(const NativeHouseModels& models,int desirability,uint8_t water){
    const uint32_t side=24,cropBase=((228-side)/2)*229;
    OriginalBuildingUpdateContext c(OriginalBuildingUpdateScope::AdmittedTicks,true);
    c.buildingModels=models.building;c.houseModels=models.house;
    c.terrain=std::vector<uint32_t>(cells,0);
    c.terrainOwners=std::vector<uint16_t>(cells,0);
    c.activeSide=side;c.cropBase=cropBase;
    c.evolutionDisabled=false;c.forceHouseEvolution=false;c.monthPart=1;
    c.buildings.emplace();c.buildingInputs.emplace();
    OriginalBuildingFootprint f;f.slot=2;f.type=3;f.x=5;f.y=5;f.size=2;
    f.imageInputs.emplace();f.imageInputs->gridIndex=cropBase+5+228*5;
    c.terrainOwners->at(f.imageInputs->gridIndex)=2;
    OriginalBuildingUpdateInputs i;i.alive=1;i.identity=2;i.house.emplace();i.house->fill(0);
    i.model=0;i.residents=10;i.modelDesirability=desirability;i.houseWater=water;
    (*c.buildings)[2]=f;(*c.buildingInputs)[2]=i;
    OriginalBuildingUpdateState requirements{f,i,std::nullopt};
    BranchProbe probe;
    probe.current=originalBuildingHouseRequirements(requirements,c,false);
    probe.next=originalBuildingHouseRequirements(requirements,c,true);
    OriginalBuildingUpdateState evolving{f,i,std::nullopt};
    const auto blocker=originalBuildingHouseEvolutionStep(evolving,c);
    probe.contextValid=blocker!=OriginalBuildingUpdateBlocker::InvalidState&&
                        blocker!=OriginalBuildingUpdateBlocker::MissingContext;
    probe.direction=c.houseEvolutionResult;
    return probe;
}
void requireBranchesMatchProbe(const NativeHouseModels& models,const NativeHouseEvolutionView& v){
    const auto checkBonus=[&](int desirability){
        const auto dryProbe=probeBranch(models,desirability,0);
        const auto wetProbe=probeBranch(models,desirability,2);
        need(dryProbe.contextValid&&wetProbe.contextValid,"strict probe context");
        if(v.directionIfNoWater)need(dryProbe.direction==*v.directionIfNoWater,"dry branch equals 519120 function");
        if(v.directionIfWater)need(wetProbe.direction==*v.directionIfWater,"watered branch equals 519120 function");
    };
    if(v.desirability)checkBonus(*v.desirability);
    if(v.desirabilityWithBonus)checkBonus(*v.desirabilityWithBonus);
}
struct StrictPlanFixture {
    OriginalBuildingUpdateState state;
    OriginalBuildingUpdateContext context{OriginalBuildingUpdateScope::AdmittedTicks,true};
};
StrictPlanFixture planFixture(const NativeHouseModels& models,uint16_t type=3,
                              int desirability=0,uint8_t delay=0,uint32_t part=1) {
    StrictPlanFixture f;
    constexpr unsigned side=24,cropBase=((228-side)/2)*229;
    auto& c=f.context;c.buildingModels=models.building;c.houseModels=models.house;
    c.terrainOwners=std::vector<uint16_t>(cells,0);
    c.activeSide=side;c.cropBase=cropBase;
    c.evolutionDisabled=false;c.forceHouseEvolution=false;c.monthPart=part;
    auto& b=f.state.building;b.slot=2;b.type=type;b.x=5;b.y=5;b.size=2;
    b.imageInputs.emplace();b.imageInputs->gridIndex=cropBase+5+228*5;
    c.terrainOwners->at(b.imageInputs->gridIndex)=uint16_t(b.slot);
    auto& i=f.state.inputs;i.alive=1;i.identity=2;i.house.emplace();i.house->fill(0);
    i.model=int16_t(std::max(0,int(type)-3));i.residents=10;i.modelDesirability=desirability;
    i.house->at(52)=delay;
    return f;
}
void connectPlanPool(StrictPlanFixture& f){
    auto& c=f.context;const size_t slot=f.state.building.slot;
    c.buildings.emplace();c.buildingInputs.emplace();c.buildingLinks.emplace();
    c.buildings->emplace(slot,f.state.building);c.buildingInputs->emplace(slot,f.state.inputs);
    OriginalFigureBuildingLink link;link.type=f.state.building.type;link.alive=f.state.inputs.alive;
    link.x=int16_t(f.state.building.x);link.y=int16_t(f.state.building.y);
    link.cell=f.state.building.imageInputs->gridIndex;
    c.buildingLinks->emplace(slot,link);
}
void testPlanContextPreservesPools(){
    auto models=parseModels({-99,1,0,0,0,0});models.house.at(0)[10]=2;
    auto f=planFixture(models,2,10,0,7);f.state.building.imageInputs->randomByte=0;
    f.state.inputs.house->at(0x16-1-4)=8;connectPlanPool(f);
    f.context.phase=39;f.context.waterScratch=73;f.context.houseEvolutionCalls=7;
    f.context.houseConversions=3;f.context.houseRedraws=2;f.context.buildingRedraws.insert(99);
    const auto plan=planNativeHouseEvolution(f.state,f.context,0);
    need(plan.candidate&&plan.candidateContext&&plan.error.empty(),"context candidate accompanies strict success");
    const auto& c=*plan.candidateContext;
    need(plan.retainsHouseIdentityAndGeometry&&plan.invariantError.empty(),"ordinary conversion retains identity and geometry");
    need(plan.candidate->building.type==3&&c.buildings->at(2).type==3&&
         c.buildingInputs->at(2).model==0&&c.buildingLinks->at(2).type==3,
         "candidate conversion reaches local pool and figure mirror");
    need(detail::i16(c.buildingInputs->at(2).house->data()+0x16-1-4)==6&&
         c.buildingInputs->at(2).houseWater==0,"pool keeps candidate goods and cached water");
    need(c.houseEvolutionCalls==8&&c.houseConversions==4&&c.houseRedraws==3&&
         c.buildingRedraws.count(2)&&c.buildingRedraws.count(99)&&
         c.phase&&*c.phase==39&&c.waterScratch==73,"full context retains strict counters redraws and unrelated state");
    need(f.state.building.type==2&&f.context.buildings->at(2).type==2&&
         detail::i16(f.context.buildingInputs->at(2).house->data()+0x16-1-4)==8&&
         f.context.houseEvolutionCalls==7&&f.context.houseConversions==3&&
         !f.context.buildingRedraws.count(2),"input state and context remain unchanged");
}
void testPlanOwnershipNeedsResolution(){
    const auto models=parseModels({-99,100,0,0,0,0});auto f=planFixture(models);
    connectPlanPool(f);const auto oldCell=f.state.building.imageInputs->gridIndex;
    f.context.terrainOwners->at(oldCell)=0;f.context.terrainOwners->at(oldCell+1)=2;
    const auto relocated=planNativeHouseEvolution(f.state,f.context,0);
    need(relocated.candidate&&relocated.candidateContext&&relocated.error.empty(),"strict relocation result retained for explicit caller policy");
    need(!relocated.retainsHouseIdentityAndGeometry&&
         relocated.invariantError=="house_candidate_geometry_changed"&&
         relocated.candidate->building.x==6&&relocated.candidate->inputs.alive==1&&
         relocated.candidateContext->ownershipRepairs[3]==1&&
         relocated.candidateContext->buildingLinks->at(2).x==6&&
         relocated.candidateContext->buildingLinks->at(2).cell==oldCell+1,
         "relocation is flagged and complete context mirrors repaired geometry");
    need(f.state.building.x==5&&f.context.buildingLinks->at(2).x==5&&
         f.context.ownershipRepairs[3]==0,"relocation cannot leak into source");
    f=planFixture(models);connectPlanPool(f);std::fill(f.context.terrainOwners->begin(),f.context.terrainOwners->end(),0);
    const auto retired=planNativeHouseEvolution(f.state,f.context,0);
    need(retired.candidate&&retired.candidateContext&&retired.error.empty()&&
         retired.candidate->inputs.alive==4&&retired.candidateContext->buildingInputs->at(2).alive==4&&
         retired.candidateContext->buildingLinks->at(2).alive==4&&
         retired.candidateContext->ownershipRepairs[2]==1,
         "strict ownership loss preserves retirement candidate and context");
    need(!retired.retainsHouseIdentityAndGeometry&&retired.invariantError=="house_candidate_inactive"&&
         f.state.inputs.alive==1&&f.context.buildingInputs->at(2).alive==1,
         "retired result is not an ordinary publishable house");
}
void testPlanContextFailureAtomic(){
    auto f=planFixture(parseModels({-99,-99,0,0,0,0}),3,10,9);connectPlanPool(f);
    const auto refused=planNativeHouseEvolution(f.state,f.context,2);
    need(!refused.candidate&&!refused.candidateContext&&!refused.retainsHouseIdentityAndGeometry&&
         refused.error=="house_evolution_target_unsupported"&&
         f.context.buildingInputs->at(2).house->at(52)==9&&
         f.context.houseConversions==0&&f.context.buildingRedraws.empty(),
         "strict target refusal exposes neither local nor context prefix");
    f=planFixture(parseModels({-99,100,0,0,0,0}));connectPlanPool(f);
    f.context.buildings->erase(2);
    const auto missing=planNativeHouseEvolution(f.state,f.context,0);
    need(!missing.candidate&&!missing.candidateContext&&
         missing.blocker==OriginalBuildingUpdateBlocker::InvalidState&&
         missing.error=="house_candidate_pool_unconnected"&&f.context.buildingInputs->count(2),
         "incomplete supplied pool refuses atomically");
    f=planFixture(parseModels({-99,100,0,0,0,0}),3,-120,8);connectPlanPool(f);
    f.context.buildingLinks->clear();
    const auto unsynced=planNativeHouseEvolution(f.state,f.context,0);
    need(!unsynced.candidate&&!unsynced.candidateContext&&
         unsynced.error=="house_candidate_pool_sync_failed"&&
         f.state.inputs.house->at(52)==8&&f.context.buildingInputs->at(2).house->at(52)==8,
         "mirror sync failure cannot leak candidate delay writes");
}
void testPlanMatchesStrictStep(){
    const auto models=parseModels({-99,100,0,0,0,0});
    for(uint16_t type:{2,3,5,7})for(int desirability:{-120,-10,0,10,120})
        for(uint8_t water:{0,1,2})for(uint8_t delay:{0,8,9})for(uint32_t part:{1,7}){
            auto f=planFixture(models,type,desirability,delay,part);
            const auto originalTail=*f.state.inputs.house;
            const auto originalImage=*f.state.building.imageInputs;
            const auto plan=planNativeHouseEvolution(f.state,f.context,water);
            auto direct=f.state;auto context=f.context;direct.inputs.houseWater=water;
            const auto blocker=originalBuildingHouseEvolutionStep(direct,context);
            need(plan.blocker==blocker,"plan uses strict blocker");
            need(plan.direction&&*plan.direction==context.houseEvolutionResult,"plan uses strict intent");
            need(bool(plan.candidate)==(blocker==OriginalBuildingUpdateBlocker::None),"candidate iff strict success");
            need(bool(plan.candidateContext)==bool(plan.candidate),"state and context candidates are paired");
            if(plan.candidate){
                const auto& candidate=*plan.candidate;
                need(candidate.building.type==direct.building.type&&candidate.inputs.model==direct.inputs.model&&
                     candidate.inputs.house==direct.inputs.house&&candidate.inputs.houseWater==direct.inputs.houseWater&&
                     candidate.inputs.alive==direct.inputs.alive&&
                     candidate.building.imageInputs->word20==direct.building.imageInputs->word20&&
                     candidate.building.imageInputs->houseByte3c==direct.building.imageInputs->houseByte3c,
                     "candidate exact strict state");
                need(plan.error.empty(),"accepted candidate has no error");
            }else need(!plan.error.empty(),"refused candidate has reason");
            need(*f.state.inputs.house==originalTail&&f.state.building.type==type&&
                 f.state.building.imageInputs->word20==originalImage.word20,"source unchanged");
            need(f.context.houseEvolutionCalls==0&&f.context.houseConversions==0&&
                 f.context.buildingRedraws.empty(),"context unchanged");
            ++planEquivalenceCases;
        }
}
void testPlanUnknownInputs(){
    const auto models=parseModels({-99,100,0,0,0,0});auto f=planFixture(models);
    auto plan=planNativeHouseEvolution(f.state,f.context);
    need(!plan.candidate&&!plan.direction&&plan.error=="house_water_unconnected","unknown water refused");
    plan=planNativeHouseEvolution(f.state,f.context,3);
    need(!plan.candidate&&plan.error=="invalid_house_water","invalid water refused");
    f.context.monthPart.reset();plan=planNativeHouseEvolution(f.state,f.context,0);
    need(!plan.candidate&&plan.error=="house_cycle_unmapped","unknown clock refused");
    f=planFixture(models);f.state.inputs.house.reset();plan=planNativeHouseEvolution(f.state,f.context,0);
    need(!plan.candidate&&plan.error=="house_runtime_unconnected","missing runtime refused");
    f=planFixture(models);f.state.building.type=4;plan=planNativeHouseEvolution(f.state,f.context,2);
    need(!plan.candidate&&!plan.direction&&plan.error=="house_type_unsupported","type4 input refused");
    f=planFixture(models);f.context.monthPart=16;plan=planNativeHouseEvolution(f.state,f.context,0);
    need(!plan.candidate&&plan.error=="invalid_house_month_part","monthly tail refused");
}
void testUnsupportedCandidateDoesNotLeak(){
    const auto models=parseModels({-99,-99,0,0,0,0});auto f=planFixture(models,3,10,9);
    const auto before=*f.state.inputs.house;
    const auto plan=planNativeHouseEvolution(f.state,f.context,2);
    need(plan.direction&&*plan.direction==1,"unsupported target retains intent");
    need(!plan.candidate&&plan.blocker==OriginalBuildingUpdateBlocker::HousingEvolution&&
         plan.error=="house_evolution_target_unsupported","type4 target refused");
    need(*f.state.inputs.house==before&&f.state.inputs.house->at(52)==9,"failed delay reset never leaks");
    const auto env=makeEnv();const auto report=queryNativeHouseEvolution(env.source,env.state,models);
    const auto& v=onlyHouse(report);
    need(v.direction&&*v.direction==1,"query preserves intent");
    need(v.candidateAcceptedIfWater&&!*v.candidateAcceptedIfWater&&
         v.candidateAcceptedIfNoWater&&!*v.candidateAcceptedIfNoWater,"query labels refusal");
    need(!v.candidateTypeIfWater&&!v.candidateTypeIfNoWater&&!v.candidatePublishable,"no publishable target");
    need(has(v.candidateMissing,"house_evolution_target_unsupported")&&
         has(v.candidateMissing,"house_runtime_state_unconnected"),"candidate blockers explicit");
}
void testPlanConsumesOnlyCandidateGoods(){
    auto models=parseModels({-99,100,0,0,0,0});models.house.at(0)[10]=2;
    auto f=planFixture(models,3,0,0,7);
    f.state.inputs.house->at(0x16-1-4)=8;
    f.state.inputs.house->at(55)=1;
    const auto plan=planNativeHouseEvolution(f.state,f.context,0);
    need(plan.candidate&&plan.direction&&*plan.direction==0,"goods candidate accepted");
    need(detail::i16(plan.candidate->inputs.house->data()+0x16-1-4)==6,
         "part7 candidate consumes model amount");
    need(detail::i16(f.state.inputs.house->data()+0x16-1-4)==8,"query cannot consume source stock");
    need(plan.candidate->building.imageInputs->word20==10&&
         plan.candidate->building.imageInputs->houseByte3c==1,"candidate image fields are strict outputs");
    f.context.monthPart=1;
    const auto other=planNativeHouseEvolution(f.state,f.context,0);
    need(other.candidate&&detail::i16(other.candidate->inputs.house->data()+0x16-1-4)==8,
         "part1 candidate does not consume stock");
}
void testPlanDeferredCounter(){
    const auto models=parseModels({-99,100,0,0,0,0});auto f=planFixture(models,3,-120,8);
    const auto first=planNativeHouseEvolution(f.state,f.context,0);
    need(first.candidate&&first.direction&&*first.direction==-1&&
         first.candidate->inputs.house->at(52)==9&&first.candidate->building.type==3,
         "delay8 becomes9 without changing type");
    need(f.state.inputs.house->at(52)==8,"delay does not change source");
    const auto second=planNativeHouseEvolution(*first.candidate,f.context,0);
    need(second.candidate&&second.candidate->inputs.house->at(52)==0&&
         second.candidate->building.type==3,"strict lower-bound type3 resets saturated delay");
}
void testPlanUsesActualModel(){
    auto models=parseModels({-99,-99,0,0,0,0});
    models.house.at(2)[0]=-99;models.house.at(2)[1]=100;
    auto f=planFixture(models,5,10);
    const auto plan=planNativeHouseEvolution(f.state,f.context,0);
    need(plan.candidate&&plan.direction&&*plan.direction==0&&
         plan.candidate->building.type==5&&plan.candidate->inputs.model==2,"actual model row used");
}
void testStrictDirection(){
    const auto env=makeEnv();const auto models=parseModels({-99,100,0,0,0,0});
    const auto report=queryNativeHouseEvolution(env.source,env.state,models);
    const auto& v=onlyHouse(report);
    need(v.desirability&&*v.desirability==0,"flat authored grid desirability0");
    need(v.desirabilityWithBonus&&*v.desirabilityWithBonus==10,"bonus adds10");
    need(v.currentThresholdDown&&*v.currentThresholdDown==-99,"current down threshold");
    need(v.currentThresholdUp&&*v.currentThresholdUp==100,"current up threshold");
    need(v.currentMet&&*v.currentMet,"current requirements met");
    need(v.nextMet&&*v.nextMet,"target requirements met");
    need(v.direction&&*v.direction==0,"strict direction unchanged");
    need(v.directionIfWater&&*v.directionIfWater==0,"watered branch strict");
    need(v.directionIfNoWater&&*v.directionIfNoWater==0,"dry branch strict");
    need(v.nativeWatered&&!*v.nativeWatered,"native coverage explicit and separate");
    need(v.missing.empty(),"no missing inputs when direction is invariant");
    need(has(v.notNeeded,"house_water_phase28"),"water not needed for direction");
    const auto text=nativeHouseEvolutionText(v);
    need(text.summary.find("不变")!=std::string::npos,"summary shows strict direction");
    need(text.detail.find("不改变城市")!=std::string::npos,"detail keeps read-only label");
    need(text.detail.find("若有水：不变化")!=std::string::npos,"watered branch displayed");
    need(text.detail.find("若无水：不变化")!=std::string::npos,"dry branch displayed");
    need(text.detail.find("原版供水两种结果相同")!=std::string::npos,"equal branches explained");
    need(text.detail.find("原生供水覆盖：无（自写规则，非原版）")!=std::string::npos,"native coverage labelled non-original");
    need(text.summary.find("house_")==std::string::npos,"summary has no internal keys");
    need(text.detail.find("house_")==std::string::npos,"detail has no internal keys");
}
void testTargetWaterMissing(){
    const auto env=makeEnv();const auto models=parseModels({-99,-99,0,0,1,0});
    const auto report=queryNativeHouseEvolution(env.source,env.state,models);
    const auto& v=onlyHouse(report);
    need(v.direction==std::nullopt,"water-dependent direction is not strict");
    need(v.directionIfWater&&*v.directionIfWater==1,"watered branch computed strictly");
    need(v.directionIfNoWater&&*v.directionIfNoWater==0,"dry branch computed strictly");
    need(v.currentMetIfWater&&*v.currentMetIfWater,"current requirement met with water");
    need(v.currentMetIfNoWater&&*v.currentMetIfNoWater,"current requirement met without water");
    need(v.nextMetIfWater&&*v.nextMetIfWater,"target requirement met with water");
    need(v.nextMetIfNoWater&&!*v.nextMetIfNoWater,"target requirement unmet without water");
    need(v.nativeWatered&&!*v.nativeWatered,"native coverage stays separate");
    need(has(v.missing,"house_water_phase28"),"missing water named");
    const auto text=nativeHouseEvolutionText(v);
    need(text.detail.find("原版房屋供水状态未接通")!=std::string::npos,"UI names missing water");
    need(text.detail.find("若有水：上升")!=std::string::npos,"watered direction branch displayed");
    need(text.detail.find("若无水：不变化")!=std::string::npos,"dry direction branch displayed");
    need(text.detail.find("差别仅在原版供水")!=std::string::npos,"difference attributed to original water");
    need(text.detail.find("原生供水覆盖：无（自写规则，非原版）")!=std::string::npos,"native coverage explicitly non-original");
}
void testBonusMissing(){
    const auto env=makeEnv();const auto models=parseModels({-99,5,0,0,0,0});
    const auto report=queryNativeHouseEvolution(env.source,env.state,models);
    const auto& v=onlyHouse(report);
    need(v.direction==std::nullopt,"bonus-dependent direction is not strict");
    need(has(v.missing,"house_desirability_bonus_60"),"missing runtime bonus named");
}
void testRuntimeColumnMissing(){
    const auto env=makeEnv();const auto models=parseModels({-99,100,0,5,0,0});
    const auto report=queryNativeHouseEvolution(env.source,env.state,models);
    const auto& v=onlyHouse(report);
    need(v.direction==std::nullopt,"runtime goods demand blocks direction");
    need(has(v.missing,"house_runtime_col10"),"runtime goods column named");
    need(!v.currentMet,"current requirement is not presumed met");
}
void testEmptyEarlyReturn(){
    const auto env=makeEnv(1,0);const auto models=parseModels({-99,100,0,0,0,0});
    const auto report=queryNativeHouseEvolution(env.source,env.state,models);
    const auto& v=onlyHouse(report);
    need(v.emptyEarlyReturn,"empty house early return");
    need(!v.direction,"empty house has no direction");
    const auto text=nativeHouseEvolutionText(v);
    need(text.summary.find("空住宅")!=std::string::npos,"empty summary");
    need(text.detail.find("提前返回")!=std::string::npos,"early return explained");
}
void testWaterIsBinary(){
    const std::array<HouseFixture,6> fixtures{{
        {-99,100,0,0,0,0},{-99,-99,0,0,1,0},{-99,5,0,0,0,0},
        {-99,100,1,0,0,0},{-99,100,0,5,0,0},{-99,100,0,0,3,0}}};
    for(const auto& fixture:fixtures){
        const auto models=parseModels(fixture);
        for(int desirability:{-10,0,5,20,120}){
            const auto one=probeBranch(models,desirability,1);
            const auto two=probeBranch(models,desirability,2);
            need(one.contextValid&&two.contextValid,"binary probe context");
            need(one.current==two.current&&one.next==two.next&&one.direction==two.direction,
                 "houseWater1 and2 give identical requirements and direction");
            ++waterBinaryCases;
        }
    }
}
void testBranchesMatchEvolutionStep(){
    const std::array<HouseFixture,3> fixtures{{{-99,-99,0,0,1,0},{-99,100,0,0,0,0},{-99,-99,0,0,0,0}}};
    for(size_t index=0;index<fixtures.size();++index){
        const auto& fixture=fixtures[index];
        const auto env=makeEnv();const auto models=parseModels(fixture);
        const auto report=queryNativeHouseEvolution(env.source,env.state,models);
        const auto& v=onlyHouse(report);
        need(v.desirability.has_value(),"branch fixture desirability "+std::to_string(index));
        const auto dry=probeBranch(models,*v.desirability,0);
        const auto wet=probeBranch(models,*v.desirability,2);
        need(dry.contextValid&&wet.contextValid,"branch probe context");
        if(v.directionIfNoWater){need(dry.direction==*v.directionIfNoWater,"dry branch equals 519120 step");++branchStepCases;}
        if(v.directionIfWater){need(wet.direction==*v.directionIfWater,"watered branch equals 519120 step");++branchStepCases;}
    }
    need(branchStepCases>=4,"both branches checked against evolution step");
}
Env withWellAt(Env env,unsigned wx,unsigned wy){
    NativeSessionObject well;well.id=3;well.kind=NativeObjectKind::Well;well.x=wx;well.y=wy;
    env.state.objects.push_back(well);env.state.nextId=4;env.state.funds-=3;
    return env;
}
std::optional<int> authoredDesirabilityAt(const NativeSessionSource& src,const NativeSessionState& st,
                                          const NativeHouseModels& models,unsigned x,unsigned y){
    const auto scene=nativeSessionScene(src,st);if(!scene)return std::nullopt;
    const auto& g=scene->ground;
    OriginalBuildingUpdateContext c(OriginalBuildingUpdateScope::AdmittedTicks,true);
    c.buildingModels=models.building;c.houseModels=models.house;
    c.terrain=g.state.terrain;c.terrainOwners=g.owner;
    c.activeSide=g.activeSide;c.cropBase=g.cropStorageBase;
    c.resourceGrowth.emplace();c.resourceGrowth->surface=g.state;
    c.evolutionDisabled=false;c.forceHouseEvolution=false;c.monthPart=1;
    c.buildings.emplace();c.buildingInputs.emplace();
    for(const auto& f:scene->buildings.footprints){
        (*c.buildings)[f.slot]=f;OriginalBuildingUpdateInputs i;i.alive=1;i.identity=uint32_t(f.slot);
        if(originalBuildingIsHouse(f.type)){i.house.emplace();i.house->fill(0);i.model=int16_t(std::max(0,int(f.type)-3));i.residents=10;}
        (*c.buildingInputs)[f.slot]=std::move(i);
    }
    if(originalBuildingDesirabilityStep(c)!=OriginalBuildingUpdateBlocker::None||!c.desirability)return std::nullopt;
    const size_t at=size_t(*c.cropBase)+228*size_t(y)+x;
    if(at>=c.desirability->size())return std::nullopt;
    return int(c.desirability->at(at));
}
void testNativeCoverageZeroInfluence(){
    // Group A: injected well model has no desirability influence. Only the
    // self-written native coverage row may change; both bonus levels stay fixed.
    const auto models=parseModels({-99,-99,0,0,1,0});
    const auto dryEnv=makeEnv(2),wetEnv=withWellAt(makeEnv(2),8,5);
    const auto dryReport=queryNativeHouseEvolution(dryEnv.source,dryEnv.state,models);
    const auto wetReport=queryNativeHouseEvolution(wetEnv.source,wetEnv.state,models);
    const auto& dry=onlyHouse(dryReport);
    const auto& wet=onlyHouse(wetReport);
    need(dry.nativeWatered&&!*dry.nativeWatered,"dry city has no native coverage");
    need(wet.nativeWatered&&*wet.nativeWatered,"well and road provide native coverage");
    need(dry.desirability==wet.desirability&&dry.desirabilityWithBonus==wet.desirabilityWithBonus,
         "zero-influence well leaves both bonus desirabilities unchanged");
    need(dry.directionIfWater==wet.directionIfWater&&dry.directionIfNoWater==wet.directionIfNoWater&&
         dry.currentMetIfWater==wet.currentMetIfWater&&dry.currentMetIfNoWater==wet.currentMetIfNoWater&&
         dry.nextMetIfWater==wet.nextMetIfWater&&dry.nextMetIfNoWater==wet.nextMetIfNoWater&&
         dry.missing==wet.missing&&dry.notNeeded==wet.notNeeded,
         "with both bonus desirabilities unchanged only native coverage differs");
    const auto dryText=nativeHouseEvolutionText(dry),wetText=nativeHouseEvolutionText(wet);
    need(dryText.detail.find("原生供水覆盖：无（自写规则，非原版）")!=std::string::npos,"dry native line");
    need(wetText.detail.find("原生供水覆盖：有（自写规则，非原版）")!=std::string::npos,"wet native line");
    const auto stripNative=[](std::string text){
        const auto at=text.find("\n原生供水覆盖：");
        if(at!=std::string::npos){const auto end=text.find('\n',at+1);text.erase(at,end==std::string::npos?std::string::npos:end-at);}
        return text;};
    need(stripNative(dryText.detail)==stripNative(wetText.detail),"only native coverage line changes");
}
void testWellDesirabilityPropagation(){
    // Group B: in-range nonzero well influence. Real desirability propagates,
    // and each strict branch must still equal the 519120 function at 0/2.
    auto models=parseModels({-99,3,0,0,0,0});
    models.building[72][1]=5;models.building[72][2]=1;models.building[72][3]=0;models.building[72][4]=4;
    const auto dryEnv=makeEnv(2),wetEnv=withWellAt(makeEnv(2),8,5);
    const auto dryReport=queryNativeHouseEvolution(dryEnv.source,dryEnv.state,models);
    const auto wetReport=queryNativeHouseEvolution(wetEnv.source,wetEnv.state,models);
    const auto& dry=onlyHouse(dryReport);
    const auto& wet=onlyHouse(wetReport);
    need(dry.nativeWatered&&!*dry.nativeWatered&&wet.nativeWatered&&*wet.nativeWatered,"native coverage flips");
    need(dry.desirability&&wet.desirability&&*wet.desirability==*dry.desirability+5,
         "in-range well influence reaches house desirability dry="+std::to_string(dry.desirability.value_or(-999))+
         " wet="+std::to_string(wet.desirability.value_or(-999)));
    need(dry.desirabilityWithBonus&&wet.desirabilityWithBonus&&
         *wet.desirabilityWithBonus==*dry.desirabilityWithBonus+5,
         "in-range well influence reaches both bonus desirabilities");
    requireBranchesMatchProbe(models,dry);
    requireBranchesMatchProbe(models,wet);
    need(!dry.direction&&has(dry.missing,"house_desirability_bonus_60"),
         "dry well resolves no strict direction until bonus is known");
    need(wet.direction&&*wet.direction==1,"in-range well influence resolves bonus ambiguity");
    const auto dryText=nativeHouseEvolutionText(dry),wetText=nativeHouseEvolutionText(wet);
    need(dryText.detail.find("原生供水覆盖：无（自写规则，非原版）")!=std::string::npos,"dry native line");
    need(wetText.detail.find("原生供水覆盖：有（自写规则，非原版）")!=std::string::npos,"wet native line");
}
void testEdgeClippedInfluence(){
    // Same in-range model at x=1 triggers the strict clipped branch: every
    // ring writes the well's own origin and never reaches the house.
    auto models=parseModels({-99,3,0,0,0,0});
    models.building[72][1]=5;models.building[72][2]=1;models.building[72][3]=0;models.building[72][4]=4;
    const auto dryEnv=makeEnv(2),edgeEnv=withWellAt(makeEnv(2),1,5);
    const auto dryReport=queryNativeHouseEvolution(dryEnv.source,dryEnv.state,models);
    const auto edgeReport=queryNativeHouseEvolution(edgeEnv.source,edgeEnv.state,models);
    const auto& dry=onlyHouse(dryReport);
    const auto& edge=onlyHouse(edgeReport);
    need(edge.desirability==dry.desirability&&edge.desirabilityWithBonus==dry.desirabilityWithBonus,
         "clipped well influence does not reach the house");
    const auto dryOrigin=authoredDesirabilityAt(dryEnv.source,dryEnv.state,models,1,5);
    const auto edgeOrigin=authoredDesirabilityAt(edgeEnv.source,edgeEnv.state,models,1,5);
    need(dryOrigin&&*dryOrigin==0,"no well leaves origin cell unchanged");
    need(edgeOrigin&&*edgeOrigin!=0,"clipped influence repeatedly writes the well origin");
}
void testTargetRowMissing(){
    auto env=makeEnv();auto models=parseModels({-99,100,0,0,0,0});
    models.house.erase(1);
    const auto report=queryNativeHouseEvolution(env.source,env.state,models);
    const auto& v=onlyHouse(report);
    need(!v.direction,"missing target row cannot yield direction");
    need(has(v.missing,"house_model_row1"),"missing target row named");
}
void testRevisionValidation(){
    auto env=makeEnv();auto models=parseModels({-99,100,0,0,0,0});
    models.sourceRevision=99;need(queryNativeHouseEvolution(env.source,env.state,models).error=="source_changed","source revision guard");
    models=parseModels({-99,100,0,0,0,0});models.modelRevision=99;
    need(queryNativeHouseEvolution(env.source,env.state,models).error=="model_changed","model revision guard");
    models=parseModels({-99,100,0,0,0,0});models.house.clear();
    need(queryNativeHouseEvolution(env.source,env.state,models).error=="models_unparsed","empty model guard");
}
void testQueryIsReadOnly(){
    const auto models=parseModels({-99,100,0,0,0,0});
    for(uint32_t rules=1;rules<=6;++rules){
        auto env=makeEnv(rules);
        const auto before=encodeNativeSession(env.source.seed,env.state);
        const auto report=queryNativeHouseEvolution(env.source,env.state,models);
        const auto& v=onlyHouse(report);
        const auto after=encodeNativeSession(env.source.seed,env.state);
        need(before==after,"v"+std::to_string(rules)+" session bytes unchanged");
        need(v.direction&&*v.direction==0,"v"+std::to_string(rules)+" direction computed");
        need(v.nativeWatered&&!*v.nativeWatered,"v"+std::to_string(rules)+" native coverage explicit");
        const auto first=nativeHouseEvolutionText(v);
        const auto repeat=queryNativeHouseEvolution(env.source,env.state,models);
        const auto& again=onlyHouse(repeat);
        const auto second=nativeHouseEvolutionText(again);
        need(first.summary==second.summary&&first.detail==second.detail,"repeat query identical");
        need(env.source.save.terrain.flags.size()==cells&&env.source.save.layers.ground->owner.size()==cells,
            "source scene unchanged shape");
    }
}
Env makeV7Env(uint16_t type=3,uint16_t model=0){
    auto env=makeEnv(6);
    env.state.rulesVersion=7;
    for(uint16_t row=0;row<15;++row)env.source.seed.houseCapacities[row]=100;
    env.state.houseClock=NativeHouseClock{39,1};
    for(auto& object:env.state.objects)if(object.kind==NativeObjectKind::House){
        NativeHouseRuntime runtime;runtime.type=type;runtime.model=model;
        runtime.servicesKnown=true;runtime.houseWater=0;
        runtime.imageWord20=int16_t(object.residents);runtime.imageHouseByte3c=1;
        runtime.remaining[55]=1;object.house=runtime;
    }
    return env;
}
NativeHouseRuntime& v7House(Env& env){
    for(auto& object:env.state.objects)if(object.kind==NativeObjectKind::House&&object.house)
        return *object.house;
    throw std::runtime_error("missing v7 fixture house");
}
void testV7IndependentWater(){
    const auto models=parseModels({-99,-99,0,0,1,0});
    auto dry=withWellAt(makeV7Env(),8,5);
    auto& dryState=v7House(dry);dryState.houseWater=0;
    dryState.remaining[45]=96;dryState.remaining[47]=96;
    const auto dryReport=queryNativeHouseEvolution(dry.source,dry.state,models);
    const auto& d=onlyHouse(dryReport);
    need(d.runtimeStateKnown&&d.actualWater&&*d.actualWater==0,"persisted dry water known");
    need(d.nativeWatered&&*d.nativeWatered,"fixture native radius is covered");
    need(d.nextMet&&!*d.nextMet&&d.direction&&*d.direction==0,
         "tail96 and native coverage do not replace cached dry water");
    need(d.actualPlan.candidate&&d.actualPlan.error.empty()&&
         d.actualPlan.candidate->inputs.houseWater==0,"actual dry plan admitted");
    need(!has(d.candidateMissing,"house_evolution_target_unsupported")&&
         has(d.scenarioMissing,"house_evolution_target_unsupported"),
         "counterfactual wet refusal is not an actual dry blocker");
    auto wet=makeV7Env();v7House(wet).houseWater=2;
    v7House(wet).remaining[45]=v7House(wet).remaining[47]=0;
    const auto wetReport=queryNativeHouseEvolution(wet.source,wet.state,models);
    const auto& w=onlyHouse(wetReport);
    need(w.actualWater&&*w.actualWater==2&&w.nativeWatered&&!*w.nativeWatered,"persisted wet value independent");
    need(w.nextMet&&*w.nextMet&&w.direction&&*w.direction==1,"cached wet value survives tail decay");
    need(!w.actualPlan.candidate&&w.actualPlan.error=="house_evolution_target_unsupported",
         "actual type4 conversion remains refused");
    need(!d.candidatePublishable&&!w.candidatePublishable,"plans never publish");
}
void testV7KnownTailStock(){
    auto env=makeV7Env();env.state.houseClock=NativeHouseClock{39,7};
    v7House(env).remaining[0x16-1-4]=8;
    const auto models=parseModels({-99,100,0,2,0,0});
    const auto before=encodeNativeSession(env.source.seed,env.state);
    need(!before.empty(),"v7 fixture encodes");
    const auto report=queryNativeHouseEvolution(env.source,env.state,models);
    const auto& v=onlyHouse(report);
    need(v.currentMet&&*v.currentMet&&v.direction&&*v.direction==0,"known tail supplies inventory requirement");
    need(v.actualMonthPart&&*v.actualMonthPart==7&&!v.scenarioClockAssumed,"actual persisted clock used");
    need(v.actualPlan.candidate&&
         detail::i16(v.actualPlan.candidate->inputs.house->data()+0x16-1-4)==6,
         "actual candidate consumes model stock at supplied part7");
    need(!has(v.missing,"house_runtime_col10"),"known runtime column not marked missing");
    need(encodeNativeSession(env.source.seed,env.state)==before&&
         v7House(env).remaining[0x16-1-4]==8,"query preserves v7 persisted bytes");
}
void testV7UnknownInputGates(){
    const auto models=parseModels({-99,100,0,0,0,0});
    auto env=makeV7Env();env.state.houseClock.reset();
    auto report=queryNativeHouseEvolution(env.source,env.state,models);
    const auto& clock=onlyHouse(report);
    need(!clock.actualPlan.candidate&&clock.actualPlan.error=="house_cycle_unmapped"&&
         !clock.actualMonthPart&&clock.scenarioClockAssumed,"unknown clock never borrowed from what-if");
    need(clock.direction&&*clock.direction==0&&!clock.candidatePublishable,"direction-only scenario retained");
    env=makeV7Env();v7House(env).houseWater.reset();
    report=queryNativeHouseEvolution(env.source,env.state,models);
    const auto& water=onlyHouse(report);
    need(!water.actualWater&&!water.actualPlan.candidate&&
         water.actualPlan.error=="house_water_unconnected","unknown water refused");
    env=makeV7Env();v7House(env).servicesKnown=false;v7House(env).remaining.fill(0);
    report=queryNativeHouseEvolution(env.source,env.state,models);
    const auto& tail=onlyHouse(report);
    need(!tail.runtimeStateKnown&&!tail.currentMet&&!tail.nextMet&&!tail.direction&&
         !tail.actualPlan.candidate&&tail.actualPlan.error=="house_runtime_state_unconnected",
         "unknown tail is not default zero runtime");
    need(has(tail.missing,"house_runtime_state_unconnected"),"unknown tail named in conditions");
}
void testV7ActualGradeModel(){
    auto env=makeV7Env(5,2);auto models=parseModels({-99,-99,0,0,0,0});
    models.house.at(2)[0]=-99;models.house.at(2)[1]=100;
    const auto report=queryNativeHouseEvolution(env.source,env.state,models);
    const auto& v=onlyHouse(report);
    need(v.currentThresholdUp&&*v.currentThresholdUp==100&&v.direction&&*v.direction==0,
         "query thresholds use persisted grade model");
    need(v.actualPlan.candidate&&v.actualPlan.candidate->building.type==5&&
         v.actualPlan.candidate->inputs.model==2,"query and candidate preserve same grade");
}
void testV7BonusCandidateAmbiguity(){
    const auto env=makeV7Env();const auto models=parseModels({-99,5,0,0,0,0});
    const auto report=queryNativeHouseEvolution(env.source,env.state,models);
    const auto& v=onlyHouse(report);
    need(!v.direction&&!v.actualPlan.candidate&&
         v.actualPlan.error=="house_desirability_bonus_unconnected",
         "unknown bonus cannot select different persistent candidate");
    need(has(v.candidateMissing,"house_desirability_bonus_unconnected")&&!v.candidatePublishable,
         "actual candidate ambiguity named");
}
void realData(){
    const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");
    if(!env||!*env){std::cout<<"{\"passed\":true,\"skipped\":true,\"reason\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return;}
    const auto root=ResourceRoot::fromAbsolute(env);
    if(!root.configured()){std::cout<<"{\"passed\":true,\"skipped\":true,\"reason\":\"root unavailable\"}\n";return;}
    const auto catalog=enumerateOriginalMapSources(root);
    size_t sources=0,rejected=0,loaded=0,same=0,different=0,blocked=0,coverageChanges=0,housesSeen=0;
    size_t wellGateSources=0,type33GateSources=0,strictChangesWithInfluence=0;
    std::vector<std::string> missingNames;
    for(const auto& entry:catalog.sources){
        if(entry.kind!=OriginalMapSourceKind::SaveFile)continue;++sources;
        std::string error;
        const auto source=loadNativeSessionSource(root,entry.relativePath,error);
        if(!source){++rejected;continue;}
        std::string missing;
        const auto models=loadNativeHouseModels(root,source->seed,missing);
        if(!models){++rejected;continue;}
        ++loaded;
        // Only a boolean gate: whether this source's difficulty can let the
        // well/type33 model influence house desirability. Values are not printed.
        const auto influences=[&](uint16_t type){
            const auto row=models->building.find(type);
            return row!=models->building.end()&&row->second[1]!=0&&row->second[4]>0;};
        const bool wellInfluence=influences(72),type33Influence=influences(33);
        wellGateSources+=wellInfluence;type33GateSources+=type33Influence;
        const auto& seed=source->seed;
        bool found=false;unsigned x=0,y=0;
        // Keep five-cell margins so the well's in-range influence is not clipped.
        for(unsigned yy=5;yy+10<seed.side&&!found;++yy)for(unsigned xx=5;xx+17<seed.side&&!found;++xx){
            bool ok=true;for(unsigned dy=0;dy<5&&ok;++dy)for(unsigned dx=0;dx<12;++dx)
                ok&=seed.buildable[size_t(yy+dy)*seed.side+xx+dx]!=0;
            if(ok){x=xx;y=yy;found=true;}}
        need(found,"real flat site for road house well hunter");
        auto state=startNativeSession(seed,6);
        const auto send=[&](NativeSessionState& s,NativeSessionAction action,int64_t px=0,int64_t py=0,
                            int64_t tx=0,int64_t ty=0,uint64_t id=0){
            const NativeSessionCommand command{action,s.revision,px,py,tx,ty,id};
            need(transactNativeSession(seed,s,command).committed,"real action committed");
        };
        send(state,NativeSessionAction::RoadPath,x,y+2,x+11,y+2);
        send(state,NativeSessionAction::House,x,y);send(state,NativeSessionAction::House,x+2,y);
        send(state,NativeSessionAction::Well,x+4,y);send(state,NativeSessionAction::Hunter,x+6,y);
        send(state,NativeSessionAction::Resume);
        for(int tick=0;tick<80;++tick)send(state,NativeSessionAction::Tick);
        send(state,NativeSessionAction::Pause);
        need(nativePopulation(state)>0,"real inhabited house");
        const auto wetBytes=encodeNativeSession(seed,state);
        const auto wetReport=queryNativeHouseEvolution(*source,state,*models);
        need(wetReport.error.empty(),"real wet query");
        need(encodeNativeSession(seed,state)==wetBytes,"wet query leaves city bytes unchanged");
        uint64_t wellId=0;for(const auto& object:state.objects)if(object.kind==NativeObjectKind::Well){wellId=object.id;break;}
        need(wellId,"real well built");
        send(state,NativeSessionAction::Demolish,0,0,0,0,wellId);
        const auto dryBytes=encodeNativeSession(seed,state);
        const auto dryReport=queryNativeHouseEvolution(*source,state,*models);
        need(dryReport.error.empty(),"real dry query");
        need(encodeNativeSession(seed,state)==dryBytes,"dry query leaves city bytes unchanged");
        for(const auto& dry:dryReport.houses){
            const auto* wet=wetReport.find(dry.id);need(wet,"same house in both real scenarios");
            const bool strictSame=dry.directionIfWater==wet->directionIfWater&&dry.directionIfNoWater==wet->directionIfNoWater&&
                 dry.currentMetIfWater==wet->currentMetIfWater&&dry.currentMetIfNoWater==wet->currentMetIfNoWater&&
                 dry.nextMetIfWater==wet->nextMetIfWater&&dry.nextMetIfNoWater==wet->nextMetIfNoWater&&
                 dry.missing==wet->missing&&dry.notNeeded==wet->notNeeded;
            if(!wellInfluence&&!type33Influence)need(strictSame,"without model influence only native coverage changes");
            else if(!strictSame)++strictChangesWithInfluence;
            requireBranchesMatchProbe(*models,dry);
            requireBranchesMatchProbe(*models,*wet);
            if(dry.nativeWatered&&wet->nativeWatered&&!*dry.nativeWatered&&*wet->nativeWatered){
                ++coverageChanges;
                const auto dryText=nativeHouseEvolutionText(dry),wetText=nativeHouseEvolutionText(*wet);
                need(dryText.detail.find("原生供水覆盖：无（自写规则，非原版）")!=std::string::npos&&
                     wetText.detail.find("原生供水覆盖：有（自写规则，非原版）")!=std::string::npos,
                     "real text separates native coverage");
            }
            const auto classify=[&](const NativeHouseEvolutionView& v){
                if(v.emptyEarlyReturn)return;
                ++housesSeen;
                for(const auto& name:v.missing)if(std::find(missingNames.begin(),missingNames.end(),name)==missingNames.end())
                    missingNames.push_back(name);
                if(!v.directionIfWater||!v.directionIfNoWater)++blocked;
                else if(*v.directionIfWater==*v.directionIfNoWater)++same;
                else ++different;
            };
            classify(dry);classify(*wet);
        }
    }
    need(loaded>0,"at least one loadable original SAV");
    need(housesSeen>0,"at least one real inhabited house evaluated");
    std::cout<<"{\"passed\":true,\"skipped\":false,\"save_sources\":"<<sources
        <<",\"rejected_sources\":"<<rejected<<",\"models_loaded\":"<<loaded
        <<",\"houses_evaluated\":"<<housesSeen<<",\"branches_same\":"<<same
        <<",\"branches_different\":"<<different<<",\"branches_blocked\":"<<blocked
        <<",\"native_coverage_changes\":"<<coverageChanges
        <<",\"well_influence_gate_sources\":"<<wellGateSources
        <<",\"type33_influence_gate_sources\":"<<type33GateSources
        <<",\"strict_changes_with_influence\":"<<strictChangesWithInfluence<<",\"missing_names\":[";
    for(size_t n=0;n<missingNames.size();++n){if(n)std::cout<<",";std::cout<<'"'<<missingNames[n]<<'"';}
    std::cout<<"]}\n";
}
} // namespace

int main(int argc,char** argv){
    try{
        bool real=false,transactionContext=false;
        for(int n=1;n<argc;++n){
            if(std::string(argv[n])=="--real")real=true;
            if(std::string(argv[n])=="--transaction-context")transactionContext=true;
        }
        if(transactionContext){
            testPlanContextPreservesPools();testPlanOwnershipNeedsResolution();testPlanContextFailureAtomic();
            std::cout<<"{\"passed\":true,\"authored_transaction_context_cases\":3}\n";return 0;
        }
        if(real){realData();return 0;}
        testStrictDirection();testTargetWaterMissing();testBonusMissing();
        testRuntimeColumnMissing();testEmptyEarlyReturn();testTargetRowMissing();
        testRevisionValidation();testQueryIsReadOnly();testWaterIsBinary();
        testBranchesMatchEvolutionStep();testNativeCoverageZeroInfluence();
        testWellDesirabilityPropagation();testEdgeClippedInfluence();
        testPlanContextPreservesPools();testPlanOwnershipNeedsResolution();testPlanContextFailureAtomic();
        testPlanMatchesStrictStep();testPlanUnknownInputs();
        testUnsupportedCandidateDoesNotLeak();testPlanUsesActualModel();
        testPlanConsumesOnlyCandidateGoods();testPlanDeferredCounter();
        testV7IndependentWater();testV7KnownTailStock();testV7UnknownInputGates();
        testV7ActualGradeModel();testV7BonusCandidateAmbiguity();
        const auto env=makeEnv();const auto models=parseModels({-99,100,0,0,0,0});
        const auto start=std::chrono::steady_clock::now();
        const auto first=queryNativeHouseEvolution(env.source,env.state,models);
        const double firstMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        const auto repeatStart=std::chrono::steady_clock::now();
        const auto second=queryNativeHouseEvolution(env.source,env.state,models);
        const double repeatMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-repeatStart).count();
        need(first.houses.size()==second.houses.size(),"repeat house count");
        std::cout<<"{\"passed\":true,\"authored_house_cases\":27,\"rules_versions\":7,\"legacy_rules_versions\":6,\"foundation_version\":7,"
            <<"\"case_names\":[\"candidate_context_pools\",\"candidate_ownership_invariant\",\"candidate_context_failure_atomic\",\"strict_direction\",\"target_water_missing\",\"bonus_missing\","
            <<"\"runtime_column_missing\",\"empty_early_return\",\"target_row_missing\","
            <<"\"revision_validation\",\"read_only_invariance\",\"water_is_binary\","
            <<"\"branches_match_evolution_step\",\"native_coverage_zero_influence\","
            <<"\"well_desirability_propagation\",\"edge_clipped_influence\",\"plan_matches_strict_step\",\"plan_unknown_inputs\",\"unsupported_candidate_no_leak\",\"plan_actual_model\",\"candidate_goods_consumption\",\"candidate_delay_counter\",\"v7_independent_water\",\"v7_known_tail_stock\",\"v7_unknown_input_gates\",\"v7_actual_grade_model\",\"v7_bonus_candidate_ambiguity\"],"
            <<"\"water_binary_cases\":"<<waterBinaryCases<<",\"branch_step_cases\":"<<branchStepCases
            <<",\"plan_equivalence_cases\":"<<planEquivalenceCases
            <<",\"query_first_ms\":"<<firstMs<<",\"query_repeat_ms\":"<<repeatMs
            <<",\"houses\":"<<first.houses.size()<<"}\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
