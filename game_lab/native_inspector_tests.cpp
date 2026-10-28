// SPDX-License-Identifier: MIT
// Independent preventive-Inspector integration. Original inputs are read only;
// authored counterexamples never count as natural or original-game evidence.
#include "native_house_runtime_source.hpp"
#include "native_session_io.hpp"
#include "native_inspector_feedback.hpp"
#include "native_session_feedback.hpp"
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <iostream>
#include <set>

using namespace game_lab;
namespace {
size_t checks=0;
void need(bool v,const std::string& reason){++checks;if(!v)throw std::runtime_error(reason);}
std::string wire(const NativeSessionSeed& seed,const NativeSessionState& s){
    const auto b=encodeNativeSession(seed,s);need(!b.empty(),"state must encode: "+validateNativeSession(seed,s));return b;
}
void authoredCodec(){
    std::string error;
    OriginalFigure f;f.slot=7;f.owner=8;f.type=39;f.alive=1;f.cell=100;f.action=6;f.counter=30;
    f.roamLimit=3840;f.roamState=1;f.searchCounter=19;f.watchCoverage=255;f.allocationSerial=65535;
    for(uint8_t action:{6,7,8,9}){
        f.action=action;const auto b=encodeNativeInspector(f,error);need(bool(b),"authored39 codec: "+error);
        const auto r=decodeNativeInspector(*b,error);need(r&&r->type==39&&r->action==action&&r->counter==30&&
            r->searchCounter==19&&r->watchCoverage==255&&r->allocationSerial==65535,"authored39 complete fields");
        need(encodeNativeInspector(*r,error)==b,"authored39 exact codec roundtrip");
        need(!decodeNativeInspector(b->substr(0,b->size()-1),error),"truncated39 payload refuses");
        auto bad=*b;bad.insert(bad.size()/2,1,'\0');need(!decodeNativeInspector(bad,error),"NUL39 payload refuses");
    }
    f.type=28;need(!encodeNativeInspector(f,error),"39 wrapper must not accept28");
    const auto well=encodeNativeWellWalker(f,error);need(bool(well),"old28 wrapper remains available");
    need(!decodeNativeInspector(*well,error),"39 cannot masquerade as28 wire");
    f.type=39;f.alive=0;need(!encodeNativeInspector(f,error),"dead39 is not live payload");
    // A real historical literal, not a regenerated golden.
    NativeSessionSeed seed;seed.source="golden.sav";seed.sourceRevision=1;seed.modelRevision=2;seed.side=8;
    seed.initialFunds=1000;seed.roadCost=2;seed.houseCost=15;seed.houseCapacity=7;seed.wellCost=25;
    seed.hunterCost=32;seed.laborPercent=50;seed.hunterWorkers=2;seed.wellWorkers=4;
    seed.laborPriority.fill(3);seed.buildable.assign(64,1);seed.sourceRoad.assign(64,0);
    const std::string old="ENCITY 1 original-data-native-v1\n\"golden.sav\" 1 2\n8 1000 2 15 7\n0 0 1 1 1000 0\n";
    auto restored=decodeNativeSession(seed,old,error);need(restored&&encodeNativeSession(seed,*restored)==old,"legacy literal v1 bytes unchanged");
    for(unsigned v=1;v<=6;++v){auto s=startNativeSession(seed,v);const auto b=wire(seed,s);
        auto d=decodeNativeSession(seed,b,error);need(d&&!d->inspection&&encodeNativeSession(seed,*d)==b,"legacy version unaffected");
        auto refused=transactNativeSession(seed,*d,{NativeSessionAction::Inspector,d->revision,1,1});
        need(!refused.committed&&encodeNativeSession(seed,*d)==b,"legacy cannot silently opt into tower");
    }
}
struct Episode {
    size_t slot=0,owner=0;uint16_t serial=0;uint64_t firstTick=0,lastTick=0;
    std::set<unsigned> actions;bool walked=false,reduced=false,completed=false;
};
using Key=std::pair<size_t,uint16_t>;
std::map<Key,OriginalFigure> inspectors(const NativeSessionState& s){
    std::map<Key,OriginalFigure> out;std::string error;need(bool(s.inspection),"inspection state exists");
    for(const auto& p:s.inspection->payloads){const auto f=decodeNativeInspector(p,error);need(bool(f),"39 payload: "+error);
        need(out.emplace(Key{f->slot,f->allocationSerial},*f).second,"unique39 allocation identity");}
    return out;
}
void stagePhase(NativeSessionState& s,uint8_t phase){
    // Authored local branch only; never used by the natural loop.
    s.houseSchedule->phase=phase;
    if(s.cityContext){std::string e;auto c=decodeNativeCityContext(*s.cityContext,e);need(bool(c),"local phase context decode");
        if(c->goods)c->goods->savedPhase=phase;const auto b=encodeNativeCityContext(*c,e);need(bool(b),"local phase context encode");s.cityContext=*b;}
}
struct Harness {
    const ResourceRoot& root;NativeSessionSource& source;std::filesystem::path output;
    size_t files=0;
    const NativeSessionSeed& seed()const{return source.seed;}
    NativeSessionResult command(NativeSessionState& s,NativeSessionCommand c,bool commit=true){
        c.expectedRevision=s.revision;const auto r=transactNativeHouseRuntime(seed(),s,c,*source.runtime);
        if(commit)need(r.committed,"command "+std::to_string(int(c.action))+" tick="+std::to_string(s.ticks)+" phase="+std::to_string(s.houseSchedule->phase)+": "+r.error);
        return r;
    }
    void action(NativeSessionState& s,NativeSessionAction a,int x=0,int y=0){command(s,{a,s.revision,x,y});}
    NativeSessionState fileRoundtrip(const NativeSessionState& s,const std::string& name){
        const auto b=wire(seed(),s);const auto file=(output/(name+".encity")).string();std::string error;
        need(writeNativeSessionNew(file,root.configuredDirectory(),b,error),"new checkpoint: "+error);
        const auto read=readNativeSessionFile(file,root.configuredDirectory(),error);need(read&&*read==b,"actual checkpoint read exact");
        const auto d=read?decodeNativeSession(seed(),*read,error):std::nullopt;need(bool(d),"checkpoint decode: "+error);
        need(validateNativeHouseRuntimeReady(seed(),*d,*source.runtime).empty(),"checkpoint resource ready");
        need(wire(seed(),*d)==b,"all saved fields roundtrip exactly");
        need(!writeNativeSessionNew(file,root.configuredDirectory(),b,error),"checkpoint overwrite refused");++files;return *d;
    }
};
struct Layout{int x=-1,y=-1;std::vector<int> path;};
Layout chooseLayout(const NativeSessionSeed& seed){
    const int n=int(seed.side);const std::vector<std::pair<int,int>> buildings={{0,1},{0,4},{3,4},{6,4},{3,1},{6,1},{9,4}};
    for(int y=1;y+7<n;++y)for(int x=1;x+12<n;++x){
        bool okay=true;for(int dy=0;dy<7&&okay;++dy)for(int dx=0;dx<12;++dx)
            if(!seed.buildable[size_t(y+dy)*n+x+dx]){okay=false;break;}
        if(!okay)continue;
        std::vector<uint8_t> reserved(size_t(n)*n);for(const auto& b:buildings)for(int dy=0;dy<2;++dy)for(int dx=0;dx<2;++dx)reserved[size_t(y+b.second+dy)*n+x+b.first+dx]=1;
        const int start=y*n+x;std::deque<int> queue{start};std::vector<int> prior(size_t(n)*n,-1);prior[start]=start;int end=-1;
        while(!queue.empty()){
            const int cell=queue.front();queue.pop_front();if(seed.sourceRoad[size_t(cell)]){end=cell;break;}
            for(const auto& d:{std::pair<int,int>{1,0},{-1,0},{0,1},{0,-1}}){const int nx=cell%n+d.first,ny=cell/n+d.second;
                if(nx<0||ny<0||nx>=n||ny>=n)continue;const int next=ny*n+nx;
                if(prior[size_t(next)]>=0||reserved[size_t(next)]||(!seed.buildable[size_t(next)]&&!seed.sourceRoad[size_t(next)]))continue;
                prior[size_t(next)]=cell;queue.push_back(next);}
        }
        if(end<0)continue;Layout out{x,y,{}};
        for(int cell=end;;cell=prior[size_t(cell)]){out.path.push_back(cell);if(cell==start)break;}
        if(out.path.size()*seed.roadCost+100*seed.roadCost+4*seed.houseCost+seed.wellCost+seed.hunterCost+seed.inspectorCost>uint64_t(seed.initialFunds))continue;
        return out;
    }
    throw std::runtime_error("no qualified connected twelve-by-seven native construction layout");
}
uint64_t buildNaturalCity(Harness& h,NativeSessionState& state,const Layout& layout){
    const int x=layout.x,y=layout.y,n=int(h.seed().side);
    const auto road=[&](int rx,int ry){if(!nativeRoads(h.seed(),state)[size_t(ry)*n+rx])h.action(state,NativeSessionAction::Road,rx,ry);};
    for(int cell:layout.path)road(cell%n,cell/n);
    for(int dx=0;dx<12;++dx)for(int dy:{0,3,6})road(x+dx,y+dy);
    for(int dx:{2,5,8,11})for(int dy=1;dy<6;++dy)road(x+dx,y+dy);
    for(const auto& p:{std::pair<int,int>{0,1},{0,4},{3,4},{6,4}})h.action(state,NativeSessionAction::House,x+p.first,y+p.second);
    h.action(state,NativeSessionAction::Well,x+3,y+1);h.action(state,NativeSessionAction::Hunter,x+6,y+1);
    const auto before=wire(h.seed(),state);const auto expectedCost=h.seed().inspectorCost;
    const auto preview=previewNativeSessionCommand(h.seed(),state,{NativeSessionAction::Inspector,state.revision,x+9,y+4},h.source.runtime.get());
    need(preview.committed&&wire(h.seed(),state)==before,"tower preview admits without mutation");
    const auto id=state.nextId;const auto funds=state.funds;h.action(state,NativeSessionAction::Inspector,x+9,y+4);
    need(state.funds==funds-int64_t(expectedCost),"tower uses original model price exactly once");
    need(std::any_of(state.objects.begin(),state.objects.end(),[&](const auto& o){return o.id==id&&o.kind==NativeObjectKind::Inspector;}),"native tower identity124");
    need(state.inspection->payloads.empty(),"construction alone does not dispatch or imply coverage");
    const auto q=queryNativeInspectorFeedback(state,id);need(q.enabled&&q.tower&&!q.visit,"staffing is not an inspection proof");
    const auto scene=nativeSessionScene(h.source,state);need(bool(scene),"tower scene projects");
    auto fp=std::find_if(scene->buildings.footprints.begin(),scene->buildings.footprints.end(),[&](const auto& f){return f.slot==id;});
    need(fp!=scene->buildings.footprints.end()&&fp->type==124&&originalBuildingBaseSelection(*fp).has_value(),"tower is not rendered as an empty House");
    return id;
}
void naturalStaffing(Harness& h,const NativeSessionState& dispatched,uint64_t towerId){
    std::string error;auto bridge=nativeToStrict(h.seed(),dispatched,h.source.runtime->strictTemplate,nativeStrictPersisted(dispatched));
    need(bridge.city&&bridge.city->buildingContext&&dispatched.populationRuntime,"natural staffing bridge: "+bridge.refusal);
    const auto& i=bridge.city->buildings.at(size_t(towerId)).state.inputs;const auto& context=*bridge.city->buildingContext;
    const auto category=originalBuildingEmploymentCategory(124,context);need(category&&*category==3,"model124 actual employment category");
    const auto& row=dispatched.populationRuntime->employment.rows[size_t(*category)];
    need(i.operational>0&&i.enabled&&!i.parent&&!i.suppression&&!i.laborDisabled,"naturally dispatched tower is eligible for real preferred jobs");
    need(row.demand>=row.preferred&&row.preferred>=int32_t(h.seed().inspectorWorkers),"eligible tower contributes its loaded preferred demand");
    need(i.workers>0&&i.workers<=int32_t(h.seed().inspectorWorkers)&&row.preferredAllocated>=i.workers&&row.preferredAllocated<=row.preferred,"natural actual staffing is bounded by loaded demand and allocated capacity");
    const auto saved=decodeNativeCityContext(*dispatched.cityContext,error);need(saved&&saved->workers.at(size_t(towerId))==i.workers,"naturally staffed tower mirror matches unique authority");
    need(dispatched.populationRuntime->employment.employed<=dispatched.populationRuntime->employment.available&&nativePopulation(dispatched)>=uint64_t(i.workers),"real worker allocation has available resident backing");
    std::cout<<"{\"natural_eligible_staffing\":true,\"tower\":"<<towerId<<",\"category\":"<<*category<<",\"workers\":"<<i.workers<<",\"required\":"<<h.seed().inspectorWorkers<<",\"preferred\":"<<row.preferred<<",\"preferred_allocated\":"<<row.preferredAllocated<<"}"<<std::endl;
}
void authoredBranches(Harness& h,const NativeSessionState& dispatched,uint64_t towerId){
    std::string error;
    const auto committed=wire(h.seed(),dispatched);
    for(const auto& object:dispatched.objects)if(object.kind==NativeObjectKind::House){
        const auto q=queryNativeInspectorFeedback(dispatched,object.id);
        need(q.recordKnown&&!q.visit,"first dispatch staffing alone never certifies a house visit");
        const auto text=nativeInspectionRecordText(q,dispatched.ticks,true,h.seed().inspectorWorkers,h.seed().inspectorWorkers,true);
        need(text.find("尚无实际巡检记录")!=std::string::npos&&text.find("不是概率")!=std::string::npos,"house UI says not inspected and not probability");
    }
    need(wire(h.seed(),dispatched)==committed,"query and text do not allocate a service");
    // This clone is labelled an authored branch. Natural proof is gathered
    // separately from the uninterrupted city and is never taken from here.
    auto upgraded=dispatched;auto record=decodeNativeStrictRecord(upgraded.strictRecords.at(towerId),error);need(bool(record),"local tower record");
    record->model=1;record->modelUpgrade=0;record->spawnCounter=1;
    upgraded.strictRecords[towerId]=encodeNativeStrictRecord(*record);stagePhase(upgraded,31);
    const auto beforeCount=upgraded.inspection->payloads.size();h.action(upgraded,NativeSessionAction::Tick);
    need(upgraded.inspection->payloads.size()>beforeCount,"upgraded tower creates a second Inspector");
    auto bridge=nativeToStrict(h.seed(),upgraded,h.source.runtime->strictTemplate,nativeStrictPersisted(upgraded));need(bool(bridge.city),"upgraded owner bridge: "+bridge.refusal);
    const auto& link=bridge.city->figures.buildings->at(size_t(towerId));
    need(link.primaryFigure>0&&link.secondaryFigures[1]>0&&link.secondaryFigures[0]==0,"Inspector auxiliary owner uses matching +6C contract");
    const auto& primary=bridge.city->figures.figures.at(size_t(link.primaryFigure));const auto& secondary=bridge.city->figures.figures.at(size_t(link.secondaryFigures[1]));
    need(primary.type==39&&secondary.type==39&&primary.owner==int(towerId)&&secondary.owner==int(towerId),"both Inspector owner links reciprocal");
    auto downgraded=upgraded;auto falling=decodeNativeStrictRecord(downgraded.strictRecords.at(towerId),error);need(bool(falling),"downgrade strict record");
    const auto& upgradeModels=h.source.runtime->strictTemplate.emptyCity.buildingContext->buildingModels;
    need(upgradeModels&&upgradeModels->count(124)&&upgradeModels->at(124)[10]>0,"actual Inspector model has desirability lifecycle boundary");
    falling->modelDesirability=int8_t(std::min<int32_t>(127,upgradeModels->at(124)[10]-1));falling->modelUpgrade=0;
    downgraded.strictRecords[towerId]=encodeNativeStrictRecord(*falling);stagePhase(downgraded,40);
    const auto returnsBefore=downgraded.inspection->returns;const auto dispatchesBefore=downgraded.inspection->dispatches;
    const auto maximumBefore=downgraded.inspection->maximum;const auto historiesBefore=downgraded.inspection->visits.size();
    h.action(downgraded,NativeSessionAction::Tick);
    auto fallen=decodeNativeStrictRecord(downgraded.strictRecords.at(towerId),error);need(fallen&&fallen->model==0&&fallen->modelUpgrade==0,"phase40 actually downgraded Tower");
    need(fallen->primaryFigure==link.primaryFigure&&fallen->hookIndustry&&detail::i16(fallen->hookIndustry->data()+0x6c-4)==0,"downgrade retains main owner and clears only auxiliary");
    need(downgraded.inspection->payloads.size()==1&&downgraded.inspection->returns==returnsBefore+1&&downgraded.inspection->dispatches==dispatchesBefore,"only one auxiliary returns to FIFO");
    auto retained=decodeNativeInspector(downgraded.inspection->payloads.front(),error);need(retained&&retained->slot==size_t(link.primaryFigure)&&retained->counter<primary.counter,"main Inspector continues its natural update");
    need(fallen->fireRisk==falling->fireRisk&&fallen->damageRisk==falling->damageRisk&&downgraded.inspection->maximum==maximumBefore&&downgraded.inspection->visits.size()==historiesBefore,"downgrade does not clear risk or service history");
    need(validateNativeHouseRuntimeReady(h.seed(),downgraded,*h.source.runtime).empty(),"downgraded graph is resource ready");
    stagePhase(downgraded,31);h.action(downgraded,NativeSessionAction::Tick);need(downgraded.inspection->payloads.size()==1,"base tower does not respawn auxiliary");
    auto paused=upgraded;h.action(paused,NativeSessionAction::Pause);auto removed=paused;
    const auto cancelBefore=wire(h.seed(),paused);const auto cancelPreview=previewNativeSessionCommand(h.seed(),paused,{NativeSessionAction::Demolish,paused.revision,0,0,0,0,towerId},h.source.runtime.get());
    need(cancelPreview.committed&&wire(h.seed(),paused)==cancelBefore,"demolition preview/cancel is pure");
    const size_t freeBefore=removed.figureRuntime->freeFigureSlots.size();h.command(removed,{NativeSessionAction::Demolish,removed.revision,0,0,0,0,towerId});
    need(removed.inspection->payloads.empty(),"demolish tower removes main and auxiliary Inspectors");
    need(removed.figureRuntime->freeFigureSlots.size()==freeBefore+2,"two Inspector slots returned once");
    for(int16_t owner:removed.figureRuntime->services.routeOwners)need(owner!=link.primaryFigure&&owner!=link.secondaryFigures[1],"no Inspector route owner remains");
    const auto removedBytes=wire(h.seed(),removed);const auto twice=h.command(removed,{NativeSessionAction::Demolish,removed.revision,0,0,0,0,towerId},false);
    need(!twice.committed&&wire(h.seed(),removed)==removedBytes,"repeat demolition does not free twice");
    auto wrongOwner=dispatched;auto f=decodeNativeInspector(wrongOwner.inspection->payloads.front(),error);need(bool(f),"local walker decode");
    f->owner=3999;const auto badPayload=encodeNativeInspector(*f,error);need(bool(badPayload),"codec is not an owner graph substitute");wrongOwner.inspection->payloads.front()=*badPayload;
    const auto wrongOwnerBefore=wrongOwner.inspection->payloads;const auto revision=wrongOwner.revision;
    const auto refused=h.command(wrongOwner,{NativeSessionAction::Tick,wrongOwner.revision},false);
    need(!refused.committed&&wrongOwner.revision==revision&&wrongOwner.inspection->payloads==wrongOwnerBefore,"wrong owner rejects atomically");
    auto activeFire=dispatched;activeFire.figureRuntime->services.fireCount=1;
    const auto fireBefore=wire(h.seed(),activeFire);const auto fireResult=h.command(activeFire,{NativeSessionAction::Tick,activeFire.revision},false);
    need(!fireResult.committed&&!fireResult.error.empty()&&wire(h.seed(),activeFire)==fireBefore,"unsupported active fire cannot publish partial phase");
    auto threshold=dispatched;auto risk=decodeNativeStrictRecord(threshold.strictRecords.at(towerId),error);need(bool(risk),"threshold source record");
    const auto& models=h.source.runtime->strictTemplate.emptyCity.buildingContext->buildingModels;
    need(models&&models->count(124),"threshold uses the loaded124 row");const int increment=models->at(124)[7];
    need(increment>0&&increment<1000,"loaded Inspector collapse increment is positive and bounded");
    risk->fireRisk=int16_t(1000-increment);threshold.strictRecords[towerId]=encodeNativeStrictRecord(*risk);stagePhase(threshold,44);
    const auto thresholdBefore=wire(h.seed(),threshold);const auto riskResult=h.command(threshold,{NativeSessionAction::Tick,threshold.revision},false);
    need(!riskResult.committed&&!riskResult.error.empty()&&wire(h.seed(),threshold)==thresholdBefore,"real loaded threshold branch rolls back phase and RNG");
    auto missingState=dispatched;missingState.inspection.reset();need(!validateNativeSession(h.seed(),missingState).empty(),"policy cannot silently lose Inspector state");
    auto missingTag=dispatched;missingTag.runtimePolicies.erase(std::remove_if(missingTag.runtimePolicies.begin(),missingTag.runtimePolicies.end(),[](const auto& p){return p.find("inspection")!=std::string::npos||p.find("inspector")!=std::string::npos;}),missingTag.runtimePolicies.end());
    need(!validateNativeSession(h.seed(),missingTag).empty(),"Inspector payload cannot fall back to legacy policy");
    const auto bytes=wire(h.seed(),dispatched);const auto endTag=bytes.rfind("ENDRUNTIME8");need(endTag!=std::string::npos,"required runtime ending exists");
    const auto noNewline=decodeNativeSession(h.seed(),bytes.substr(0,bytes.size()-1),error);need(noNewline&&wire(h.seed(),*noNewline)==bytes,"outer whitespace-only ending canonicalizes legally");
    need(!decodeNativeSession(h.seed(),bytes.substr(0,endTag),error),"substantive missing runtime end tag refuses");
    const auto inspectorPayload=dispatched.inspection->payloads.front();auto malformed=dispatched;malformed.inspection->payloads.front()=inspectorPayload.substr(0,inspectorPayload.size()/2);
    need(!validateNativeHouseRuntimeReady(h.seed(),malformed,*h.source.runtime).empty(),"partial39 payload not resource ready");
    auto oversized=dispatched;oversized.inspection->payloads.front()=std::string(16385,'x');need(!validateNativeSession(h.seed(),oversized).empty(),"oversized39 payload refuses before execution");
    const auto frozen=wire(h.seed(),dispatched);const auto q=queryNativeInspectorFeedback(dispatched,towerId);
    need(q.recordKnown&&q.tower&&wire(h.seed(),dispatched)==frozen,"feedback reads only committed records");
    std::cout<<"{\"authored_runtime_branches\":true,\"checks\":"<<checks<<",\"natural_or_original_equivalence_claim\":false}"<<std::endl;
}
void authoredRoadEdit(Harness& h,const NativeSessionState& source){
    auto state=source;h.action(state,NativeSessionAction::Pause);const auto live=inspectors(state);
    uint64_t road=0;
    for(const auto& [key,f]:live){(void)key;for(const auto& o:state.objects)
        if(o.kind==NativeObjectKind::Road&&int(o.x)==f.x&&int(o.y)==f.y)road=o.id;
        if(road)break;}
    need(road!=0,"road-edit fixture is a real current constructed road under an Inspector");
    NativeSessionCommand edit{NativeSessionAction::Demolish,state.revision};edit.objectId=road;
    const auto before=wire(h.seed(),state);const auto preview=previewNativeSessionCommand(h.seed(),state,edit,h.source.runtime.get());
    need(wire(h.seed(),state)==before,"road-edit preview pure");
    const auto changed=h.command(state,edit,false);need(changed.committed==preview.committed,"road-edit preview and command agree");
    if(!changed.committed){need(!changed.error.empty()&&changed.error==preview.error&&wire(h.seed(),state)==before,"stranding road-edit rejects atomically with same reason");}
    else{
        need(validateNativeHouseRuntimeReady(h.seed(),state,*h.source.runtime).empty(),"accepted road edit leaves admitted actor graph");
        h.action(state,NativeSessionAction::Resume);for(unsigned n=0;n<5;++n)h.action(state,NativeSessionAction::Tick);
    }
    std::cout<<"{\"authored_road_edit_committed\":"<<(changed.committed?"true":"false")<<",\"error\":"<<std::quoted(changed.error)<<",\"natural_or_original_equivalence_claim\":false}"<<std::endl;
}
}
int main(int argc,char**argv){try{
    authoredCodec();std::cout<<"{\"authored_codec_checks\":"<<checks<<",\"passed\":true}"<<std::endl;
    auto root=ResourceRoot::fromAbsolute(std::getenv("EMPEROR_GAMEDATA_ROOT")?std::getenv("EMPEROR_GAMEDATA_ROOT"):"");
    if(!root.configured()){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}"<<std::endl;return 0;}
    need(argc>2,"explicit source and new outside-data output required");const std::filesystem::path output(argv[2]);
    need(output.is_absolute()&&std::filesystem::is_directory(output),"existing test output directory");
    const unsigned maxSteps=argc>3?unsigned(std::stoul(argv[3])):10000;need(maxSteps>0&&maxSteps<=10000,"bounded natural loop");
    std::string error;auto src=loadNativeSessionSource(root,argv[1],error);need(bool(src),"source load: "+error);
    need(prepareNativeHouseRuntimeSource(root,*src,error),"source runtime: "+error);Harness h{root,*src,output};
    auto created=startNativeHouseRuntime(src->seed,*src->runtime,error,NativeWellReturnPolicy::ObservedHomeRoad,true,true,true,true,true,true);
    need(bool(created),"explicit inspection start: "+error);auto state=*created;
    need(nativeUsesInspection(state)&&nativeHouseRuntimeWireVersion(state)>6,"new explicit policy/wire");
    need(h.seed().inspectorCost>0&&h.seed().inspectorWorkers>0,"source-bound Inspector model input");
    const auto initial=wire(h.seed(),state);const auto legacy=startNativeHouseRuntime(src->seed,*src->runtime,error,NativeWellReturnPolicy::ObservedHomeRoad,true,true,true,true,true);
    need(legacy&&!nativeUsesInspection(*legacy)&&!legacy->inspection,"old runtime start does not gain policy");
    auto layout=chooseLayout(h.seed());auto tower=buildNaturalCity(h,state,layout);
    state=h.fileRoundtrip(state,"constructed");std::string preparation="fresh-final-source";
    if(argc>4){
        // Final-source construction and employment contracts still execute;
        // then reuse only a real, normally paused pre-first-dispatch input.
        const auto constructionContext=decodeNativeCityContext(*state.cityContext,error);need(bool(constructionContext),"fresh construction C1 decode");
        const auto beforeEmploymentCalls=constructionContext->employmentCalls;
        h.action(state,NativeSessionAction::Resume);bool employment=false;
        for(unsigned n=0;n<52;++n){const auto p=state.houseSchedule->phase;h.action(state,NativeSessionAction::Tick);if(p==25){employment=true;break;}}
        need(employment&&state.populationRuntime&&state.cityContext,"final source fresh-city phase25 ran");
        auto context=decodeNativeCityContext(*state.cityContext,error);need(context&&context->workers.count(size_t(tower)),"new tower has phase25-owned job key");
        need(context->employmentCalls==beforeEmploymentCalls+1,"fresh final-source phase25 executes employment exactly once");
        const auto freshRecord=decodeNativeStrictRecord(state.strictRecords.at(tower),error);need(freshRecord&&context->workers.at(size_t(tower))==freshRecord->workers,"fresh phase25 job mirror matches authoritative strict worker count");
        need(nativePopulation(state)==0&&freshRecord->workers==0,"empty fresh city cannot claim staffed coverage");
        std::cout<<"{\"fresh_phase25\":true,\"workers\":"<<freshRecord->workers<<",\"row3_demand\":"<<state.populationRuntime->employment.rows[3].demand<<",\"row3_preferred\":"<<state.populationRuntime->employment.rows[3].preferred<<",\"employment_calls\":"<<context->employmentCalls<<"}"<<std::endl;
        h.action(state,NativeSessionAction::Pause);h.fileRoundtrip(state,"fresh-construction-employment");
        const auto saved=readNativeSessionFile(argv[4],root.configuredDirectory(),error);need(bool(saved),"explicit real preparation input: "+error);
        const auto reference=nativeSessionReference(*saved);need(reference&&reference->source==h.seed().source&&reference->version==8,"preparation input is bound to same original source");
        const auto loaded=decodeNativeSession(h.seed(),*saved,error);need(loaded&&loaded->paused&&nativeUsesInspection(*loaded),"preparation is a normal paused inspection archive");
        need(loaded->inspection->payloads.empty()&&loaded->inspection->dispatches==0&&loaded->inspection->returns==0&&loaded->inspection->services==0,"preparation cannot supply any patrol episode");
        need(validateNativeHouseRuntimeReady(h.seed(),*loaded,*h.source.runtime).empty(),"preparation fully admitted by final source");state=*loaded;
        auto object=std::find_if(state.objects.begin(),state.objects.end(),[](const auto& o){return o.kind==NativeObjectKind::Inspector;});
        need(object!=state.objects.end()&&std::count_if(state.objects.begin(),state.objects.end(),[](const auto& o){return o.kind==NativeObjectKind::Inspector;})==1,"preparation owns exactly one real Tower");
        tower=object->id;layout.x=int(object->x)-9;layout.y=int(object->y)-4;preparation=argv[4];state=h.fileRoundtrip(state,"resumed-preparation");
        std::cout<<"{\"prepared_input\":"<<std::quoted(preparation)<<",\"prepared_tick\":"<<state.ticks<<",\"fresh_final_construction_and_employment_checked\":true,\"prior_patrols\":0}"<<std::endl;
    }
    const auto naturalStartTick=state.ticks;h.action(state,NativeSessionAction::Resume);
    std::map<Key,Episode> episodes;std::map<size_t,uint64_t> lastVisit;std::set<unsigned> restoredPhases,restoredActions;
    size_t restores=0,naturalSteps=0;std::optional<NativeSessionState> firstDispatch,roadEditState;size_t completed=0,reducedCompleted=0;bool thirdDispatch=false,sequentialCompletion=false,restoredDecayWrap=false;
    for(;naturalSteps<maxSteps;++naturalSteps){
        const auto prior=inspectors(state);const unsigned phase=state.houseSchedule->phase;bool checkpoint=false;
        if((phase==25||phase==31||phase==40||phase==43||phase==44||phase==45||phase==50)&&!restoredPhases.count(phase)){restoredPhases.insert(phase);checkpoint=true;}
        if(phase==45&&state.inspection->decayCounter==7&&!restoredDecayWrap){restoredDecayWrap=true;checkpoint=true;}
        for(const auto& [key,f]:prior){(void)key;if(!restoredActions.count(f.action)){restoredActions.insert(f.action);checkpoint=true;}}
        std::optional<NativeSessionState> independent;if(checkpoint){independent=h.fileRoundtrip(state,"resume-"+std::to_string(restores++));h.action(*independent,NativeSessionAction::Tick);}
        const auto before=wire(h.seed(),state);const auto r=h.command(state,{NativeSessionAction::Tick,state.revision},false);
        if(!r.committed){
            need(wire(h.seed(),state)==before,"natural failure must roll back whole state");
            h.fileRoundtrip(state,"failure-state");
            throw std::runtime_error("natural tick "+std::to_string(naturalSteps)+" at phase "+std::to_string(phase)+": "+r.error);
        }
        if(independent)need(wire(h.seed(),state)==wire(h.seed(),*independent),"same-source next update after real file restore");
        const auto current=inspectors(state);
        for(const auto& [key,f]:current){auto& episode=episodes[key];
            if(episode.firstTick==0){episode.slot=f.slot;episode.serial=f.allocationSerial;episode.owner=size_t(f.owner);episode.firstTick=state.ticks;}
            episode.lastTick=state.ticks;episode.actions.insert(f.action);episode.walked|=f.action==8&&f.roamSteps>0;
            if(!roadEditState&&f.action==8)for(const auto& o:state.objects)
                if(o.kind==NativeObjectKind::Road&&int(o.x)==f.x&&int(o.y)==f.y){roadEditState=state;break;}
        }
        for(const auto& [target,v]:state.inspection->visits)if(v.tick>lastVisit[target]){
            lastVisit[target]=v.tick;need(v.tick<=state.ticks,"visit timestamp is committed native time");
            if(v.collapseAfter<v.collapseBefore||v.ignitionAfter<v.ignitionBefore){
                bool found=false;for(const auto& [key,f]:current)if(f.slot==v.figure){episodes[key].reduced=true;found=true;}
                if(!found)for(const auto& [key,f]:prior)if(f.slot==v.figure)episodes[key].reduced=true;
            }
        }
        for(const auto& [key,f]:prior)if(!current.count(key)){auto& e=episodes.at(key);e.completed=true;e.lastTick=state.ticks;
            need(f.owner==int(tower),"only owned Inspector contributes rounds");
        }
        if(!firstDispatch&&!current.empty()){
            firstDispatch=state;
            auto pre=decodeNativeSession(h.seed(),before,error);need(bool(pre),"real pre-first-dispatch snapshot decodes");
            need(pre->inspection->payloads.empty()&&pre->inspection->dispatches==0,"real checkpoint precedes first dispatch");
            h.action(*pre,NativeSessionAction::Pause);auto frozenPre=h.fileRoundtrip(*pre,"pre-first-patrol");
            need(frozenPre.paused&&frozenPre.houseSchedule->phase==phase&&frozenPre.ticks+1==state.ticks,"Pause preserves actual pre-allocation phase and tick");
            auto resumed=frozenPre;h.action(resumed,NativeSessionAction::Resume);h.action(resumed,NativeSessionAction::Tick);
            need(nativeSessionContent(h.seed(),resumed)==nativeSessionContent(h.seed(),state),"normal Pause/Resume checkpoint reproduces same first39 next tick");
            std::ofstream metadata(output/"pre-first-patrol.json");
            metadata<<"{\"source\":"<<std::quoted(h.seed().source)<<",\"checkpoint\":"<<std::quoted((output/"pre-first-patrol.encity").string())<<",\"paused_by_normal_command\":true,\"phase\":"<<unsigned(frozenPre.houseSchedule->phase)<<",\"ticks\":"<<frozenPre.ticks<<",\"revision\":"<<frozenPre.revision<<",\"tower_id\":"<<tower<<",\"tower_x\":"<<layout.x+9<<",\"tower_y\":"<<layout.y+4<<",\"dispatches\":0,\"expected_next_tick_first_dispatch\":true,\"matches_uninterrupted_first_dispatch\":true}\n";
            need(bool(metadata),"checkpoint metadata written");
        }
        completed=0;reducedCompleted=0;for(const auto& [key,e]:episodes){(void)key;if(e.completed&&e.owner==tower&&e.walked){++completed;if(e.reduced)++reducedCompleted;}}
        uint64_t firstReturn=UINT64_MAX;for(const auto& [key,e]:episodes){(void)key;if(e.completed&&e.owner==tower&&e.walked&&e.reduced)firstReturn=std::min(firstReturn,e.lastTick);}
        sequentialCompletion=false;for(const auto& [key,e]:episodes){(void)key;if(e.completed&&e.owner==tower&&e.walked&&e.reduced&&e.firstTick>firstReturn)sequentialCompletion=true;}
        thirdDispatch=episodes.size()>=3;
        if(naturalSteps%500==0)std::cout<<"{\"progress_ticks\":"<<state.ticks<<",\"phase\":"<<unsigned(state.houseSchedule->phase)<<",\"population\":"<<nativePopulation(state)<<",\"dispatches\":"<<state.inspection->dispatches<<",\"returns\":"<<state.inspection->returns<<",\"completed_natural_rounds\":"<<completed<<",\"reduced_rounds\":"<<reducedCompleted<<"}"<<std::endl;
        if(completed>=2&&reducedCompleted>=2&&thirdDispatch&&sequentialCompletion)break;
    }
    h.fileRoundtrip(state,"natural-observation-end");
    need(naturalSteps<maxSteps&&completed>=2&&reducedCompleted>=2&&thirdDispatch&&sequentialCompletion,"two sequential natural full patrol/reduction/return rounds and re-dispatch required");
    need(firstDispatch.has_value()&&nativePopulation(state)>0,"natural city supplied residents and Inspector");
    need(state.inspection->returns>=2&&state.inspection->dispatches>=3&&state.inspection->services>0,"runtime receipts agree with natural proof");
    need(restoredActions.count(6)&&restoredActions.count(7)&&restoredActions.count(8)&&restoredActions.count(9),"real file restores cover all Inspector actions");
    need(restoredDecayWrap,"actual file restoration covers eighth service-grid decay boundary");
    const auto frozen=wire(h.seed(),state);const auto q=queryNativeInspectorFeedback(state,tower);const auto feedback=queryNativeCityFeedback(h.seed(),state);
    need(q.recordKnown&&q.tower&&feedback.error.empty()&&wire(h.seed(),state)==frozen,"queries are pure during live patrol");
    naturalStaffing(h,*firstDispatch,tower);authoredBranches(h,*firstDispatch,tower);
    need(roadEditState.has_value(),"actual patrol produced an occupied-road edit point");authoredRoadEdit(h,*roadEditState);
    h.action(state,NativeSessionAction::Pause);state=h.fileRoundtrip(state,"natural-final");
    const auto still=wire(h.seed(),state);const auto paused=h.command(state,{NativeSessionAction::Tick,state.revision},false);
    need(!paused.committed&&wire(h.seed(),state)==still,"paused Tick does not patrol");
    std::cout<<"{\"source\":"<<std::quoted(h.seed().source)<<",\"passed\":true,\"checks\":"<<checks<<",\"preparation_input\":"<<std::quoted(preparation)<<",\"natural_start_tick\":"<<naturalStartTick<<",\"final_tick\":"<<state.ticks<<",\"natural_steps\":"<<naturalSteps+1<<",\"population\":"<<nativePopulation(state)<<",\"tower_id\":"<<tower<<",\"layout_x\":"<<layout.x<<",\"layout_y\":"<<layout.y<<",\"natural_rounds\":"<<completed<<",\"risk_reducing_rounds\":"<<reducedCompleted<<",\"sequential_completion\":"<<(sequentialCompletion?"true":"false")<<",\"allocations_observed\":"<<episodes.size()<<",\"file_restores\":"<<h.files<<",\"dispatches\":"<<state.inspection->dispatches<<",\"returns\":"<<state.inspection->returns<<",\"services\":"<<state.inspection->services<<",\"initial_bytes\":"<<initial.size()<<",\"final_bytes\":"<<wire(h.seed(),state).size()<<",\"GUI_executed\":false,\"original_game_executed\":false}"<<std::endl;
    return 0;
}catch(const std::exception& e){std::cerr<<"FAIL:"<<e.what()<<std::endl;return 3;}}
