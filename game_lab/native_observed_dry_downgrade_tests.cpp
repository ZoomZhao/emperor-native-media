// SPDX-License-Identifier: MIT
// Synthetic contract tests. No original data, GUI, asset loading or IO.
#include "native_observed_dry_downgrade.hpp"
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace game_lab;
namespace {
size_t checks=0,groups=0;
void need(bool yes,const char* why){++checks;if(!yes)throw std::runtime_error(why);}
constexpr auto enabled=NativeObservedDryDowngradePolicy::WaterOnlyType4To3V1;
struct Fixture {
    OriginalBuildingUpdateState source;
    OriginalBuildingUpdateContext context{OriginalBuildingUpdateScope::AdmittedTicks,true};
};
Fixture fixture(uint8_t delay=9,uint32_t part=3){
    Fixture f;auto& s=f.source;auto& b=s.building;auto& i=s.inputs;auto& c=f.context;
    b.type=4;b.slot=1;b.x=84;b.y=88;b.size=2;b.imageInputs.emplace();
    constexpr uint32_t base=44*229;const uint32_t grid=base+84+228*88;
    b.imageInputs->gridIndex=grid;b.imageInputs->randomByte=95;b.imageInputs->word20=14;
    i.alive=1;i.identity=1;i.model=1;i.residents=14;i.free=0;i.distance=3;i.houseInUse=1;
    i.house.emplace();i.house->fill(0);i.house->at(52)=delay;i.modelDesirability=0;
    c.phase=39;c.monthPart=part;c.cropBase=base;c.activeSide=140;
    c.evolutionDisabled=false;c.forceHouseEvolution=false;
    c.houseModels.emplace();(*c.houseModels)[0]={};auto& row=(*c.houseModels)[1];row={};row[0]=-14;row[1]=-2;row[3]=1;
    c.terrainOwners=std::vector<uint16_t>(OriginalTerrainState::cells,0);
    for(unsigned dy=0;dy<2;++dy)for(unsigned dx=0;dx<2;++dx)c.terrainOwners->at(grid+228*dy+dx)=1;
    c.buildings.emplace();c.buildingInputs.emplace();c.buildingLinks.emplace();c.houses.emplace();
    c.buildings->emplace(1,b);c.buildingInputs->emplace(1,i);
    OriginalFigureBuildingLink l;l.type=4;l.alive=1;l.residents=14;l.houseModel=1;l.freeSlots=0;l.accepting=1;l.cell=grid;
    c.buildingLinks->emplace(1,l);
    OriginalBuildingHouseCapacityInputs h;h.type=4;h.model=1;h.residents=14;h.free=0;h.distance=3;h.peak=19;
    c.houses->emplace(1,h);s.houseCapacity=h;
    c.houseEvolutionCalls=5;c.houseConversions=2;c.houseRedraws=3;c.populationTotal=14;c.populationPeak=19;
    c.houseModelCapacity=std::map<int,int>{{0,7},{1,14}};
    c.figureServices.emplace();c.figureServices->population=OriginalFigurePopulation{14,19,std::nullopt};
    return f;
}
std::string snapshot(const Fixture& f){
    std::ostringstream o;const auto add=[&](const auto& s){o<<s.building.type<<' '<<s.building.slot<<' '<<s.building.x<<' '<<s.building.y<<' '<<int(s.building.size)<<' '<<s.inputs.model<<' '<<s.inputs.residents<<' '<<s.inputs.free<<' '<<unsigned(s.inputs.houseWater)<<' ';if(s.inputs.house)for(auto n:*s.inputs.house)o<<unsigned(n)<<',';};
    add(f.source);const auto& c=f.context;o<<c.houseEvolutionCalls<<' '<<c.houseConversions<<' '<<c.houseRedraws<<' '<<c.houseEvolutionResult;
    if(c.terrainOwners)for(auto n:*c.terrainOwners)o<<n<<',';
    if(c.buildingInputs)for(const auto& [slot,i]:*c.buildingInputs){o<<slot<<':'<<i.model<<','<<i.residents<<','<<i.free<<';';if(i.house)for(auto n:*i.house)o<<unsigned(n)<<',';}
    if(c.buildingLinks)for(const auto& [slot,l]:*c.buildingLinks)o<<slot<<':'<<l.type<<','<<l.houseModel<<','<<l.residents<<','<<l.freeSlots<<';';
    if(c.houses)for(const auto& [slot,h]:*c.houses)o<<slot<<':'<<h.type<<','<<h.model<<','<<h.residents<<','<<h.free<<','<<h.peak<<';';
    for(auto slot:c.buildingRedraws)o<<slot<<',';return o.str();
}
void disabled(){
    ++groups;auto f=fixture();const auto before=snapshot(f);
    const auto p=planNativeObservedDryDowngrade(f.source,f.context);
    need(!p.candidate&&!p.candidateContext&&!p.direction,"default policy must not run");
    need(p.error=="observed_dry_downgrade_disabled","disabled reason explicit");
    need(snapshot(f)==before,"disabled attempt does not mutate inputs");
}
void delayAndConversion(){
    ++groups;
    for(uint8_t delay=0;delay<10;++delay){auto f=fixture(delay);const auto before=snapshot(f);
        const auto p=planNativeObservedDryDowngrade(f.source,f.context,enabled);
        need(p.candidate&&p.candidateContext&&p.error.empty(),"each admitted delay produces complete candidate");
        const auto& s=*p.candidate;const auto& c=*p.candidateContext;const bool converted=delay==9;
        need(p.direction==std::optional<int>{-1}&&p.retainsHouseIdentityAndGeometry,"explicit downward intent and invariants");
        need(s.inputs.house->at(52)==(converted?0:delay+1),"0..8 increment; saturated9 resets");
        need(s.building.type==(converted?3:4)&&s.inputs.model==(converted?0:1),"target only at delay9");
        need(s.inputs.residents==14&&s.inputs.free==0,"conversion preserves observed overcapacity and stale free");
        need(s.building.slot==1&&s.inputs.identity==1&&s.building.x==84&&s.building.y==88&&s.building.size==2,"same slot and complete square geometry");
        need(c.houseEvolutionCalls==6&&c.houseConversions==unsigned(converted?3:2),"one evolution call and at most one conversion");
        need(c.buildingRedraws.count(1)==size_t(converted)&&c.houseRedraws==3,"conversion redraw separated from goods-part bookkeeping");
        need(c.buildingInputs->at(1).model==s.inputs.model&&c.buildings->at(1).type==s.building.type,"local canonical pools match candidate");
        const auto& l=c.buildingLinks->at(1);need(l.type==s.building.type&&l.houseModel==s.inputs.model&&l.residents==14&&l.freeSlots==0,"figure link model/population/free retained");
        const auto& h=c.houses->at(1);need(h.type==s.building.type&&h.model==s.inputs.model&&h.residents==14&&h.free==0&&h.peak==19,"capacity mirror keeps population/free/peak");
        need(s.houseCapacity&&s.houseCapacity->model==s.inputs.model&&s.houseCapacity->peak==19,"optional local capacity mirror updated without recomputation");
        need(c.populationTotal==std::optional<int32_t>{14}&&c.figureServices->population->total==14,"no early excess departure");
        need(snapshot(f)==before,"successful plan never mutates source/context");
    }
    auto f=fixture(0);for(unsigned n=0;n<10;++n){const auto p=planNativeObservedDryDowngrade(f.source,f.context,enabled);need(bool(p.candidate),"sequential delay plan succeeds");f.source=*p.candidate;f.context=*p.candidateContext;need(f.source.building.type==(n==9?3:4),"nine full deferred calls before transition");}
    const auto repeated=planNativeObservedDryDowngrade(f.source,f.context,enabled);need(!repeated.candidate,"already downgraded house cannot repeat transition");
}
void guards(){
    ++groups;
    using Change=std::function<void(Fixture&)>;
    const std::vector<Change> changes={
        [](auto& f){f.context.phase.reset();},[](auto& f){f.context.phase=38;},
        [](auto& f){f.source.building.type=3;},[](auto& f){f.source.inputs.model=0;},
        [](auto& f){f.source.inputs.alive=3;},[](auto& f){f.source.inputs.residents=0;},
        [](auto& f){f.source.inputs.houseInUse=0;},[](auto& f){f.source.building.size=1;},
        [](auto& f){f.source.building.width=2;},[](auto& f){f.source.inputs.identity=2;},
        [](auto& f){f.source.inputs.house.reset();},[](auto& f){f.source.inputs.houseWater=1;},
        [](auto& f){f.source.inputs.house->at(45)=1;},[](auto& f){f.source.inputs.house->at(47)=1;},
        [](auto& f){f.context.evolutionDisabled=true;},[](auto& f){f.context.forceHouseEvolution=true;},
        [](auto& f){f.context.forceHouseEvolution.reset();},[](auto& f){f.context.houseModels.reset();},
        [](auto& f){f.context.houseModels->erase(0);},[](auto& f){f.context.houseModels->at(1)[3]=0;},
        [](auto& f){f.source.inputs.modelDesirability.reset();},[](auto& f){f.context.monthPart.reset();},
        [](auto& f){f.context.monthPart=16;},[](auto& f){f.context.terrainOwners.reset();},
        [](auto& f){f.context.terrainOwners->at(f.source.building.imageInputs->gridIndex+229)=2;},
        [](auto& f){++f.source.building.imageInputs->gridIndex;},[](auto& f){f.source.building.x=139;},
        [](auto& f){f.source.building.imageInputs->word20=7;},[](auto& f){f.source.building.imageInputs->houseByte3c=1;},
        [](auto& f){f.source.building.imageInputs->randomByte=94;},[](auto& f){f.source.inputs.house->at(52)=10;},
        [](auto& f){f.context.buildingInputs.reset();},[](auto& f){f.context.buildings->erase(1);},
        [](auto& f){f.context.houses->erase(1);},[](auto& f){f.context.buildingLinks->erase(1);},
        [](auto& f){f.context.houseEvolutionCalls=std::numeric_limits<unsigned>::max();}
    };
    for(const auto& change:changes){auto f=fixture();change(f);const auto before=snapshot(f);const auto p=planNativeObservedDryDowngrade(f.source,f.context,enabled);
        need(!p.candidate&&!p.candidateContext&&!p.error.empty(),"out-of-domain attempt exposes no partial candidate");need(snapshot(f)==before,"rejected guard leaves all source state untouched");}
    for(unsigned col:{2u,4u,5u,6u,7u,8u,9u,10u,11u,12u,13u,21u,22u,23u}){auto f=fixture();f.context.houseModels->at(1)[col]=1;const auto p=planNativeObservedDryDowngrade(f.source,f.context,enabled);need(!p.candidate&&p.error=="observed_dry_downgrade_water_only_model_required","any other service requirement is refused");}
    // Dry failure overrides no inferred DES direction. Wet houses, including
    // extreme DES, are explicitly outside this helper.
    for(int desire:{-99,0,99}){auto f=fixture();f.source.inputs.modelDesirability=int8_t(desire);f.source.inputs.houseWater=1;need(!planNativeObservedDryDowngrade(f.source,f.context,enabled).candidate,"no wet/recovery/DES upgrade branch");}
}
void contextAndPostlude(){
    ++groups;
    for(uint32_t part:{0u,7u}){auto f=fixture(9,part);const auto p=planNativeObservedDryDowngrade(f.source,f.context,enabled);need(p.candidateContext&&p.candidateContext->houseRedraws==4,"goods-part bookkeeping retained with zero goods demands");}
    auto targetGoods=fixture(9,7);targetGoods.context.houseModels->at(0)[10]=1;
    auto blocked=planNativeObservedDryDowngrade(targetGoods.source,targetGoods.context,enabled);
    need(!blocked.candidate&&!blocked.candidateContext&&blocked.error=="observed_dry_downgrade_target_goods_unported","unknown target consumption is explicit atomic refusal");
    auto f=fixture();const auto second=f.source;auto other=second.inputs;other.identity=2;other.residents=6;
    auto footprint=second.building;footprint.slot=2;footprint.type=5;
    f.context.buildingInputs->emplace(2,other);f.context.buildings->emplace(2,footprint);
    OriginalFigureBuildingLink link;link.type=5;f.context.buildingLinks->emplace(2,link);
    OriginalBuildingHouseCapacityInputs cap;cap.type=5;cap.model=2;cap.residents=6;cap.peak=21;f.context.houses->emplace(2,cap);
    const auto p=planNativeObservedDryDowngrade(f.source,f.context,enabled);
    need(p.candidateContext&&p.candidateContext->buildings->size()==2,"other slots retained");
    need(p.candidateContext->buildingInputs->at(2).residents==6&&p.candidateContext->houses->at(2).peak==21,"other population and peak retained");
    need(p.candidateContext->buildings->at(2).type==5&&p.candidateContext->buildingLinks->at(2).type==5,"other semantic type remains canonical");
    f.context.buildingLinks->erase(2);const auto before=snapshot(f);
    const auto refused=planNativeObservedDryDowngrade(f.source,f.context,enabled);
    need(!refused.candidate&&!refused.candidateContext&&refused.error=="observed_dry_downgrade_pool_sync_failed","late unrelated-pool sync failure publishes nothing");
    need(snapshot(f)==before,"late failure leaks no delay/model/context prefix");
    auto noPools=fixture();noPools.context.buildingInputs.reset();noPools.context.buildings.reset();noPools.context.buildingLinks.reset();noPools.context.houses.reset();
    need(bool(planNativeObservedDryDowngrade(noPools.source,noPools.context,enabled).candidate),"pure isolated caller may omit all pool mirrors");
}
}
int main(){try{disabled();delayAndConversion();guards();contextAndPostlude();
    std::cout<<"{\"groups\":"<<groups<<",\"checks\":"<<checks<<",\"passed\":true,\"synthetic_contract_only\":true}\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
