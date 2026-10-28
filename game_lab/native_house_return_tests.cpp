// SPDX-License-Identifier: MIT
// Native observed-home policy regression; original inputs remain read-only.
#include "native_house_runtime_source.hpp"
#include "native_session_io.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
using namespace game_lab;
namespace {
size_t checks=0,checkpointCount=0,waterWrites=0,decays=0,steps=0;
void need(bool okay,const std::string& why){++checks;if(!okay)throw std::runtime_error(why);}
struct Test {
 const ResourceRoot& root;const NativeSessionSource& source;std::filesystem::path output;
 const NativeSessionSeed& seed()const{return source.seed;}
 OriginalFigure walker(const NativeSessionState& state)const{need(state.figureRuntime&&state.figureRuntime->wellWalkerPayloads.size()==1,"one live Well walker required");std::string e;auto w=decodeNativeWellWalker(state.figureRuntime->wellWalkerPayloads.front(),e);need(bool(w),"walker decode:"+e);return *w;}
 void put(NativeSessionState& state,const OriginalFigure& w)const{std::string e;auto p=encodeNativeWellWalker(w,e);need(bool(p),"staged walker encodes:"+e);state.figureRuntime->wellWalkerPayloads={*p};}
 NativeStrictConversion convert(const NativeSessionState& state)const{auto c=nativeToStrict(seed(),state,source.runtime->strictTemplate,nativeStrictPersisted(state));need(bool(c.city),"bridge:"+c.refusal);return c;}
 void tick(NativeSessionState& state)const{
  std::map<uint64_t,std::array<uint8_t,2>> before;for(const auto& o:state.objects)if(o.house)before[o.id]={o.house->remaining[45],o.house->remaining[47]};
  const auto phase=state.houseSchedule->phase;auto r=transactNativeHouseRuntime(seed(),state,{NativeSessionAction::Tick,state.revision},*source.runtime);
  need(r.committed,"tick phase="+std::to_string(phase)+" "+r.error);++steps;
  for(const auto& o:state.objects)if(o.house&&before.count(o.id))for(size_t n=0;n<2;++n){auto v=o.house->remaining[n?47:45];auto old=before.at(o.id)[n];waterWrites+=v==96&&v>old;decays+=v<old;}
 }
 void checkpoint(const NativeSessionState& state,const std::string& label)const{
  std::string e;const auto bytes=encodeNativeSession(seed(),state);need(!bytes.empty(),"checkpoint encodes:"+label);
  const auto path=(output/(std::to_string(checkpointCount++)+"-"+label+".encity")).string();
  need(writeNativeSessionNew(path,root.configuredDirectory(),bytes,e),"checkpoint new file:"+e);
  const auto disk=readNativeSessionFile(path,root.configuredDirectory(),e);need(disk&&*disk==bytes,"checkpoint file bytes:"+label);
  auto reloaded=decodeNativeSession(seed(),*disk,e);need(reloaded&&encodeNativeSession(seed(),*reloaded)==bytes,"checkpoint decode:"+label+":"+e);
  auto continuous=state;const NativeSessionCommand cmd{NativeSessionAction::Tick,state.revision};
  const auto a=transactNativeHouseRuntime(seed(),continuous,cmd,*source.runtime),b=transactNativeHouseRuntime(seed(),*reloaded,cmd,*source.runtime);
  need(a.committed&&b.committed,"checkpoint next step:"+label+":"+a.error+":"+b.error);
  need(encodeNativeSession(seed(),continuous)==encodeNativeSession(seed(),*reloaded),"checkpoint resumed next bytes:"+label);
 }
 void clean(const NativeSessionState& state,const OriginalFigure& old)const{
  const auto owner=std::find_if(state.objects.begin(),state.objects.end(),[&](const auto& o){return o.id==uint64_t(old.owner);});
  need(owner!=state.objects.end()&&owner->well&&owner->well->primaryFigure==0,"arrival clears owner primary");
  need(state.figureRuntime->wellWalkerPayloads.empty(),"arrival removes live payload");
  const auto& free=state.figureRuntime->freeFigureSlots;need(free.size()==1999&&std::count(free.begin(),free.end(),uint16_t(old.slot))==1,"arrival appends one FIFO identity");
  for(auto id:state.figureRuntime->cellHeads)need(id!=old.slot,"arrival removes cell head reference");
  for(auto id:state.figureRuntime->services.routeOwners)need(id!=int(old.slot),"arrival releases every route owner");
  need(state.heldWalkerReasons.empty(),"observed successful return not held");
 }
};
void selectObserved(NativeSessionState& s){s.runtimePolicies.erase(std::remove(s.runtimePolicies.begin(),s.runtimePolicies.end(),nativeWellReturnHoldTag),s.runtimePolicies.end());nativeRuntimePolicy(s,nativeWellReturnObservedTag);}
void resume(const Test& t,NativeSessionState& s){if(s.paused){auto r=transactNativeHouseRuntime(t.seed(),s,{NativeSessionAction::Resume,s.revision},*t.source.runtime);need(r.committed,"resume:"+r.error);}}
void local(const Test& t,const NativeSessionState& saved){
 std::string e;auto freshHold=startNativeHouseRuntime(t.seed(),*t.source.runtime,e);need(freshHold&&nativeSavedWellReturnPolicy(*freshHold,e)==NativeWellReturnPolicy::Hold,"constructor defaults Hold");
 auto freshObserved=startNativeHouseRuntime(t.seed(),*t.source.runtime,e,NativeWellReturnPolicy::ObservedHomeRoad);need(freshObserved&&nativeSavedWellReturnPolicy(*freshObserved,e)==NativeWellReturnPolicy::ObservedHomeRoad,"constructor explicitly Observed");
 auto untagged=saved;untagged.runtimePolicies.erase(std::remove_if(untagged.runtimePolicies.begin(),untagged.runtimePolicies.end(),[](const auto& p){return p==nativeWellReturnHoldTag||p==nativeWellReturnObservedTag||p==nativeWellReturnTransitionTag;}),untagged.runtimePolicies.end());
 need(nativeSavedWellReturnPolicy(untagged,e)==NativeWellReturnPolicy::Hold,"old untagged v8 stays Hold");
 auto base=untagged;selectObserved(base);resume(t,base);
 for(unsigned n=0;n<400;++n){const auto w=t.walker(base);if(w.action==1&&w.route==0&&(w.x!=w.homeX||w.y!=w.homeY))break;t.tick(base);}
 const auto original=t.walker(base);need(original.action==1&&original.route==0&&(original.x!=original.homeX||original.y!=original.homeY),"real roaming fixture away from home");
 auto boundary=original;boundary.roamSteps=boundary.roamLimit;
 auto hold=base;hold.runtimePolicies.erase(std::remove(hold.runtimePolicies.begin(),hold.runtimePolicies.end(),nativeWellReturnObservedTag),hold.runtimePolicies.end());t.put(hold,boundary);t.tick(hold);need(hold.heldWalkerReasons.count(boundary.slot)&&hold.heldWalkerReasons.at(boundary.slot)=="well_walker_return_destination_unported","Hold remains named boundary isolation");
 const auto frozen=hold.figureRuntime->wellWalkerPayloads;t.tick(hold);need(hold.figureRuntime->wellWalkerPayloads==frozen,"Hold never silently returns or respawns");
 auto strict=t.convert(base);auto context=strict.city->figures;context.wellReturnObservedHomePolicy=true;
 auto refs=t.source.runtime->archiveRefs();context.figures.at(boundary.slot)=boundary;
 need(originalFigureObservedWellReturnBlocker(boundary,context).empty(),"observed guard admitted for common home road");
 const auto first=originalFigureOrderedNextPass(context,refs);need(first.complete&&first.context,"threshold transition computes");
 const auto& transitioned=first.context->figures.at(boundary.slot);
 need(transitioned.action==2&&transitioned.destinationX==boundary.homeX&&transitioned.destinationY==boundary.homeY&&transitioned.roamSteps==0&&transitioned.route==0,"threshold enters action2 with shared home and no route");
 need(transitioned.x==boundary.x&&transitioned.y==boundary.y&&transitioned.cell==boundary.cell&&transitioned.subX==boundary.subX&&transitioned.subY==boundary.subY&&transitioned.destinationCell==boundary.destinationCell,"transition frame does not move position or destinationCell");
 need(transitioned.frame==uint8_t((unsigned(boundary.frame)+1)%12),"transition advances ordinary frame");
 const auto second=originalFigureOrderedNextPass(*first.context,refs);need(second.complete&&second.context,"next frame follows route");const auto& routed=second.context->figures.at(boundary.slot);
 need(routed.route>0&&second.context->services->routeOwners.at(size_t(routed.route))==int(boundary.slot),"next frame allocates owned FollowRoute");
 auto originContext=context;auto origin=boundary;const auto homeCell=*originContext.cropBase+uint32_t(origin.homeX)+228u*uint32_t(origin.homeY);
 need(originalFigureRelink(origin,homeCell,originContext),"origin branch relink uses normal cell helper");origin.x=origin.homeX;origin.y=origin.homeY;origin.roamSteps=int16_t((int(origin.roamLimit)*4+4)/5);originContext.figures.at(origin.slot)=origin;
 need(origin.roamSteps<origin.roamLimit,"4/5 test is distinct from full limit");const auto four=originalFigureOrderedNextPass(originContext,refs);need(four.complete&&four.context,"4/5 origin branch computes");
 const auto& o=four.context->figures.at(origin.slot);need(o.action==2&&o.destinationX==origin.homeX&&o.destinationY==origin.homeY&&o.roamSteps==0&&o.route==0&&o.x==origin.x&&o.y==origin.y&&o.cell==origin.cell&&o.frame==uint8_t((unsigned(origin.frame)+1)%12),"4/5 origin transition has same nonmoving contract");
 auto negative=[&](OriginalFigure f,OriginalFigureUpdateContext c,const std::string& reason){need(originalFigureObservedWellReturnBlocker(f,c)==reason,"guard refusal:"+reason);};
 {auto f=boundary;auto c=context;f.route=1;c.services->routeOwners[1]=int16_t(f.slot);negative(f,c,"existing_route_unproven");}
 {auto f=boundary;++f.homeX;negative(f,context,"home_origin_owner_road_differ");}
 {auto f=boundary;++f.roamOriginY;negative(f,context,"home_origin_owner_road_differ");}
 {auto c=context;++c.buildings->at(size_t(boundary.owner)).roadX;negative(boundary,c,"home_origin_owner_road_differ");}
 {auto c=context;c.terrain.at(homeCell)&=~uint32_t(0x40);negative(boundary,c,"home_road_missing");}
 {auto f=routed;f.route=-1;need(originalFigureObservedWellReturnBlocker(f,*second.context,true)=="return_route_invalid","in-flight negative route never becomes implicit zero");}
 {auto f=boundary;f.x=-1;negative(f,context,"current_position_invalid");}
 {auto f=boundary;++f.cell;negative(f,context,"current_position_invalid");}
 {auto c=context;c.terrain.at(boundary.cell)&=~uint32_t(0x40);negative(boundary,c,"current_road_missing");}
 {auto f=boundary;f.slot=0;negative(f,context,"walker_identity_invalid");}
 {auto city=*strict.city;city.figures.buildings->at(size_t(boundary.owner)).primaryFigure=int(boundary.slot)-1;const auto before=native_bridge_detail::fingerprint(city,strict.isolation);NativePhaseStepInputs in;in.archives=&refs;in.wellReturnPolicy=NativeWellReturnPolicy::ObservedHomeRoad;auto r=stepNativeHousePhase(city,strict.isolation,in);need(!r.candidate&&r.refusal.find("well_walker_owner_primary_")==0,"invalid graph is atomic refusal, not a hold");need(before==native_bridge_detail::fingerprint(city,strict.isolation),"graph rejection preserves full input");}
 for(int kind=0;kind<3;++kind){auto bad=base;std::string expected;
  if(kind==0){nativeRuntimePolicy(bad,"native_well_return_policy_future_v99");expected="well_return_policy_version_unsupported";}
  if(kind==1){nativeRuntimePolicy(bad,nativeWellReturnHoldTag);expected="well_return_policy_conflict";}
  if(kind==2){bad.runtimePolicies.erase(std::remove(bad.runtimePolicies.begin(),bad.runtimePolicies.end(),nativeWellReturnObservedTag),bad.runtimePolicies.end());nativeRuntimePolicy(bad,nativeWellReturnTransitionTag);expected="well_return_policy_missing_for_saved_transition";}
  const auto before=encodeNativeSession(t.seed(),bad);auto r=transactNativeHouseRuntime(t.seed(),bad,{NativeSessionAction::Tick,bad.revision},*t.source.runtime);need(!r.committed&&r.error==expected,"saved policy refusal:"+expected);need(encodeNativeSession(t.seed(),bad)==before,"policy refusal atomic");}
 std::cout<<"{\"local_guards_and_transitions\":true,\"checks\":"<<checks<<"}"<<std::endl;
 // Threshold staging does not alter the real model, speed or roamLimit.
 auto city=base;
 for(unsigned round=0;round<2;++round){
  for(unsigned n=0;n<400;++n){const auto w=t.walker(city);if(w.action==1&&(w.x!=w.homeX||w.y!=w.homeY))break;t.tick(city);}
  auto w=t.walker(city);need(w.action==1&&w.route==0&&(w.x!=w.homeX||w.y!=w.homeY),"staged round has away roamer");w.roamSteps=w.roamLimit;t.put(city,w);t.checkpoint(city,"before-return-"+std::to_string(round));
  t.tick(city);auto returning=t.walker(city);need(returning.action==2&&returning.route==0,"wrapper enters return");need(std::find(city.runtimePolicies.begin(),city.runtimePolicies.end(),nativeWellReturnTransitionTag)!=city.runtimePolicies.end(),"transition policy persisted");t.checkpoint(city,"after-transition-"+std::to_string(round));
  t.tick(city);returning=t.walker(city);need(returning.route>0&&city.figureRuntime->services.routeOwners.at(size_t(returning.route))==int(returning.slot),"wrapper follows owned route next tick");t.checkpoint(city,"in-flight-"+std::to_string(round));
  unsigned travel=0;while(!city.figureRuntime->wellWalkerPayloads.empty()&&travel++<600){t.tick(city);need(city.heldWalkerReasons.empty(),"guarded valid return is never isolated");}
  need(travel<=600&&city.figureRuntime->wellWalkerPayloads.empty(),"actual return arrives within local bound");t.clean(city,w);t.checkpoint(city,"arrival-"+std::to_string(round));
  unsigned wait=0;uint8_t phase=0;while(city.figureRuntime->wellWalkerPayloads.empty()&&wait++<2000){phase=city.houseSchedule->phase;t.tick(city);}
  need(!city.figureRuntime->wellWalkerPayloads.empty()&&phase==31,"actual phase31 redispatch follows primary clear");need(t.walker(city).owner==w.owner,"redispatch belongs to original Well");t.checkpoint(city,"redispatch-"+std::to_string(round));
  std::cout<<"{\"staged_completed_round\":"<<round+1<<",\"travel_steps\":"<<travel<<",\"redispatch_wait\":"<<wait<<",\"steps\":"<<steps<<"}"<<std::endl;
 }
 need(waterWrites>0&&decays>0,"staged rounds contain actual service writes and decay");
}
void soak(const Test& t,NativeSessionState city,unsigned limit){
 selectObserved(city);resume(t,city);const auto baseline=t.walker(city);const auto fixedLimit=baseline.roamLimit;size_t transitions=0,arrivals=0,dispatches=0,writesAfterDispatch=0;bool savedBefore=false,savedFlight=false;
 t.checkpoint(city,"natural-start");
 for(unsigned n=0;n<limit;++n){
  const auto priorPayload=city.figureRuntime->wellWalkerPayloads;std::optional<OriginalFigure> prior;if(!priorPayload.empty())prior=t.walker(city);
  if(prior){need(prior->roamLimit==fixedLimit,"natural mode never changes real roamLimit");const bool boundary=prior->action==1&&(prior->roamSteps>=prior->roamLimit||(prior->roamSteps>=int(prior->roamLimit)*4/5&&prior->x==prior->roamOriginX&&prior->y==prior->roamOriginY));if(boundary&&!savedBefore){t.checkpoint(city,"natural-before-"+std::to_string(transitions));savedBefore=true;}}
  const auto phase=city.houseSchedule->phase;const auto oldWrites=waterWrites;t.tick(city);if(dispatches)writesAfterDispatch+=waterWrites-oldWrites;need(city.heldWalkerReasons.empty(),"natural supported trajectory remains unheld");
  std::optional<OriginalFigure> next;if(!city.figureRuntime->wellWalkerPayloads.empty())next=t.walker(city);
  if(prior&&next&&prior->action==1&&next->action==2){++transitions;savedFlight=false;t.checkpoint(city,"natural-transition-"+std::to_string(transitions));}
  if(next&&next->action==2&&next->route>0&&!savedFlight){t.checkpoint(city,"natural-flight-"+std::to_string(transitions));savedFlight=true;}
  if(prior&&!next){need(prior->action==2,"natural removal is an actual return");++arrivals;t.clean(city,*prior);t.checkpoint(city,"natural-arrival-"+std::to_string(arrivals));}
  if(!prior&&next){need(phase==31&&next->owner==baseline.owner,"natural redispatch phase and owner");++dispatches;savedBefore=false;t.checkpoint(city,"natural-dispatch-"+std::to_string(dispatches));}
  if((n+1)%500==0)std::cout<<"{\"natural_steps\":"<<n+1<<",\"returns\":"<<arrivals<<",\"redispatches\":"<<dispatches<<",\"fresh_water_writes\":"<<waterWrites<<",\"decays\":"<<decays<<"}"<<std::endl;
  if(arrivals>=2&&dispatches>=2&&transitions>=2&&writesAfterDispatch>0&&decays>0){std::cout<<"{\"natural_completed\":true,\"steps\":"<<n+1<<",\"returns\":"<<arrivals<<",\"redispatches\":"<<dispatches<<",\"writes_after_redispatch\":"<<writesAfterDispatch<<",\"real_roam_limit\":"<<fixedLimit<<"}"<<std::endl;return;}
 }
 need(false,"natural bound exhausted without two return/cleanup/redispatch cycles plus fresh service and decay");
}
}
int main(int argc,char** argv){try{
 auto root=ResourceRoot::fromAbsolute(std::getenv("EMPEROR_GAMEDATA_ROOT")?std::getenv("EMPEROR_GAMEDATA_ROOT"):"");if(!root.configured()){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}"<<std::endl;return 0;}
 need(argc>=5,"usage: source checkpoint output local|soak [max-steps]");const std::filesystem::path out(argv[3]);need(out.is_absolute()&&std::filesystem::is_directory(out),"existing absolute output directory");std::string e;
 auto source=loadNativeSessionSource(root,argv[1],e);need(bool(source),"load source:"+e);need(prepareNativeHouseRuntimeSource(root,*source,e),"runtime source:"+e);
 auto bytes=readNativeSessionFile(argv[2],root.configuredDirectory(),e);need(bool(bytes),"read frozen700-step checkpoint:"+e);auto state=decodeNativeSession(source->seed,*bytes,e);need(bool(state),"decode checkpoint:"+e);need(state->ticks==700,"reuse frozen700-step fixture, never rebuild it");
 Test test{root,*source,out};std::cout<<"{\"loaded\":true,\"mode\":"<<std::quoted(argv[4])<<",\"ticks\":"<<state->ticks<<",\"walker_count\":"<<state->figureRuntime->wellWalkerPayloads.size()<<"}"<<std::endl;
 if(std::string(argv[4])=="local")local(test,*state);else{need(std::string(argv[4])=="soak","known test mode");unsigned limit=argc>5?unsigned(std::stoul(argv[5])):10000;need(limit>0&&limit<=10000,"natural bound1..10000");soak(test,*state,limit);}
 std::cout<<"{\"source\":"<<std::quoted(source->seed.source)<<",\"checks\":"<<checks<<",\"steps\":"<<steps<<",\"checkpoints\":"<<checkpointCount<<",\"water_writes\":"<<waterWrites<<",\"decays\":"<<decays<<",\"passed\":true}"<<std::endl;return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL:"<<e.what()<<std::endl;return 1;}}
