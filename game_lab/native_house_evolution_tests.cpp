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
size_t waterBinaryCases=0,branchStepCases=0;
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
        bool real=false;for(int n=1;n<argc;++n)if(std::string(argv[n])=="--real")real=true;
        if(real){realData();return 0;}
        testStrictDirection();testTargetWaterMissing();testBonusMissing();
        testRuntimeColumnMissing();testEmptyEarlyReturn();testTargetRowMissing();
        testRevisionValidation();testQueryIsReadOnly();testWaterIsBinary();
        testBranchesMatchEvolutionStep();testNativeCoverageZeroInfluence();
        testWellDesirabilityPropagation();testEdgeClippedInfluence();
        const auto env=makeEnv();const auto models=parseModels({-99,100,0,0,0,0});
        const auto start=std::chrono::steady_clock::now();
        const auto first=queryNativeHouseEvolution(env.source,env.state,models);
        const double firstMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        const auto repeatStart=std::chrono::steady_clock::now();
        const auto second=queryNativeHouseEvolution(env.source,env.state,models);
        const double repeatMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-repeatStart).count();
        need(first.houses.size()==second.houses.size(),"repeat house count");
        std::cout<<"{\"passed\":true,\"authored_house_cases\":13,\"rules_versions\":6,"
            <<"\"case_names\":[\"strict_direction\",\"target_water_missing\",\"bonus_missing\","
            <<"\"runtime_column_missing\",\"empty_early_return\",\"target_row_missing\","
            <<"\"revision_validation\",\"read_only_invariance\",\"water_is_binary\","
            <<"\"branches_match_evolution_step\",\"native_coverage_zero_influence\","
            <<"\"well_desirability_propagation\",\"edge_clipped_influence\"],"
            <<"\"water_binary_cases\":"<<waterBinaryCases<<",\"branch_step_cases\":"<<branchStepCases
            <<",\"query_first_ms\":"<<firstMs<<",\"query_repeat_ms\":"<<repeatMs
            <<",\"houses\":"<<first.houses.size()<<"}\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
