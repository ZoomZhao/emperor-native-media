#include "original_building_update.hpp"
#include "original_save.hpp"
#include "original_pak.hpp"
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
}
void original(){
    const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"skipped\":true,\"reason\":\"EMPEROR_GAMEDATA_ROOT unset or empty\"}\n";return;}
    auto root=ResourceRoot::fromAbsolute(env);need(root.configured(),"configured original root");
    auto catalog=enumerateOriginalMapSources(root);need(catalog.complete,"complete read-only source catalog");
    auto archive=loadSG3(root.resolve("DATA/China_General.sg3"));
    const std::map<uint16_t,size_t> expected{{2,10},{3,8},{5,26},{7,26},{33,12},{35,12},{43,8},{53,4},{54,6},{58,2},{59,2},{62,4},{66,2},{67,2},{71,2},{72,2},{124,6},{126,6},{127,2},{192,6},{193,8},{194,287},{196,490},{207,2},{211,2},{214,2},{231,126}};
    for(unsigned count:{1u,10u,100u}){
        std::map<uint16_t,size_t> live,tickComputed,productionComputed,selectorInputs,changedImages,decodedImages;
        std::map<std::string,size_t> tickRefused,productionRefused;size_t saves=0,consistent=0,growthRecords=0,serviceRecords=0,serviceChanged=0,serviceBytes=0;
        for(const auto& source:catalog.sources)if(source.kind==OriginalMapSourceKind::SaveFile){++saves;
            auto snapshot=loadOriginalSave(root.resolve(source.relativePath));need(snapshot.status==OriginalSaveStatus::LoadedSnapshot&&snapshot.readStable,"stable original save");
            std::vector<uint8_t> raw;need(detail::readFile(root.resolve(source.relativePath),4194304,raw),"bounded original read");auto container=decodeOriginalMapContainer(raw);need(container.complete,"closed original container");
            auto parsed=originalBuildingUpdateInputs(container.payload,snapshot.buildings);need(bool(parsed),"all original building update records framed from public parser");
            auto buildings=snapshot.buildings.footprints;
            for(const auto& object:snapshot.buildings.objectBuildings){OriginalBuildingFootprint f;f.type=object.type;f.slot=object.slot;f.size=object.size;f.x=object.x;f.y=object.y;f.recordOffset=object.recordOffset;f.imageInputs=OriginalBuildingImageInputs{};f.imageInputs->gridIndex=object.gridIndex;buildings.push_back(f);}
            for(const auto& f:buildings){++live[f.type];OriginalBuildingUpdateState state{f,parsed->at(f.slot)};
                need(state.inputs.identity==f.slot,"saved building identity equals table slot");
                auto ctx=originalBuildingSavedUpdateContext(container.payload,snapshot.terrainOffset,snapshot.sizeOffset,f,OriginalBuildingUpdateScope::AdmittedTicks,true);need(bool(ctx),"explicit saved city phase and terrain context");
                if(state.inputs.industry){need(f.imageInputs&&f.imageInputs->cropWord4==originalBuildingCropProgress(state.inputs),"raw industrial progress equals independent existing selector input");++consistent;}
                auto result=originalBuildingStateAfterUpdates(state,*ctx,count);
                if(result.state)++tickComputed[f.type];else ++tickRefused[originalBuildingUpdateBlockerName(result.blocker)];
                ctx->scope=OriginalBuildingUpdateScope::ProductionPasses;result=originalBuildingStateAfterUpdates(state,*ctx,count);
                if(!result.state){++productionRefused[originalBuildingUpdateBlockerName(result.blocker)];continue;}
                ++productionComputed[f.type];selectorInputs[f.type]+=result.changedSelectorInputs;
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
        need(production==1033&&productionComputed[194]==287&&productionComputed[196]==490&&productionComputed[59]==2&&
            productionComputed[33]==0&&productionComputed[35]==0&&productionComputed[43]==0,"production closes777 crops and256 zero-output records;32 industry records refuse");
        need(ticks==(count==1?1065u:0u),"requested admitted horizon denominator locked separately from subpasses");
        need(growthRecords==137&&inputs==137&&selectorInputs[194]==114&&selectorInputs[196]==23,"real growth and selector-input changes from saved flags, never fitted deltas");
        need(changed==(count==1?9u:count==10?82u:137u)&&changedImages[194]==(count==1?6u:count==10?63u:114u)&&
            changedImages[196]==(count==1?3u:count==10?19u:23u),"per-type image-stage transition denominator locks");
        need(serviceRecords==70,"all70 house service records close");
        need(serviceChanged==52&&serviceBytes==208,"52 of70 saved houses change208 service bytes without selector changes");
        const std::map<std::string,size_t> refused=count==1?std::map<std::string,size_t>{}:std::map<std::string,size_t>{{"production_virtuals_51e4a0_74_208",2},{"road_refresh_4acfc0_426df0_1391fe0",15},{"city_accounting_4f1560_4ac2fa",1048}};
        need(tickRefused==refused&&productionRefused==std::map<std::string,size_t>{{"production_virtuals_51e4a0_74_208",32}},"refusal partition is exact and named for each requested operation");
        need(changed==decoded,"all changed image words decoded denominator");
        std::cout<<"{\"N\":"<<count<<",\"buildings\":1065,\"admitted_ticks_computed\":"<<ticks<<",\"production_passes_computed\":"<<production<<",\"crop_progress_records_changed\":"<<growthRecords<<",\"image_selector_inputs_changed\":"<<inputs<<",\"image_words_changed\":"<<changed<<",\"image_words_decoded\":"<<decoded<<",\"crop_raw_selector_consistency\":"<<consistent<<",\"house_service_records\":"<<serviceRecords<<",\"house_service_records_changed\":"<<serviceChanged<<",\"house_service_bytes_changed\":"<<serviceBytes<<"}\n";
        for(const auto& e:live)std::cout<<"{\"N\":"<<count<<",\"type\":"<<e.first<<",\"live\":"<<e.second<<",\"ticks_computed\":"<<tickComputed[e.first]<<",\"production_computed\":"<<productionComputed[e.first]<<",\"selector_inputs_changed\":"<<selectorInputs[e.first]<<",\"images_changed\":"<<changedImages[e.first]<<",\"images_decoded\":"<<decodedImages[e.first]<<"}\n";
        for(const auto& e:tickRefused)std::cout<<"{\"N\":"<<count<<",\"scope\":\"admitted_ticks\",\"blocker\":\""<<e.first<<"\",\"refused\":"<<e.second<<"}\n";
        for(const auto& e:productionRefused)std::cout<<"{\"N\":"<<count<<",\"scope\":\"production_passes\",\"blocker\":\""<<e.first<<"\",\"refused\":"<<e.second<<"}\n";
    }
}
}
int main(){try{authored();const size_t authoredChecks=checks;original();std::cout<<"{\"passed\":true,\"authored_checks\":"<<authoredChecks<<",\"all_checks\":"<<checks<<"}\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
