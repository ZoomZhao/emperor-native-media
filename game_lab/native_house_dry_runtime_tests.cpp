// SPDX-License-Identifier: MIT
// Real source/model integration with EXPLICIT SYNTHETIC native stage fixtures.
// Does not import original inhabited saves or claim observed original progress.
#include "native_house_runtime_source.hpp"
#include "native_session_io.hpp"
#include <chrono>
#include <filesystem>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <stdexcept>
using namespace game_lab;
namespace {
size_t checks=0,steps=0,replays=0;
void need(bool yes,const std::string& why){++checks;if(!yes)throw std::runtime_error(why);}
struct Test {
 const NativeSessionSource& src;const ResourceRoot& root;std::filesystem::path output;uint64_t house=0;
 const NativeSessionSeed& seed()const{return src.seed;}
 NativeSessionObject& home(NativeSessionState& s)const{for(auto& o:s.objects)if(o.id==house)return o;throw std::runtime_error("house missing");}
 const NativeSessionObject& home(const NativeSessionState& s)const{for(const auto& o:s.objects)if(o.id==house)return o;throw std::runtime_error("house missing");}
 OriginalBuildingUpdateInputs record(const NativeSessionState& s)const{std::string e;auto i=decodeNativeStrictRecord(s.strictRecords.at(house),e);need(bool(i),"record decode:"+e);return *i;}
 NativeStrictConversion strict(const NativeSessionState& s)const{auto c=nativeToStrict(seed(),s,src.runtime->strictTemplate,nativeStrictPersisted(s));need(bool(c.city),"bridge:"+c.refusal);return c;}
 void action(NativeSessionState& s,NativeSessionAction a,int x=0,int y=0)const{const unsigned phase=s.houseSchedule->phase;const auto r=transactNativeHouseRuntime(seed(),s,{a,s.revision,x,y},*src.runtime);need(r.committed,"action "+std::to_string(int(a))+" phase="+std::to_string(phase)+":"+r.error);if(a==NativeSessionAction::Tick)++steps;}
 void tick(NativeSessionState& s)const{action(s,NativeSessionAction::Tick);}
 void replay(const NativeSessionState& s,const std::string& label)const{
  std::string e;const auto bytes=encodeNativeSession(seed(),s);need(!bytes.empty(),"encode:"+label);
  auto restored=decodeNativeSession(seed(),bytes,e);need(restored&&encodeNativeSession(seed(),*restored)==bytes,"exact restore:"+label+":"+e);
  auto continuous=s;const auto a=transactNativeHouseRuntime(seed(),continuous,{NativeSessionAction::Tick,s.revision},*src.runtime);
  const auto b=transactNativeHouseRuntime(seed(),*restored,{NativeSessionAction::Tick,s.revision},*src.runtime);
  need(a.committed&&b.committed,"replay next:"+label+":"+a.error+":"+b.error);
  need(encodeNativeSession(seed(),continuous)==encodeNativeSession(seed(),*restored),"next tick byte equality:"+label);++replays;
 }
 void saveStage(const NativeSessionState& state,const std::string& label)const{
  if(output.empty())return;auto paused=state;if(!paused.paused)action(paused,NativeSessionAction::Pause);
  const auto bytes=encodeNativeSession(seed(),paused);need(!bytes.empty(),"paused stage encodes:"+label);
  const auto stamp=std::chrono::system_clock::now().time_since_epoch().count();const auto path=(output/(label+"-"+std::to_string(stamp)+".encity")).string();std::string e;
  need(writeNativeSessionNew(path,root.configuredDirectory(),bytes,e),"new independent stage:"+e);
  std::cout<<"{\"stage_file\":\""<<path<<"\",\"label\":\""<<label<<"\",\"synthetic_runtime_stage\":true,\"paused\":true}"<<std::endl;
 }
 OriginalFigure migrant(const NativeSessionState& s)const{need(s.figureRuntime->migrantPayloads.size()==1,"one migrant");std::string e;auto f=decodeNativeMigrant(s.figureRuntime->migrantPayloads.front(),e);need(bool(f),"migrant decode:"+e);return *f;}
 void canonicalize(NativeSessionState& s)const{const auto c=strict(s);const auto p=strictToNative(seed(),s,*c.city,c.isolation,src.runtime->strictTemplate);need(bool(p.state),"fixture canonicalization:"+p.refusal);s=*p.state;}
};
// Find one 2x2 native legal footprint with the already-admitted odd type4
// variant. Build its road through ordinary commands before synthetic staging.
std::pair<int,int> choose(const NativeSessionSource& src,std::vector<int>& path){
 const auto& seed=src.seed;const int side=int(seed.side),origin=(228-side)/2;
 const auto& random=src.save.layers.ground->state.randomByte;
 for(int y=1;y+2<side;++y)for(int x=1;x+2<side;++x){bool okay=true;
  for(int dy=0;dy<2;++dy)for(int dx=0;dx<2;++dx)okay=okay&&seed.buildable[size_t(y+dy)*side+x+dx];
  if(!okay||!(random[size_t(y+origin)*228+x+origin]&1))continue;
  const int start=(y-1)*side+x;if(!seed.buildable[start]&&!seed.sourceRoad[start])continue;
  std::vector<int> previous(size_t(side)*side,-1);std::deque<int> q{start};previous[start]=start;int end=-1;
  while(!q.empty()){const int at=q.front();q.pop_front();if(seed.sourceRoad[at]){end=at;break;}
   for(auto d:{std::pair<int,int>{1,0},{-1,0},{0,1},{0,-1}}){const int xx=at%side+d.first,yy=at/side+d.second;
    if(xx<0||yy<0||xx>=side||yy>=side||(xx>=x&&xx<x+2&&yy>=y&&yy<y+2))continue;
    const int next=yy*side+xx;if(previous[next]>=0||(!seed.buildable[next]&&!seed.sourceRoad[next]))continue;previous[next]=at;q.push_back(next);}}
  if(end<0)continue;for(int at=end;;at=previous[at]){path.push_back(at);if(at==start)break;}return {x,y};
 }
 throw std::runtime_error("no connected synthetic footprint");
}
NativeSessionState fixture(Test& t,bool observed){
 std::string e;auto initial=startNativeHouseRuntime(t.seed(),*t.src.runtime,e,NativeWellReturnPolicy::Hold,observed);need(bool(initial),"constructor:"+e);auto s=*initial;
 std::vector<int> road;const auto xy=choose(t.src,road);for(int at:road)if(!t.seed().sourceRoad[at])t.action(s,NativeSessionAction::Road,at%int(t.seed().side),at/int(t.seed().side));
 t.action(s,NativeSessionAction::House,xy.first,xy.second);t.house=s.objects.back().id;
 // This is an authored test stage, not a naturally grown original city. Its
 // population/ledger/schedule/strict-record mirrors are all explicitly bound.
 s.ticks=200;s.revision+=200;s.taxCycles=50;s.paused=false;s.lastFood.demand=14;
 auto& o=t.home(s);o.residents=14;o.house->type=4;o.house->model=1;o.house->remaining.fill(0);o.house->servicesKnown=true;o.house->houseWater=0;o.house->imageWord20=14;o.house->imageHouseByte3c=0;
 o.automation->cachedWater=0;o.automation->delay=0;
 auto i=t.record(s);i.model=1;i.residents=14;i.free=0;i.houseInUse=1;i.house=o.house->remaining;i.houseWater=0;i.modelDesirability=0;i.modelDesirabilityBonus=0;s.strictRecords[t.house]=encodeNativeStrictRecord(i);
 auto extra=decodeNativeStrictContext(*s.strictContext,e);need(bool(extra),"fixture context:"+e);extra->population=OriginalFigurePopulation{14,14,std::nullopt};
 if(observed){extra->nativePopulation=14;extra->laborCounters=std::make_pair(0,0);}s.strictContext=encodeNativeStrictContext(*extra);
 s.houseSchedule->phase=39;s.houseSchedule->cycle=178;s.houseSchedule->monthPart=3;s.houseSchedule->month=0;s.houseSchedule->origin=NativeHouseClockOrigin::NativeConstructedZeroInit;
 need(t.seed().houseCapacities.at(0)==7&&t.seed().houseCapacities.at(1)==14,"actual model capacities match observed branch");
 need(validateNativeSession(t.seed(),s).empty(),"synthetic fixture valid:"+validateNativeSession(t.seed(),s));t.canonicalize(s);return s;
}

// R4 synthetic integration: the original observation is Save30/2788..2790,
// NOT these fixtures. Stage3 matches that measured boundary; other delays
// exercise the explicit native policy's bounded existing reset contract.
void rewaterRecovery(Test& t){
 auto base=fixture(t,true);nativeRuntimePolicy(base,nativeMigrantNonemptyTag);nativeRuntimePolicy(base,nativeObservedRewaterTag);
 const auto stage=[&](NativeSessionState& s,uint8_t water,uint8_t tail45,uint8_t tail47,uint8_t delay,uint8_t phase){
  auto& o=t.home(s);o.house->houseWater=water;o.house->remaining[45]=tail45;o.house->remaining[47]=tail47;o.house->remaining[52]=delay;o.automation->cachedWater=water;o.automation->delay=delay;
  auto i=t.record(s);i.houseWater=water;i.house=o.house->remaining;s.strictRecords[t.house]=encodeNativeStrictRecord(i);s.houseSchedule->phase=phase;need(validateNativeSession(t.seed(),s).empty(),"R4 synthetic stage valid");t.canonicalize(s);
 };
 need(encodeNativeSession(t.seed(),base).find("RUNTIME8 4\n")!=std::string::npos,"explicit recovery selects new wire4");
 for(uint8_t delay:{1,3,9}){
  auto s=base;stage(s,0,96,0,delay,17);t.tick(s);
  need(t.home(s).house->remaining[52]==delay&&!s.heldHouseReasons.count(t.house),"real Well tail alone waits for cache without sticky legacy hold");t.replay(s,"R4-tail-pending-"+std::to_string(delay));
  s.houseSchedule->phase=28;t.tick(s);need(t.home(s).house->houseWater==1&&t.home(s).house->remaining[52]==delay,"cache refresh preserves positive delay before evolution");
  s.houseSchedule->phase=39;t.replay(s,"R4-before39-"+std::to_string(delay));if(delay==3)t.saveStage(s,"r4-rewater-before39");const auto before=t.home(s);t.tick(s);
  const auto& o=t.home(s);need(o.house->remaining[52]==0&&o.automation->delay==0,"first wet phase39 resets both delay projections");
  need(o.house->type==4&&o.house->model==1&&o.id==before.id&&o.x==before.x&&o.y==before.y&&o.residents==14,"recovery preserves grade identity footprint and population");
  need(!s.heldHouseReasons.count(t.house)&&s.observedDry->pendingExcess.empty()&&s.figureRuntime->migrantPayloads.empty(),"no stale rewater hold, departure or excess ledger");t.replay(s,"R4-cleared-"+std::to_string(delay));if(delay==3)t.saveStage(s,"r4-rewater-cleared");
  stage(s,0,0,0,0,39);t.tick(s);need(t.home(s).house->remaining[52]==1,"later genuine dryness starts a fresh delay");t.replay(s,"R4-redry-"+std::to_string(delay));
 }
 for(unsigned bad=0;bad<3;++bad){auto s=base;stage(s,bad==0?1:0,bad==1?96:0,bad==2?96:0,3,39);t.tick(s);need(t.home(s).house->remaining[52]==3&&t.home(s).house->type==4,"cache-only, tail-only-before-refresh, other water source cannot invent recovery");t.replay(s,"R4-negative-"+std::to_string(bad));}
 auto invalid=base;invalid.runtimePolicies.erase(std::remove(invalid.runtimePolicies.begin(),invalid.runtimePolicies.end(),nativeMigrantNonemptyTag),invalid.runtimePolicies.end());need(validateNativeSession(t.seed(),invalid)=="water_recovery_requires_r3_runtime","R4 requires explicit R3 accounting");
 std::cout<<"{\"rewater_recovery\":true,\"observed_original_delay\":3,\"synthetic_native_delay_boundaries\":[1,3,9],\"checks\":"<<checks<<",\"steps\":"<<steps<<",\"replays\":"<<replays<<"}"<<std::endl;
}

void policyOff(Test& t){
 auto old=fixture(t,false);const auto before=t.home(old);const auto bytes=encodeNativeSession(t.seed(),old);
 need(!old.observedDry&&bytes.find("RUNTIME8 1")!=std::string::npos,"old v8 keeps wire v1");
 t.replay(old,"old-v8");t.tick(old);need(t.home(old).house->type==4&&t.home(old).house->remaining[52]==before.house->remaining[52],"old v8 still isolates type4 without dry edits");
 need(old.heldHouseReasons.count(t.house)&&old.figureRuntime->migrantPayloads.empty(),"old isolation explicit and no migrant");
}
// Synthetic rewatering is deliberately NOT treated as an observed recovery
// rule. After any positive water signal during a pending delay, later dry
// evolution stays isolated with its exact persisted reason and original delay.
void rewaterIsolation(Test& t){
 constexpr const char* reason="observed_dry_downgrade_rewatered_during_delay_unverified";
 auto base=fixture(t,true);t.tick(base);need(t.home(base).house->remaining[52]==1,"dry opportunity creates delay1 before synthetic water return");
 const auto stage=[&](NativeSessionState& state,uint8_t cached,uint8_t tail45,uint8_t tail47,uint8_t phase){
  auto& house=t.home(state);house.house->houseWater=cached;house.house->remaining[45]=tail45;house.house->remaining[47]=tail47;house.automation->cachedWater=cached;
  auto i=t.record(state);i.houseWater=cached;i.house=house.house->remaining;state.strictRecords[t.house]=encodeNativeStrictRecord(i);state.houseSchedule->phase=phase;
  need(validateNativeSession(t.seed(),state).empty(),"explicit water fixture keeps DTO/strict projections valid");
 };
 for(unsigned signal=0;signal<3;++signal){auto state=base;
  stage(state,signal==0?1:0,signal==1?96:0,signal==2?96:0,17);
  t.tick(state);need(state.houseSchedule->phase==18,"rewatering sampled outside evolution phase");
  need(state.heldHouseReasons.count(t.house)&&state.heldHouseReasons.at(t.house)==reason,"cache/tail water records exact persistent isolation");
  need(t.home(state).house->type==4&&t.home(state).house->remaining[52]==1,"wet isolation neither converts nor resets delay");t.replay(state,"rewater-signal-"+std::to_string(signal));
  if(signal==0)t.saveStage(state,"rewatered-held-delay1");
  stage(state,0,0,0,39);t.replay(state,"redried-pre39-"+std::to_string(signal));t.tick(state);
  need(t.home(state).house->type==4&&t.home(state).house->model==1&&t.home(state).house->remaining[52]==1,"redrying never resumes old delay");
  need(state.heldHouseReasons.at(t.house)==reason&&state.observedDry->pendingExcess.empty()&&state.figureRuntime->migrantPayloads.empty(),"redrying keeps precise hold without conversion/departure");t.replay(state,"redried-post39-"+std::to_string(signal));
  state.houseSchedule->phase=39;t.tick(state);need(t.home(state).house->remaining[52]==1&&state.heldHouseReasons.at(t.house)==reason,"repeated dry opportunities remain isolated");
  if(signal==0)t.saveStage(state,"redried-held-delay1");
 }
 auto zero=fixture(t,true);stage(zero,1,0,0,17);t.tick(zero);need(!zero.heldHouseReasons.count(t.house),"water without an active delay is outside rewater guard");
 auto old=fixture(t,false);auto& h=t.home(old);h.house->remaining[52]=1;h.automation->delay=1;auto i=t.record(old);i.house=h.house->remaining;old.strictRecords[t.house]=encodeNativeStrictRecord(i);
 stage(old,1,0,0,17);t.tick(old);need(!old.heldHouseReasons.count(t.house)&&!old.observedDry,"old v8 receives no observed-dry rewater policy");t.replay(old,"old-v8-rewater-unchanged");
 std::cout<<"{\"rewater_isolation\":true,\"positive_water_signals\":3,\"checks\":"<<checks<<",\"steps\":"<<steps<<",\"replay_boundaries\":"<<replays<<",\"synthetic_water_return_not_original_observation\":true}"<<std::endl;
}
// Independent continuation case: consumes one of this test's own paused
// in-flight checkpoints. No prefix fixtures are rerun and no teleport is used.
void continueNaturalExit(Test& t,NativeSessionState s,unsigned tickBudget=1500){
 need(tickBudget>0&&tickBudget<=1500,"bounded continuation tick budget");
 need(s.observedDry&&s.figureRuntime->migrantPayloads.size()==1,"continuation requires explicit dry migrant checkpoint");
 for(const auto& o:s.objects)if(o.kind==NativeObjectKind::House){need(!t.house,"continuation fixture has one house");t.house=o.id;}
 need(t.house&&t.home(s).residents==7,"continuation preserves post-departure population7");
 if(s.paused)t.action(s,NativeSessionAction::Resume);
 auto actor=t.migrant(s);need(actor.action==9&&actor.owner==0&&actor.home==0&&actor.route>0,"continuation starts on actual owned exit route");
 const auto freeBefore=s.figureRuntime->freeFigureSlots.size();const auto slot=actor.slot;
 t.replay(s,"natural-route-restored-start");unsigned routeTicks=0;bool checkpointed=false;
 while(!s.figureRuntime->migrantPayloads.empty()&&routeTicks<tickBudget){
  actor=t.migrant(s);
  try{t.tick(s);}catch(const std::exception& e){throw std::runtime_error(std::string(e.what())+" x="+std::to_string(actor.x)+" y="+std::to_string(actor.y)+" route="+std::to_string(actor.route)+" cursor="+std::to_string(actor.routeCursor)+" direction="+std::to_string(actor.direction));}
  ++routeTicks;
  if(!checkpointed&&routeTicks==10&&!s.figureRuntime->migrantPayloads.empty()){t.replay(s,"natural-route-midpoint");checkpointed=true;}
  if(routeTicks%100==0&&!s.figureRuntime->migrantPayloads.empty()){
   const auto now=t.migrant(s);std::cout<<"{\"natural_route_tick\":"<<routeTicks<<",\"x\":"<<now.x<<",\"y\":"<<now.y<<",\"route\":"<<now.route<<",\"cursor\":"<<now.routeCursor<<",\"action\":"<<unsigned(now.action)<<",\"held\":"<<s.heldWalkerReasons.size()<<"}"<<std::endl;
   t.saveStage(s,"natural-progress"+std::to_string(routeTicks));
  }
 }
 if(!s.figureRuntime->migrantPayloads.empty()){const auto last=t.migrant(s);throw std::runtime_error("natural exit budget"+std::to_string(tickBudget)+": x="+std::to_string(last.x)+" y="+std::to_string(last.y)+" action="+std::to_string(last.action)+" route="+std::to_string(last.route)+" cursor="+std::to_string(last.routeCursor)+" direction="+std::to_string(last.direction)+" held="+std::to_string(s.heldWalkerReasons.size()));}
 need(routeTicks>0&&s.figureRuntime->freeFigureSlots.size()==freeBefore+1&&std::count(s.figureRuntime->freeFigureSlots.begin(),s.figureRuntime->freeFigureSlots.end(),uint16_t(slot))==1,"natural arrival returns exactly one FIFO identity");
 for(auto id:s.figureRuntime->cellHeads)need(id!=slot,"natural cleanup removes cell-head reference");
 for(auto owner:s.figureRuntime->services.routeOwners)need(owner!=int(slot),"natural cleanup releases route owner");
 need(t.home(s).residents==7&&t.record(s).migrant==0&&s.heldWalkerReasons.empty(),"natural exit preserves housing population and no home reservation");
 t.replay(s,"natural-cleaned");t.saveStage(s,"natural-cleaned");
 std::cout<<"{\"natural_exit_complete\":true,\"route_ticks\":"<<routeTicks<<",\"checks\":"<<checks<<",\"replay_boundaries\":"<<replays<<",\"ported_path_not_original_observation\":true}"<<std::endl;
}
void dryStages(Test& t,bool naturalExit){
 auto s=fixture(t,true);need(s.observedDry&&encodeNativeSession(t.seed(),s).find("RUNTIME8 2")!=std::string::npos,"explicit dry policy uses extended wire");
 const auto original=t.home(s);
 for(unsigned delay=0;delay<10;++delay){
  // Steer only the scheduling phase to exercise ten successive evolution
  // opportunities quickly. This is not an original calendar duration test.
  s.houseSchedule->phase=39;need(t.home(s).house->remaining[52]==delay,"staged incoming delay");t.replay(s,"delay-"+std::to_string(delay));t.tick(s);
  const auto& o=t.home(s);need(o.id==original.id&&o.x==original.x&&o.y==original.y&&o.residents==14,"same identity/footprint and no phase39 population clamp");
  need(o.house->type==(delay==9?3:4)&&o.house->model==(delay==9?0:1)&&o.house->remaining[52]==(delay==9?0:delay+1),"exact delay/type/model transition");
  need(t.record(s).free==0,"phase39 keeps pre-capacity free value");
  need(s.observedDry->pendingExcess.size()==size_t(delay==9),"pending only upon lower-capacity transition");
 }
 need(s.observedDry->pendingExcess.at(t.house)==7&&s.figureRuntime->migrantPayloads.empty(),"overcapacity saved without early departure");t.replay(s,"post39-pending");t.saveStage(s,"post39-pending14");
 while(s.houseSchedule->phase!=22)t.tick(s);t.replay(s,"pre22");t.tick(s);
 need(t.record(s).free==-7&&t.home(s).residents==14&&s.observedDry->pendingExcess.at(t.house)==7,"phase22 writes signed negative free without clipping residents");t.replay(s,"post22");
 t.tick(s);need(s.houseSchedule->phase==24&&t.home(s).residents==14&&s.figureRuntime->migrantPayloads.empty(),"phase23 does not perform excess departure");const auto before24=s;t.replay(s,"pre24");
 auto noSlots=before24;noSlots.figureRuntime->freeFigureSlots.clear();t.tick(noSlots);
 need(t.home(noSlots).residents==7&&noSlots.observedDry->pendingExcess.empty()&&noSlots.observedDry->departedSinceMeal==7,"no-slot migrated contract keeps native departure accounting");
 need(noSlots.figureRuntime->migrantPayloads.empty()&&noSlots.figureRuntime->freeFigureSlots.empty(),"no-slot contract allocates no actor");
 const auto noSlotStrict=t.strict(noSlots);need(noSlotStrict.city->figures.services->population->total==14,"known no-slot strict population inconsistency remains explicit");t.replay(noSlots,"no-slots");
 auto missing=before24;missing.figureRuntime->services.allocationSerial.reset();const auto missingBytes=encodeNativeSession(t.seed(),missing);need(!missingBytes.empty(),"missing-input fixture encodable");
 const auto failure=transactNativeHouseRuntime(t.seed(),missing,{NativeSessionAction::Tick,missing.revision},*t.src.runtime);
 need(!failure.committed&&!failure.error.empty()&&encodeNativeSession(t.seed(),missing)==missingBytes,"missing allocation input refuses entire ledger/population prefix");
 t.tick(s);auto w=t.migrant(s);need(t.home(s).residents==7&&s.observedDry->pendingExcess.empty()&&s.observedDry->departedSinceMeal==7,"phase24 atomically records departure7");
 need(w.type==13&&w.action==6&&w.owner==0&&w.home==0&&w.employmentAmount==7&&w.counter==1,"allocated type13 already receives current figure pass");
 need(t.record(s).free==-7&&t.strict(s).city->figures.services->population->total==7,"negative free stays stale while allocated departure reduces strict population");t.replay(s,"post24");t.saveStage(s,"after24-migrant7");
 while(s.houseSchedule->phase!=50)t.tick(s);w=t.migrant(s);need(w.counter==26&&w.action==6&&w.employmentAmount==7,"phase50 boundary retains observed waiting counter26");t.replay(s,"phase50-counter26");
 unsigned waited=0;while(t.migrant(s).action==6&&waited++<60)t.tick(s);w=t.migrant(s);
 need(w.action==9&&w.counter==0&&w.home==0&&s.heldWalkerReasons.empty(),"ported waiting branch selects exit with no available house");
 auto leaving=t.strict(s);const auto endpoints=*leaving.city->figures.migrationRoadEndpoints;
 need(w.destinationX==endpoints[2]&&w.destinationY==endpoints[3],"exit target is actual source endpoint");t.replay(s,"leaving-route");
 t.tick(s);w=t.migrant(s);need(w.action==9&&w.route>0,"actual next tick allocates exit route");t.saveStage(s,"in-flight-action9");leaving=t.strict(s);
 const auto freeBefore=s.figureRuntime->freeFigureSlots.size();auto atExit=s;unsigned naturalSteps=0;
 if(naturalExit){
  bool checkpointed=false;
  while(!atExit.figureRuntime->migrantPayloads.empty()&&naturalSteps<1500){
   const auto prior=t.migrant(atExit);
   try{t.tick(atExit);}catch(const std::exception& e){throw std::runtime_error(std::string(e.what())+" migrant x="+std::to_string(prior.x)+" y="+std::to_string(prior.y)+" route="+std::to_string(prior.route)+" cursor="+std::to_string(prior.routeCursor)+" direction="+std::to_string(prior.direction));}
   ++naturalSteps;if(!checkpointed&&naturalSteps==10&&!atExit.figureRuntime->migrantPayloads.empty()){t.replay(atExit,"natural-route-midpoint");checkpointed=true;}
  }
  if(!atExit.figureRuntime->migrantPayloads.empty()){const auto last=t.migrant(atExit);throw std::runtime_error("natural exit limit1500: x="+std::to_string(last.x)+" y="+std::to_string(last.y)+" action="+std::to_string(last.action)+" route="+std::to_string(last.route)+" cursor="+std::to_string(last.routeCursor)+" direction="+std::to_string(last.direction));}
  need(naturalSteps>0,"natural exit executes route updates");
 }else{
  // Separate SYNTHETIC terminal boundary. The complete natural route has its
  // own --natural-exit case; this original boundary test remains intact.
  auto city=*leaving.city;auto terminal=city.figures.figures.at(w.slot);
  if(terminal.route>0&&city.figures.services->routeOwners[size_t(terminal.route)]==int(terminal.slot))city.figures.services->routeOwners[size_t(terminal.route)]=0;
  terminal.route=terminal.routeCursor=terminal.routeLength=0;terminal.progress=20;terminal.roamSteps=0;terminal.counter=0;
  const uint32_t exitCell=*city.figures.cropBase+uint32_t(endpoints[2])+228u*uint32_t(endpoints[3]);
  need(originalFigureRelink(terminal,exitCell,city.figures),"synthetic exit relinks original acyclic cell chain");
  terminal.x=terminal.priorX=terminal.destinationX=endpoints[2];terminal.y=terminal.priorY=terminal.destinationY=endpoints[3];terminal.subX=int16_t(20*int(terminal.x));terminal.subY=int16_t(20*int(terminal.y));terminal.direction=terminal.previousDirection=0;city.figures.figures.at(terminal.slot)=terminal;
  const auto projected=strictToNative(t.seed(),s,city,leaving.isolation,t.src.runtime->strictTemplate);need(bool(projected.state),"synthetic exit bridge:"+projected.refusal);atExit=*projected.state;t.replay(atExit,"terminal-cleanup");
  for(unsigned n=0;n<3&&!atExit.figureRuntime->migrantPayloads.empty();++n)t.tick(atExit);
 }
 need(atExit.figureRuntime->migrantPayloads.empty(),"terminal arrival removes payload");
 need(atExit.figureRuntime->freeFigureSlots.size()==freeBefore+1&&std::count(atExit.figureRuntime->freeFigureSlots.begin(),atExit.figureRuntime->freeFigureSlots.end(),uint16_t(w.slot))==1,"cleanup returns exactly one FIFO slot");
 for(auto id:atExit.figureRuntime->cellHeads)need(id!=w.slot,"cleanup clears cell-head references");for(auto owner:atExit.figureRuntime->services.routeOwners)need(owner!=int(w.slot),"cleanup clears route ownership");
 need(t.home(atExit).residents==7&&t.record(atExit).migrant==0,"cleanup neither rehouses nor reserves house worker");t.replay(atExit,"cleaned");
 std::cout<<"{\"dry_stage_chain\":true,\"synthetic_runtime_stages\":true,\"long_exit_walk_tested\":"<<(naturalExit?"true":"false")<<",\"natural_exit_ticks\":"<<naturalSteps<<",\"checks\":"<<checks<<",\"steps\":"<<steps<<",\"replay_boundaries\":"<<replays<<"}"<<std::endl;
}
}
int main(int argc,char** argv){try{
 const char* data=std::getenv("EMPEROR_GAMEDATA_ROOT");auto root=ResourceRoot::fromAbsolute(data?data:"");if(!root.configured()){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return 0;}
 std::string error;auto src=loadNativeSessionSource(root,argc>1?argv[1]:"Save/2026102301/历史战役自动保存_1.sav",error);need(bool(src),"source:"+error);need(prepareNativeHouseRuntimeSource(root,*src,error),"runtime source:"+error);
 const bool continuation=argc>2&&std::string(argv[2])=="--continue-exit";
 const bool naturalExit=argc>2&&std::string(argv[2])=="--natural-exit";std::filesystem::path output=continuation?(argc>4?argv[4]:""):(argc>3?argv[3]:"");if(!output.empty())need(output.is_absolute()&&std::filesystem::is_directory(output),"explicit existing stage output directory");
 Test t{*src,root,output};
 if(continuation){need(argc>3,"continuation checkpoint argument");auto bytes=readNativeSessionFile(argv[3],root.configuredDirectory(),error);need(bool(bytes),"read own checkpoint:"+error);auto saved=decodeNativeSession(src->seed,*bytes,error);need(bool(saved),"decode own checkpoint:"+error);const unsigned budget=argc>5?unsigned(std::stoul(argv[5])):1500;continueNaturalExit(t,*saved,budget);}
 else if(argc>2&&std::string(argv[2])=="--rewater-isolation"){rewaterIsolation(t);}
 else if(argc>2&&std::string(argv[2])=="--rewater-recovery"){rewaterRecovery(t);}
 else {policyOff(t);dryStages(t,naturalExit);}std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"steps\":"<<steps<<",\"replay_boundaries\":"<<replays<<"}\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<std::endl;return 1;}}
