#include "original_building_update.hpp"
#include "original_save.hpp"
#include "original_pak.hpp"
#include "original_building_scene.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace game_lab;
namespace {
size_t checks=0;
void need(bool v,const char* message){++checks;if(!v)throw std::runtime_error(message);}
OriginalBuildingUpdateState field(uint16_t type=194,int16_t progress=0,uint16_t flags=0){
    OriginalBuildingUpdateState s;s.building.type=type;s.building.slot=7;s.building.size=1;
    s.building.imageInputs=OriginalBuildingImageInputs{};s.building.imageInputs->randomByte=0;s.building.imageInputs->cropWord4=progress;
    s.inputs.identity=7;s.inputs.alive=1;s.inputs.output=type==194?19:1;s.inputs.industry.emplace();
    s.inputs.industry->at(0)=uint8_t(progress);s.inputs.industry->at(1)=uint8_t(uint16_t(progress)>>8);
    originalBuildingSetCropFlags(s.inputs,flags);return s;
}
OriginalBuildingUpdateContext context(OriginalBuildingUpdateScope scope){
    OriginalBuildingUpdateContext c(scope,true);c.phase=0;c.monthPart=1;c.cycle=4;c.month=7;c.terrainFlags=8;c.typeDisabled=false;return c;
}
void authored(){
    auto c=context(OriginalBuildingUpdateScope::ProductionPasses);
    for(uint16_t type:{uint16_t(194),uint16_t(196)})for(uint16_t flags:{uint16_t(0),uint16_t(1),uint16_t(32),uint16_t(33),uint16_t(0x4000),uint16_t(0x8021)}){
        const int delta=(flags&255)>32?(type==194?125:157):(flags&255)?(type==194?63:79):0;
        auto s=field(type,1990,flags);auto r=originalBuildingStateAfterUpdates(s,c,1);
        need(r.state&&originalBuildingCropProgress(r.state->inputs)==1990+delta,"literal per-type growth uses LOW byte32 threshold");
        need(originalBuildingCropProgress(s.inputs)==1990&&originalBuildingCropFlags(s.inputs)==flags,"input state is immutable");
        need(r.changedSelectorInputs==unsigned(delta!=0)&&r.selectorChanged==(delta!=0),"existing crop selector reports threshold image change");
    }
    for(uint16_t type:{uint16_t(194),uint16_t(196)}){
        auto s=field(type,9990,33);auto r=originalBuildingStateAfterUpdates(s,c,100);
        need(r.state&&originalBuildingCropProgress(r.state->inputs)==10000,"production clamps at10000 for repeated passes");
        c.month=10;r=originalBuildingStateAfterUpdates(field(type,0,33),c,1);
        need(r.state&&originalBuildingCropProgress(r.state->inputs)==(type==194?125:0),"millet final harvest month10 differs from hemp");c.month=7;
        auto delay=field(type,100,33);delay.inputs.industry->at(95)=2;
        auto once=originalBuildingStateAfterUpdates(delay,c,1);need(once.state&&originalBuildingCropProgress(once.state->inputs)==100&&once.state->inputs.industry->at(95)==1,"delay decrements before all growth and disabled writes");
        c.typeDisabled=true;once=originalBuildingStateAfterUpdates(delay,c,2);need(once.state&&originalBuildingCropProgress(once.state->inputs)==100,"last delay pass still skips disabled reset");
        once=originalBuildingStateAfterUpdates(delay,c,3);need(once.state&&originalBuildingCropProgress(once.state->inputs)==0,"disabled crop reset after delay expires");c.typeDisabled=false;
        c.terrainFlags=0x108;r=originalBuildingStateAfterUpdates(field(type,500,33),c,10);need(r.state&&originalBuildingCropProgress(r.state->inputs)==500,"terrain100 skips crop production");c.terrainFlags=8;
        r=originalBuildingStateAfterUpdates(field(type,32760,33),c,1);need(r.state&&originalBuildingCropProgress(r.state->inputs)==(type==194?-32651:-32619),"x86 word addition wraps BEFORE signed clamp");
        auto full=originalBuildingStateAfterUpdates(field(type,0,33),c,10),split=originalBuildingStateAfterUpdates(field(type,0,33),c,4);
        auto rest=originalBuildingStateAfterUpdates(*split.state,*split.context,6);need(full.state&&rest.state&&full.state->inputs.industry==rest.state->inputs.industry,"split horizon equals repeated production");
    }
    for(uint16_t flags:{uint16_t(0),uint16_t(1),uint16_t(32),uint16_t(33),uint16_t(0x7fff),uint16_t(0xffff)}){
        auto s=field(194,0,flags);originalBuildingDecayCropFlags(s.inputs);
        const unsigned low=flags&255;need(originalBuildingCropFlags(s.inputs)==((flags&0xc000)|(low?low-1:0)),"crop month-part decay preserves C000 and drops middle bits");
    }
    auto house=field();house.building.type=5;house.inputs.industry.reset();house.inputs.house.emplace();house.inputs.house->fill(3);
    auto hc=context(OriginalBuildingUpdateScope::HouseServiceDecayPasses);auto h=originalBuildingStateAfterUpdates(house,hc,10);
    need(h.state&&h.serviceDecayCalls==10,"house service operation horizon closes");
    for(unsigned offset=4;offset<=64;++offset){const bool dec=offset==41||offset==42||offset==43||offset==44||offset==45||offset==49||offset==50||offset==51||(offset>=13&&offset<=16);
        need(h.state->inputs.house->at(offset-4)==(dec?0:3),"house literal serializer bytes decay and saturate; adjacent fields stay intact");}
    c=context(OriginalBuildingUpdateScope::AdmittedTicks);auto s=field();c.phase=20;
    auto r=originalBuildingStateAfterUpdates(s,c,1);need(r.state&&r.productionCalls==1&&*r.context->phase==21,"phase20 production precedes next admitted figure pass");
    r=originalBuildingStateAfterUpdates(s,c,2);need(!r.state&&r.blocker==OriginalBuildingUpdateBlocker::RoadRefresh,"phase21 refuses atomically, no production prefix");
    c.phase=31;c.monthPart=0;c.month=8;r=originalBuildingStateAfterUpdates(s,c,1);need(r.state&&(originalBuildingCropFlags(r.state->inputs)&0x8000)&&r.virtual20Calls==1,"phase31 hemp harvest flag is not production growth");
    c.scope=OriginalBuildingUpdateScope::ProductionPasses;s.inputs.output=0;r=originalBuildingStateAfterUpdates(s,c,1);
    need(r.state&&(originalBuildingCropFlags(r.state->inputs)&0x8000)&&originalBuildingCropProgress(r.state->inputs)==0,"production season prepass precedes even zero-output crop gate");
    c=context(OriginalBuildingUpdateScope::AdmittedTicks);c.phase=31;auto wall=field();wall.building.type=231;wall.inputs.industry.reset();
    r=originalBuildingStateAfterUpdates(wall,c,1);need(r.state&&r.virtual20Calls==1&&r.changedSelectorInputs==0,"wall virtual20 is cited false no-op, no crop growth substituted");
    c.phase=50;c.monthPart=1;s=field(194,500,0xc021);r=originalBuildingStateAfterUpdates(s,c,1);
    need(!r.state&&r.blocker==OriginalBuildingUpdateBlocker::MonthlyCity,"ordinary month-part wrap refuses unported4AC650 city work");
    c.monthPart=15;r=originalBuildingStateAfterUpdates(s,c,1);need(!r.state&&r.blocker==OriginalBuildingUpdateBlocker::MonthlyCity,"monthly work cannot masquerade as a closed wrap");
    c=context(OriginalBuildingUpdateScope::ProductionPasses);c.typeDisabled.reset();r=originalBuildingStateAfterUpdates(s,c,1);need(!r.state&&r.blocker==OriginalBuildingUpdateBlocker::MissingContext,"missing explicit disabled input refuses");
    c=context(OriginalBuildingUpdateScope::ProductionPasses);c.isolateBuildingWrites=false;r=originalBuildingStateAfterUpdates(s,c,1);need(!r.state&&r.blocker==OriginalBuildingUpdateBlocker::ExternalWrites,"intervening city/figure mutation premise required");
    c.isolateBuildingWrites=true;r=originalBuildingStateAfterUpdates(s,c,10001);need(!r.state,"bounded operation budget");
    auto inconsistent=s;inconsistent.building.imageInputs->cropWord4=9999;r=originalBuildingStateAfterUpdates(inconsistent,c,1);
    need(!r.state&&r.blocker==OriginalBuildingUpdateBlocker::InvalidState,"contradictory raw and selector progress refuses");
    // Literal read-order offsets with deliberately contradictory decoys.
    std::vector<uint8_t> globals(1986237+128,0);const size_t terrain=128,side=200;globals[0]=19;
    auto put32=[&](size_t at,uint32_t value){for(unsigned j=0;j<4;++j)globals.at(at+j)=uint8_t(value>>(8*j));};
    put32(terrain+1299328,19);put32(terrain+1299332,1);put32(terrain+1299336,113);put32(side-176,8);
    put32(terrain+1291328,45);put32(terrain+1291332,15);put32(terrain+1291336,999);
    put32(terrain,8);globals[terrain+1985968+194]=1;
    auto gc=originalBuildingSavedUpdateContext(globals,terrain,side,field().building,OriginalBuildingUpdateScope::ProductionPasses,true);
    need(gc&&gc->phase==19&&gc->monthPart==1&&gc->cycle==113&&gc->month==8&&gc->typeDisabled==true&&gc->terrainFlags==8,"literal corrected SAV19 read-order offsets drive context");
    globals.resize(terrain+1985968+268);need(!originalBuildingSavedUpdateContext(globals,terrain,side,field().building,OriginalBuildingUpdateScope::ProductionPasses,true),"missing final disabled-table byte refuses context");
    // Explicit parser fixture: common5 fields must not use neighbouring bytes.
    OriginalBuildingProfile p;p.status=OriginalBuildingStatus::Complete;p.parsedRecords=4000;p.liveTypeCounts[194]=1;p.slotTypes[7]=194;
    auto f=field().building;f.recordOffset=0;p.footprints.push_back(f);
    std::vector<uint8_t> bytes(2+182+140);bytes[0]=1;bytes[1]=128;auto w=[&](size_t at,uint16_t v){bytes.at(at)=uint8_t(v);bytes.at(at+1)=uint8_t(v>>8);};
    w(2,5);w(18,194);bytes[4]=1;bytes[66]=253;bytes[67]=19;bytes[131]=9;bytes[132]=201;w(2+162,7);
    w(184,1);w(186,1);w(188,4321);w(314,1);w(316,1);w(322,1);bytes[184+99]=17;w(184+106,0x8021);
    auto parsed=originalBuildingUpdateInputs(bytes,p);need(parsed&&parsed->size()==1&&parsed->at(7).output==19&&parsed->at(7).identity==7,"common5 exact wire65/162 parsed independently");
    need(originalBuildingCropProgress(parsed->at(7))==4321&&originalBuildingCropFlags(parsed->at(7))==0x8021&&parsed->at(7).industry->at(95)==17,"industrial1 byte mapping exact tail4/99/106");
    bytes.pop_back();need(!originalBuildingUpdateInputs(bytes,p),"late crop tail truncation refuses all update inputs");
    for(uint16_t type:{uint16_t(33),uint16_t(35),uint16_t(43)}){
        auto industrial=field();industrial.building.type=type;industrial.inputs.industry.reset();industrial.production.emplace();
        auto& ip=*industrial.production;ip.operational=1;ip.workers=8;ip.enabled=1;
        auto pc=context(OriginalBuildingUpdateScope::ProductionPasses);pc.unavailable=0;pc.requiredWorkers=8;pc.festivalProductionBoost=false;
        auto pr=originalBuildingStateAfterUpdates(industrial,pc,1);
        need(pr.state&&detail::i16(pr.state->production->industry.data())==10,"industry workforce100 percent produces10 with ordinary divisor10");
        pc.festivalProductionBoost=true;pr=originalBuildingStateAfterUpdates(industrial,pc,1);
        need(pr.state&&detail::i16(pr.state->production->industry.data())==20,"qualified festival doubles production via divisor5");
        pc.festivalProductionBoost=false;pr=originalBuildingStateAfterUpdates(industrial,pc,100);
        need(pr.state&&detail::i16(pr.state->production->industry.data())==(type==43?320:160),"industry caps differ kiln320 versus raw160");
        ip.industry[0x63-4]=2;ip.industry[0x5f-4]=3;pc.requiredWorkers.reset();pc.festivalProductionBoost.reset();
        pr=originalBuildingStateAfterUpdates(industrial,pc,2);
        need(pr.state&&pr.state->production->industry[0x63-4]==0&&pr.state->production->industry[0x5f-4]==3,"industry delay precedes model festival and auxiliary timer");
        pr=originalBuildingStateAfterUpdates(industrial,pc,3);need(!pr.state&&pr.blocker==OriginalBuildingUpdateBlocker::MissingContext,"industry positive growth requires model festival provenance");
        ip.primaryResource=18;ip.primaryStock=99;pr=originalBuildingStateAfterUpdates(industrial,pc,100);
        need(pr.state&&pr.state->production->industry==ip.industry,"input stock99 gate precedes delay");
        ip.primaryStock=100;ip.secondaryResource=1;ip.secondaryStock=99;pr=originalBuildingStateAfterUpdates(industrial,pc,100);
        need(pr.state&&pr.state->production->industry==ip.industry,"secondary stock99 gate is independent");
        ip.secondaryStock=100;pc.unavailable=1;pr=originalBuildingStateAfterUpdates(industrial,pc,100);
        need(pr.state&&pr.state->production->industry==ip.industry,"unavailable output precedes all input and delay writes");
        pc.unavailable.reset();ip.workers=0;pr=originalBuildingStateAfterUpdates(industrial,pc,100);
        need(pr.state&&pr.state->production->industry==ip.industry,"zero workers closes without unavailable model or festival guesses");
        pc.unavailable=0;pc.requiredWorkers=8;pc.festivalProductionBoost=false;ip.workers=8;ip.industry.fill(0);
        ip.industry[0]=0xf8;ip.industry[1]=0x7f;pr=originalBuildingStateAfterUpdates(industrial,pc,1);
        need(pr.state&&detail::i16(pr.state->production->industry.data())==-32766,"industry word wraps before signed cap");
    }
    //Independent literal perimeter order and ranked-network precedence.
    OriginalBuildingRoadInputs road;road.terrain.resize(51984);road.distance.resize(51984);road.owners.resize(51984);road.networks.resize(51984);
    auto rf=field().building;rf.x=20;rf.y=20;rf.size=2;rf.imageInputs->gridIndex=4580;
    road.terrain[4352]=road.terrain[4582]=0x40;road.distance[4352]=2;road.distance[4582]=50;
    road.networks[4352]=1;road.networks[4582]=2;road.rankedNetworks[0]=2;road.rankedNetworks[1]=1;
    auto access=originalBuildingRoadAccess(rf,road);
    need(access&&access->distance==50&&access->x==22&&access->y==20&&access->network==2,"ranked road network beats nearest distance");
    road.rankedNetworks[0]=1;road.rankedNetworks[1]=2;access=originalBuildingRoadAccess(rf,road);
    need(access&&access->distance==2&&access->x==20&&access->y==19,"rank ordering is caller state not fitted nearest road");
    road.networks[4582]=1;road.distance[4582]=1;access=originalBuildingRoadAccess(rf,road);
    need(access&&access->distance==2,"equal network rank preserves clockwise first candidate");
    road.terrain.assign(51984,0);access=originalBuildingRoadAccess(rf,road);
    need(access&&access->distance==1&&access->x==22,"fallback chooses nearest positive distance without terrain or owner gates");
    road.owners[4352]=7;access=originalBuildingRoadAccess(rf,road);need(!access,"unknown owner virtual D0 refuses even before fallback");
    road.ownerTypes.emplace();road.ownerTypes->at(7)=231;access=originalBuildingRoadAccess(rf,road);
    need(access&&access->distance==1,"cited wall owner rejects ranked candidate while fallback remains independent");
    road.networks.assign(51984,0);road.networks[0]=2;road.networks[1]=1;
    auto ranks=originalBuildingRankedRoadNetworks(road.networks);need(ranks&&ranks->at(0)==1&&ranks->at(1)==2,"equal network sizes retain ascending label order");
    std::vector<uint16_t> movement(51984);movement[10000]=movement[10001]=4;movement[10002]=0x40;movement[10229]=0x1000;
    auto distances=originalBuildingRoadDistances(movement,10000);
    need(distances&&distances->at(10000)==1&&distances->at(10001)==2&&distances->at(10002)==3&&distances->at(10229)==0,
        "road flood uses B7C including40 but excludes1000");
    need(!originalBuildingRoadDistances(movement,0),"road flood refuses original out-of-grid access premise");
    auto capacityContext=context(OriginalBuildingUpdateScope::AdmittedTicks);capacityContext.phase=22;capacityContext.houses.emplace();capacityContext.houseModelCapacity=std::map<int,int>{{0,3},{1,8},{2,15}};
    capacityContext.houses->emplace(7,OriginalBuildingHouseCapacityInputs{3,1,6,99,1,2,0});
    capacityContext.houses->emplace(8,OriginalBuildingHouseCapacityInputs{11,1,12,99,1,10,0});
    capacityContext.houses->emplace(9,OriginalBuildingHouseCapacityInputs{5,1,4,99,1,8,1});
    capacityContext.houses->emplace(10,OriginalBuildingHouseCapacityInputs{5,1,5,99,0,8,0});
    auto capacity=originalBuildingStateAfterUpdates(field(),capacityContext,1);
    need(capacity.state&&capacity.context->cityHouseCapacity==27&&capacity.context->cityHouseFree==5,"capacity totals include model11 next row and labor-disabled resident override");
    need(capacity.context->houses->at(7).free==2&&capacity.context->houses->at(8).free==3&&capacity.context->houses->at(9).free==0&&capacity.context->houses->at(10).free==0,"capacity resets disconnected free space and preserves signed field arithmetic");
    need(capacity.context->houses->at(7).peak==6&&capacity.context->houses->at(9).peak==8,"capacity peak raises only below residents");
    auto rc=context(OriginalBuildingUpdateScope::AdmittedTicks);rc.phase=21;rc.roadAccess=OriginalBuildingRoadAccess{12,3,4,uint8_t(2)};rc.roadRefreshOrdinaryExit=true;
    auto rr=originalBuildingStateAfterUpdates(field(),rc,1);need(rr.state&&rr.state->roadDistance==12&&rr.state->roadX==3&&rr.state->roadY==4&&rr.state->roadNetwork==2,"phase21 publishes cited road fields on verified ordinary city exit");
    rc.roadRefreshOrdinaryExit=false;rr=originalBuildingStateAfterUpdates(field(),rc,1);need(!rr.state&&rr.blocker==OriginalBuildingUpdateBlocker::RoadRefresh,"road repair branch refuses atomically");
    rc.roadRefreshOrdinaryExit=true;rc.roadAccess.reset();auto inactive=field();inactive.inputs.alive=2;
    rr=originalBuildingStateAfterUpdates(inactive,rc,1);need(rr.state&&!rr.state->roadDistance,"road phase skips nonlive record before per-record road input");
    OriginalBuildingCityEmployment city;city.available=7;city.shortageNotice=false;city.unemploymentNotice=false;
    city.rows[0]={10,99,4,99,5};city.rows[1]={10,99,4,99,5};city.rows[9]={99,98,97,96,95};
    auto account=originalBuildingCityEmploymentStep(city);need(account&&account->state.rows[0].allocated==4&&account->state.rows[1].allocated==3&&account->state.rows[0].preferredAllocated==4&&account->state.rows[1].preferredAllocated==3,"accounting preferred shortage distributes ordered integer remainder");
    need(account->state.rows[9].allocated==98&&account->state.rows[9].priority==95,"accounting touches nine of ten saved category rows");
    city.available=12;account=originalBuildingCityEmploymentStep(city);need(account&&account->state.rows[0].allocated==6&&account->state.rows[1].allocated==6&&account->state.shortage==8,"accounting ordinary remainder follows preferred allocation");
    city.available=30;city.shortageNotice=true;city.unemploymentNotice=true;account=originalBuildingCityEmploymentStep(city);
    need(account&&account->state.employed==20&&account->state.unemployed==10&&account->state.unemploymentPercent==33&&account->unemploymentNotification&&!account->shortageNotification,"accounting excess labor preserves exact notification branch order");
    city.shortageNotice.reset();need(!originalBuildingCityEmploymentStep(city),"unserialized notification flags are required inputs");
    city.shortageNotice=false;city.rows[0].priority=9;account=originalBuildingCityEmploymentStep(city);need(account&&account->state.rows[0].priority==3,"out-of-range positive priority normalizes to3 before demand test");
    //Independent serializer positions, with adjacent contradictory values.
    std::vector<uint8_t> population(13000);const size_t anchor=12287;population[0]=7;population[anchor-28]=6;
    const auto put=[&](size_t at,uint32_t value){for(unsigned j=0;j<4;++j)population.at(at+j)=uint8_t(value>>(8*j));};
    put(10190,77);put(10194,11);put(10198,12);put(10202,13);put(10206,14);put(10210,15);
    put(10394,21);put(10398,22);put(10402,23);put(10410,24);
    auto restored=originalBuildingSavedCityEmployment(population,anchor);
    need(restored&&restored->available==77&&restored->rows[0].demand==11&&restored->rows[0].allocated==12&&
        restored->rows[0].preferred==13&&restored->rows[0].preferredAllocated==14&&restored->rows[0].priority==15&&
        restored->employed==21&&restored->unemployed==22&&restored->unemploymentPercent==23&&restored->shortage==24,"accounting nested Population7 literal offsets preserve five distinct row fields");
    put(9904+6,0xfffffff9);need(originalBuildingSavedUnavailable(population,anchor,3)==-7,"unavailable uses signed word at Population7 wire9904");
    std::vector<uint8_t> scenario(900);scenario[0]=6;scenario[794]=11;scenario[796]=12;scenario[798]=13;scenario[800]=14;
    need(originalBuildingSavedRoadEndpoints(scenario,28)==std::array<int16_t,4>{11,12,13,14},"road scenario wire794 through800 is not runtime offset31C");
    std::vector<uint8_t> festival(3355+10*6+2+17+51984+83);const size_t festivalAt=3355+60+2+17+51984;
    for(unsigned n=0;n<10;++n)festival[3355+6*n]=1;festival[3355+60]=1;festival[festivalAt]=4;festival[festivalAt+2]=1;
    festival[festivalAt+66]=1;festival[festivalAt+67]=3;festival[festivalAt+71]=2;
    need(originalBuildingSavedProductionBoost(festival,35)==true&&originalBuildingSavedProductionBoost(festival,43)==true&&originalBuildingSavedProductionBoost(festival,33)==false,"festival production deity2 accepts clay and kiln but never Hunter");
    festival[festivalAt+71]=4;need(originalBuildingSavedProductionBoost(festival,35)==false,"Well water override deity4 cannot be substituted for production deity2");
    festival.resize(festivalAt+70);need(!originalBuildingSavedProductionBoost(festival,35),"truncated Festival4 does not invent false boost");
}
void original(){
    const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"skipped\":true,\"reason\":\"EMPEROR_GAMEDATA_ROOT unset or empty\"}\n";return;}
    auto root=ResourceRoot::fromAbsolute(env);need(root.configured(),"configured original root");
    auto catalog=enumerateOriginalMapSources(root);need(catalog.complete,"complete read-only source catalog");
    auto archive=loadSG3(root.resolve("DATA/China_General.sg3"));
    auto ground=loadSG3(root.resolve("DATA/China_Terrain.sg3")),elevation=loadSG3(root.resolve("DATA/China_Elevation.sg3"));
    const std::map<uint16_t,size_t> expected{{2,10},{3,8},{5,26},{7,26},{33,12},{35,12},{43,8},{53,4},{54,6},{58,2},{59,2},{62,4},{66,2},{67,2},{71,2},{72,2},{124,6},{126,6},{127,2},{192,6},{193,8},{194,287},{196,490},{207,2},{211,2},{214,2},{231,126}};
    std::vector<uint8_t> modelBytes;need(detail::readFile(root.resolve("Model/EmperorBuildingModels.txt"),1048576,modelBytes),"bounded production employment model");
    const auto employment=parseOriginalBuildingEmployment(std::string(modelBytes.begin(),modelBytes.end()));need(bool(employment),"difficulty-invariant production employment parsed");
    const auto houseModels=originalFigureHouseCapacities(std::string(modelBytes.begin(),modelBytes.end()));need(bool(houseModels),"existing cited house capacity model column");
    for(unsigned count:{1u,10u,48u,51u,100u}){
        std::map<uint16_t,size_t> live,tickComputed,productionComputed,selectorInputs,changedImages,decodedImages;
        std::map<std::string,size_t> tickRefused,productionRefused;size_t saves=0,consistent=0,growthRecords=0,serviceRecords=0,serviceChanged=0,serviceBytes=0;
        size_t roadComputed=0,roadRefused=0,industryChanged=0,industryGated=0,phase21Computed=0,phase22Computed=0,accountingComputed=0;
        std::map<uint16_t,size_t> tickSelectorInputs,tickImages,tickDecoded,industrialChanges;
        std::map<uint32_t,size_t> refusedPhases;std::map<size_t,std::pair<uint32_t,uint32_t>> capacityTotals;
        for(const auto& source:catalog.sources)if(source.kind==OriginalMapSourceKind::SaveFile){++saves;
            auto snapshot=loadOriginalSave(root.resolve(source.relativePath));need(snapshot.status==OriginalSaveStatus::LoadedSnapshot&&snapshot.readStable,"stable original save");
            std::vector<uint8_t> raw;need(detail::readFile(root.resolve(source.relativePath),4194304,raw),"bounded original read");auto container=decodeOriginalMapContainer(raw);need(container.complete,"closed original container");
            auto parsed=originalBuildingUpdateInputs(container.payload,snapshot.buildings);need(bool(parsed),"all original building update records framed from public parser");
            auto industry=originalBuildingProductionInputs(container.payload,snapshot.buildings);need(bool(industry),"all industrial production inputs framed independently");
            auto houses=originalBuildingHouseCapacityInputs(container.payload,snapshot.buildings);need(bool(houses),"all house capacity inputs independently framed");
            auto accounting=originalBuildingSavedCityEmployment(container.payload,snapshot.sizeOffset);need(bool(accounting),"nested Population7 restores city employment block");
            accounting->shortageNotice=false;accounting->unemploymentNotice=false; //explicit notification-history premises, never SAV fields.
            const auto account=originalBuildingCityEmploymentStep(*accounting);need(bool(account),"first accounting service closes on all six original snapshots");++accountingComputed;
            constexpr int32_t available[]={0,234,397,11,0,11},shortage[]={0,154,2,35,0,35};
            need(saves<=6&&accounting->available==available[saves-1]&&account->state.employed==available[saves-1]&&
                account->state.shortage==shortage[saves-1]&&account->state.unemployed==0,"original employment totals derived from nested serialized table");
            if(count==1)std::cout<<"{\"accounting_source\":"<<saves<<",\"available\":"<<accounting->available<<",\"employed_after\":"<<account->state.employed<<",\"unemployed_after\":"<<account->state.unemployed<<",\"shortage_after\":"<<account->state.shortage<<"}\n";
            std::optional<OriginalBuildingRoadInputs> roads;bool ordinaryRoadExit=false;
            {
                const auto endpoints=originalBuildingSavedRoadEndpoints(container.payload,snapshot.sizeOffset);
                const auto spawn=originalFigureSpawnServices(container.payload,snapshot.sizeOffset);
                need(endpoints&&spawn&&snapshot.layers.ground,"SAV19 scenario road endpoints and existing ground input contract");
                const auto scene=composeOriginalSavedGroundScene(snapshot.terrain,snapshot.buildings,*snapshot.layers.ground,archive,ground,elevation);
                auto surface=scene.surface;surface.terrain=scene.rebuiltMovementTerrain;
                auto masks=originalFigureMovementPassability(surface,snapshot.buildings,snapshot.layers.ground->owner,*spawn);
                if(masks){const int entry=int(spawn->cropBase)+endpoints->at(0)+228*endpoints->at(1);
                    need(entry>=0&&entry<51984,"saved road entry has valid backing coordinate");
                    auto distances=originalBuildingRoadDistances(masks->cells,uint32_t(entry));need(bool(distances),"original movement grid satisfies road flood memory bounds");
                    OriginalFigureUpdateContext networkContext;networkContext.terrain=surface.terrain;networkContext.passability=masks;networkContext.cropBase=spawn->cropBase;networkContext.activeSide=spawn->side;
                    const auto networks=originalFigureRoadNetworks(networkContext);need(bool(networks),"existing cited network producer");
                    const auto ranked=originalBuildingRankedRoadNetworks(*networks);need(bool(ranked),"ranked road network producer");
                    roads.emplace();roads->cropBase=spawn->cropBase;roads->terrain=surface.terrain;roads->distance=std::move(*distances);
                    roads->owners=snapshot.layers.ground->owner;roads->networks=*networks;roads->rankedNetworks=*ranked;roads->ownerTypes=snapshot.buildings.slotTypes;
                    const int exit=int(spawn->cropBase)+endpoints->at(2)+228*endpoints->at(3);
                    ordinaryRoadExit=exit>=0&&exit<51984&&roads->distance[size_t(exit)]!=0;
                    for(const auto& h:snapshot.buildings.footprints)if(originalBuildingIsHouse(h.type)&&!originalBuildingHouseRoadAccess(h,*roads))ordinaryRoadExit=false;
                    if(ordinaryRoadExit)for(const auto& h:snapshot.buildings.footprints)if(originalBuildingIsHouse(h.type))houses->at(h.slot).distance=int16_t(originalBuildingHouseRoadAccess(h,*roads)->distance);
                    need(ordinaryRoadExit,"five rebuilt original grids reach scenario exit and every house ordinary branch");
                }
            }
            auto buildings=snapshot.buildings.footprints;
            for(const auto& object:snapshot.buildings.objectBuildings){OriginalBuildingFootprint f;f.type=object.type;f.slot=object.slot;f.size=object.size;f.x=object.x;f.y=object.y;f.recordOffset=object.recordOffset;f.imageInputs=OriginalBuildingImageInputs{};f.imageInputs->gridIndex=object.gridIndex;buildings.push_back(f);}
            for(const auto& f:buildings){++live[f.type];OriginalBuildingUpdateState state{f,parsed->at(f.slot),std::nullopt};
                const auto ip=industry->find(f.slot);if(ip!=industry->end())state.production=ip->second;
                if(count==1){if(roads&&f.type!=59&&f.type!=62&&f.type!=66&&f.type!=67&&
                    (originalBuildingIsHouse(f.type)?originalBuildingHouseRoadAccess(f,*roads):originalBuildingRoadAccess(f,*roads)))++roadComputed;else ++roadRefused;}
                need(state.inputs.identity==f.slot,"saved building identity equals table slot");
                auto ctx=originalBuildingSavedUpdateContext(container.payload,snapshot.terrainOffset,snapshot.sizeOffset,f,OriginalBuildingUpdateScope::AdmittedTicks,true);need(bool(ctx),"explicit saved city phase and terrain context");
                ctx->unavailable=originalBuildingSavedUnavailable(container.payload,snapshot.sizeOffset,state.inputs.output);
                const auto required=employment->find(f.type);if(required!=employment->end())ctx->requiredWorkers=required->second;
                if(state.production){ctx->festivalProductionBoost=originalBuildingSavedProductionBoost(container.payload,f.type);need(bool(ctx->festivalProductionBoost),"Festival4 production predicate supplied");}
                if(roads){ctx->roadAccess=originalBuildingIsHouse(f.type)?originalBuildingHouseRoadAccess(f,*roads):originalBuildingRoadAccess(f,*roads);ctx->roadRefreshOrdinaryExit=ordinaryRoadExit;}
                ctx->houses=*houses;ctx->houseModelCapacity=*houseModels;
                if(count==1&&roads&&ordinaryRoadExit){auto passContext=*ctx;passContext.phase=21;
                    auto pass=originalBuildingStateAfterUpdates(state,passContext,1);if(pass.state){++phase21Computed;
                        need(pass.state->roadDistance&&pass.state->roadX&&pass.state->roadY,"road pass publishes every retained road field");}
                    passContext.phase=22;pass=originalBuildingStateAfterUpdates(state,passContext,1);need(bool(pass.state),"complete house capacity pass returns");++phase22Computed;
                    capacityTotals[saves]={*pass.context->cityHouseCapacity,*pass.context->cityHouseFree};
                    const std::array<std::pair<uint32_t,uint32_t>,6> totals{{{0,0},{572,0},{1066,69},{98,70},{0,0},{28,0}}};
                    need(capacityTotals[saves]==totals[saves-1],"phase22 original city capacity and free totals lock all supported sources");}
                if(state.inputs.industry){need(f.imageInputs&&f.imageInputs->cropWord4==originalBuildingCropProgress(state.inputs),"raw industrial progress equals independent existing selector input");++consistent;}
                auto result=originalBuildingStateAfterUpdates(state,*ctx,count);
                if(result.state){++tickComputed[f.type];tickSelectorInputs[f.type]+=result.changedSelectorInputs;
                    need(result.changedSelectorInputs==0&&!result.selectorChanged,"N1 admitted tick keeps all existing image selectors");
                }else {++tickRefused[originalBuildingUpdateBlockerName(result.blocker)];need(bool(result.refusedPhase),"admitted refusal names its exact phase");++refusedPhases[*result.refusedPhase];
                    need(result.operations==(*ctx->phase==0?1u:4u),"atomic refusal reports exact executed prefix without publishing state");}
                ctx->scope=OriginalBuildingUpdateScope::ProductionPasses;result=originalBuildingStateAfterUpdates(state,*ctx,count);
                if(!result.state){++productionRefused[originalBuildingUpdateBlockerName(result.blocker)];continue;}
                ++productionComputed[f.type];selectorInputs[f.type]+=result.changedSelectorInputs;
                if(state.production){if(state.production->industry!=result.state->production->industry){++industryChanged;++industrialChanges[f.type];}else ++industryGated;}
                if(state.inputs.industry&&originalBuildingCropProgress(state.inputs)!=originalBuildingCropProgress(result.state->inputs))++growthRecords;
                if(result.selectorChanged){++changedImages[f.type];auto bound=bindOriginalBuildingBaseImage(result.state->building,"DATA/China_General.sg3",archive);
                    need(bool(bound),"changed crop selector binds existing archive API");auto image=decodeSG3Sprite(archive,bound->archiveIndex,root.resolve("DATA/China_General.555"));need(image.status==SG3DecodeStatus::Decoded&&image.image.valid(),"changed production image word decodes");++decodedImages[f.type];}
                if(originalBuildingIsHouse(f.type)){ctx->scope=OriginalBuildingUpdateScope::HouseServiceDecayPasses;auto service=originalBuildingStateAfterUpdates(state,*ctx,count);need(bool(service.state),"original house service decay closes");++serviceRecords;
                    bool changedService=false;for(unsigned n=0;n<61;++n)if(state.inputs.house->at(n)!=service.state->inputs.house->at(n)){++serviceBytes;changedService=true;}
                    serviceChanged+=changedService;}
            }
        }
        need(saves==6&&live==expected&&consistent==777,"six-source1065/type and777 crop consistency denominators");
        size_t ticks=0,production=0,changed=0,decoded=0,inputs=0;for(const auto& e:tickComputed)ticks+=e.second;for(const auto& e:productionComputed)production+=e.second;
        for(const auto& e:changedImages)changed+=e.second;for(const auto& e:decodedImages)decoded+=e.second;for(const auto& e:selectorInputs)inputs+=e.second;
         need(production==1065&&productionComputed==expected&&productionComputed[194]==287&&productionComputed[196]==490&&productionComputed[59]==2,"production closes all27 types including32 industrial records");
        need(ticks==(count==1?1065u:0u),"requested admitted horizon denominator locked separately from subpasses");
        need(growthRecords==137&&inputs==137&&selectorInputs[194]==114&&selectorInputs[196]==23,"real growth and selector-input changes from saved flags, never fitted deltas");
        if(count==1||count==10||count==100)need(changed==(count==1?9u:count==10?82u:137u)&&changedImages[194]==(count==1?6u:count==10?63u:114u)&&
            changedImages[196]==(count==1?3u:count==10?19u:23u),"per-type image-stage transition denominator locks");
        need(serviceRecords==70,"all70 house service records close");
        need(serviceChanged==52&&serviceBytes==208,"52 of70 saved houses change208 service bytes without selector changes");
        const std::map<std::string,size_t> refused=count==1?std::map<std::string,size_t>{}:std::map<std::string,size_t>{{"city_labor_4ad4a0_4ada10_4adc90",17},{"city_accounting_4f1560_4ac2fa",1048}};
        need(tickRefused==refused&&productionRefused.empty(),"exact admitted and isolated refusal partitions");
        need(refusedPhases==(count==1?std::map<uint32_t,size_t>{}:std::map<uint32_t,size_t>{{1,1048},{23,17}}),"new phase23 blocker replaces phase20 and21 refusals");
        need(industryChanged==20&&industryGated==12,"20 of32 industrial records mutate and12 remain gated or saturated");
        need(industrialChanges==std::map<uint16_t,size_t>{{33,4},{35,8},{43,8}},"industrial mutation counts partition exactly by Hunter Clay Kiln");
        need(accountingComputed==6,"accounting allocation service closes6 of6 original cities");
        if(count==1)need(roadComputed==1055&&roadRefused==10&&phase21Computed==1055&&phase22Computed==1065,"road helper and ordinary phase21 exclude10 multipart overrides;phase22 covers1065");
        need(changed==decoded,"all changed image words decoded denominator");
        std::cout<<"{\"N\":"<<count<<",\"buildings\":1065,\"admitted_ticks_computed\":"<<ticks<<",\"production_passes_computed\":"<<production<<",\"crop_progress_records_changed\":"<<growthRecords<<",\"image_selector_inputs_changed\":"<<inputs<<",\"image_words_changed\":"<<changed<<",\"image_words_decoded\":"<<decoded<<",\"crop_raw_selector_consistency\":"<<consistent<<",\"house_service_records\":"<<serviceRecords<<",\"house_service_records_changed\":"<<serviceChanged<<",\"house_service_bytes_changed\":"<<serviceBytes<<"}\n";
        for(const auto& e:live)std::cout<<"{\"N\":"<<count<<",\"type\":"<<e.first<<",\"live\":"<<e.second<<",\"ticks_computed\":"<<tickComputed[e.first]<<",\"tick_selector_inputs_changed\":"<<tickSelectorInputs[e.first]<<",\"tick_images_changed\":"<<tickImages[e.first]<<",\"tick_images_decoded\":"<<tickDecoded[e.first]<<",\"production_computed\":"<<productionComputed[e.first]<<",\"industry_records_changed\":"<<industrialChanges[e.first]<<",\"selector_inputs_changed\":"<<selectorInputs[e.first]<<",\"images_changed\":"<<changedImages[e.first]<<",\"images_decoded\":"<<decodedImages[e.first]<<"}\n";
        for(const auto& e:tickRefused)std::cout<<"{\"N\":"<<count<<",\"scope\":\"admitted_ticks\",\"blocker\":\""<<e.first<<"\",\"refused\":"<<e.second<<"}\n";
        for(const auto& e:productionRefused)std::cout<<"{\"N\":"<<count<<",\"scope\":\"production_passes\",\"blocker\":\""<<e.first<<"\",\"refused\":"<<e.second<<"}\n";
        std::cout<<"{\"N\":"<<count<<",\"industry_changed\":"<<industryChanged<<",\"industry_unchanged\":"<<industryGated<<",\"road_service_computed\":"<<roadComputed<<",\"road_service_refused\":"<<roadRefused<<"}\n";
        for(const auto& total:capacityTotals)std::cout<<"{\"phase22_source\":"<<total.first<<",\"city_130F994_capacity\":"<<total.second.first<<",\"city_130F998_free\":"<<total.second.second<<"}\n";
    }
}
}
int main(){try{authored();const size_t authoredChecks=checks;original();std::cout<<"{\"passed\":true,\"authored_checks\":"<<authoredChecks<<",\"all_checks\":"<<checks<<"}\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
