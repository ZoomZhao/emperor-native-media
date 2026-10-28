// SPDX-License-Identifier: MIT
// C1 source/model integration. Gameplay runs in the native port; none of these
// checkpoints represents an original-game experiment or original observation.
#define main nativeCityPreviousDryMain
#include "native_house_dry_runtime_tests.cpp"
#undef main
#include "native_city_context.hpp"
#include "native_session_feedback.hpp"
#include <array>
#include <set>

namespace {
NativeCityContextState cityContextOf(const NativeSessionState& s){
 std::string e;need(nativeUsesCityContext(s)&&bool(s.cityContext),"explicit C1 payload");
 auto c=decodeNativeCityContext(*s.cityContext,e);need(bool(c),"C1 context:"+e);return *c;
}
std::string cityContextWire(const NativeCityContextState& c){std::string e;auto b=encodeNativeCityContext(c,e);need(bool(b),"C1 snapshot:"+e);return *b;}
template<class T> bool cityCallsUnchanged(const std::optional<T>& a,const std::optional<T>& b){return bool(a)==bool(b)&&(!a||a->calls==b->calls);}
using HouseInventories=std::map<uint64_t,std::array<int16_t,12>>;
HouseInventories cityHouseInventories(const NativeSessionState& s){
 HouseInventories out;for(const auto& o:s.objects)if(o.house){std::array<int16_t,12> goods{};
  for(size_t n=0;n<goods.size();++n){const auto v=originalHouseGoodsWord(o.house->remaining.data(),o.house->remaining.size(),n);need(bool(v),"known house goods field");goods[n]=*v;}out.emplace(o.id,goods);
 }return out;
}
unsigned cityCheckJobs(Test& t,const NativeSessionState& s,const NativeCityContextState& c,bool full){
 const auto plan=planNativeLabor(t.seed(),s);need(plan.error.empty(),"scheduled job projection:"+plan.error);
 size_t jobs=0;unsigned workers=0;
 for(const auto& o:s.objects)if(o.kind==NativeObjectKind::Well||o.kind==NativeObjectKind::Hunter){
  ++jobs;const auto saved=c.workers.find(size_t(o.id));need(saved!=c.workers.end(),"saved job owns native object");
  const auto* projected=plan.find(o.id);need(projected&&projected->workers==unsigned(saved->second),"plan reflects scheduled workers");
  std::string e;const auto i=decodeNativeStrictRecord(s.strictRecords.at(o.id),e);need(bool(i),"job record:"+e);
  need(i->workers==saved->second,"record reflects scheduled workers");workers+=unsigned(saved->second);
 }
 need(c.workers.size()==jobs&&plan.jobs.size()==jobs,"job key set is complete");
 if(full){const auto strict=t.strict(s);const auto& b=*strict.city->buildingContext;
  need(b.goods&&strict.city->figures.goods&&b.goods->savedPhase==s.houseSchedule->phase&&strict.city->figures.goods->savedPhase==s.houseSchedule->phase,"C goods phase and canonical mirror");
  need(cityContextWire(captureNativeCityContext(b))==*s.cityContext,"all C fields survive actual template rebuild");
  for(const auto& job:plan.jobs){const auto& record=strict.city->buildings.at(size_t(job.id)).state;
   need(record.inputs.workers==int(job.workers)&&strict.city->figures.buildings->at(size_t(job.id)).workers==int(job.workers),"record/link/plan workers agree");
   if(record.production)need(record.production->workers==int(job.workers),"production worker mirror agrees");
  }
 }
 return workers;
}
void cityScene(Test& t,const NativeSessionState& s){
 const auto scene=nativeSessionScene(t.src,s);need(scene&&scene->figures,"C1 native scene projection");
 const auto& f=*s.figureRuntime;need(scene->figures->live.size()==f.wellWalkerPayloads.size()+f.immigrantPayloads.size()+f.migrantPayloads.size(),"all C1 live actor families reach scene");
 for(const auto& payload:f.immigrantPayloads){std::string e;const auto actor=decodeNativeImmigrant(payload,e);need(bool(actor),"natural immigrant payload:"+e);
  const auto projected=encodeNativeImmigrant(scene->figures->slots.at(actor->slot),e);need(projected&&*projected==payload,"scene preserves all type11 fields");
 }
}
// Pause and save a disposable copy; its revision and clock never escape into
// the continuous native trajectory. Output is limited to the C1 test folder.
void citySaveStage(Test& t,const NativeSessionState& state,const std::string& label){
 if(t.output.empty())return;const auto before=encodeNativeSession(t.seed(),state);auto paused=state;
 if(!paused.paused)t.action(paused,NativeSessionAction::Pause);const auto bytes=encodeNativeSession(t.seed(),paused);need(!bytes.empty(),"C1 stage encodes");
 const auto stamp=std::chrono::system_clock::now().time_since_epoch().count();const auto file=t.output/(label+"-"+std::to_string(stamp)+".encity");std::string e;
 need(writeNativeSessionNew(file.string(),t.root.configuredDirectory(),bytes,e),"new C1 checkpoint:"+e);
 const auto read=readNativeSessionFile(file.string(),t.root.configuredDirectory(),e);need(read&&*read==bytes,"saved C1 file readback:"+e);
 auto restored=decodeNativeSession(t.seed(),*read,e);need(restored&&validateNativeHouseRuntimeReady(t.seed(),*restored,*t.src.runtime).empty(),"saved C1 checkpoint load-ready:"+e);
 need(encodeNativeSession(t.seed(),state)==before,"checkpoint did not advance original trajectory");
 std::cout<<"{\"stage_file\":"<<std::quoted(file.string())<<",\"label\":"<<std::quoted(label)<<",\"source\":"<<std::quoted(t.seed().source)
  <<",\"state\":\"native-c1\",\"tick\":"<<paused.ticks<<",\"phase\":"<<unsigned(paused.houseSchedule->phase)<<",\"cycle\":"<<paused.houseSchedule->cycle
  <<",\"month_part\":"<<paused.houseSchedule->monthPart<<",\"population\":"<<nativePopulation(paused)<<",\"wire\":"<<nativeHouseRuntimeWireVersion(paused)
  <<",\"paused\":true,\"original_behavior_observed\":false}"<<std::endl;
}
NativeSessionState cityBuild(Test& t,uint64_t& well){
 std::string e;auto initial=startNativeHouseRuntime(t.seed(),*t.src.runtime,e,NativeWellReturnPolicy::ObservedHomeRoad,true,true,true,true,true);
 need(bool(initial),"C1 real-source constructor:"+e);auto s=*initial;
 need(nativeHouseRuntimeWireVersion(s)==6&&nativeUsesImmigration(s)&&nativeUsesCityContext(s),"explicit C1 wire6 mode");
 std::vector<int> path;const auto xy=choose(t.src,path);
 for(int cell:path)if(!t.seed().sourceRoad[size_t(cell)])t.action(s,NativeSessionAction::Road,cell%int(t.seed().side),cell/int(t.seed().side));
 t.action(s,NativeSessionAction::House,xy.first,xy.second);t.house=s.objects.back().id;
 // Reuse B's bounded nearby Well placement query on a discarded candidate;
 // actual construction goes through the resource-aware C1 transaction.
 const auto house=t.home(s);const auto roads=nativeRoads(t.seed(),s);int wx=-1,wy=-1;
 for(int y=std::max(0,int(house.y)-4);y<=std::min(int(t.seed().side)-2,int(house.y)+5)&&wx<0;++y)
  for(int x=std::max(0,int(house.x)-4);x<=std::min(int(t.seed().side)-2,int(house.x)+5)&&wx<0;++x){
   NativeSessionObject probe;probe.kind=NativeObjectKind::Well;probe.x=uint32_t(x);probe.y=uint32_t(y);if(!nativeHouseConnected(t.seed(),probe,roads))continue;
   auto candidate=s;const auto placed=transactNativeSession(t.seed(),candidate,{NativeSessionAction::Well,s.revision,x,y});if(placed.committed){wx=x;wy=y;}
  }
 need(wx>=0,"legal nearby connected Well");t.action(s,NativeSessionAction::Well,wx,wy);well=s.objects.back().id;
 need(nativePopulation(s)==0&&s.figureRuntime->immigrantPayloads.empty(),"constructed C1 city starts uninhabited");
 const auto c=cityContextOf(s);need(c.workers.at(size_t(well))==0,"new C1 job has no invented workers before phase25");
 need(planNativeFoodCycle(t.seed(),s).arrivals.empty(),"native meals cannot create C1 residents");cityScene(t,s);cityCheckJobs(t,s,c,true);citySaveStage(t,s,"c1-empty");
 return s;
}
void cityQueries(Test& t,const NativeSessionState& s,uint64_t well){
 const auto before=encodeNativeSession(t.seed(),s);const auto saved=cityContextOf(s);
 for(unsigned n=0;n<3;++n){const auto feedback=queryNativeCityFeedback(t.seed(),s);need(feedback.error.empty()&&feedback.find(well),"C1 feedback exists");
  need(feedback.find(well)->workers==unsigned(saved.workers.at(size_t(well))),"feedback cannot redistribute workers");
  const auto food=planNativeFoodCycle(t.seed(),s);need(food.error.empty()&&food.arrivals.empty(),"food query cannot allocate immigrants");cityScene(t,s);
 }
 need(encodeNativeSession(t.seed(),s)==before,"repeated feedback/planning/scene queries do not mutate or dispatch");
}
void runCityContext(Test& t){
 uint64_t well=0;auto s=cityBuild(t,well);const auto initialClock=*s.houseSchedule;const auto initial=cityContextOf(s);
 t.action(s,NativeSessionAction::Resume);std::array<unsigned,51> phases{};std::set<unsigned> restored;std::set<unsigned> actions;
 std::optional<uint64_t> firstArrival,firstStaffed;bool querySaved=false;unsigned availabilityPasses=0,foodPasses=0,employmentPasses=0,wraps=0;
 for(unsigned n=0;n<5000;++n){
  const unsigned p=s.houseSchedule->phase;need(p<=50,"current phase bounded");const auto before=cityContextOf(s);const auto previousPopulation=nativePopulation(s);
  const bool firstPhase=restored.insert(p).second;
  if(firstPhase){t.replay(s,"c1-phase-"+std::to_string(p));cityCheckJobs(t,s,before,true);cityScene(t,s);}
  for(const auto& payload:s.figureRuntime->immigrantPayloads){std::string e;const auto actor=decodeNativeImmigrant(payload,e);need(bool(actor),"natural type11 decode:"+e);
   if((actor->action==7||actor->action==8)&&actions.insert(unsigned(actor->action)).second){cityScene(t,s);citySaveStage(t,s,"c1-action-"+std::to_string(actor->action));}
  }
  if(firstStaffed&&!querySaved&&p==17){cityQueries(t,s,well);citySaveStage(t,s,"c1-query-phase-17");querySaved=true;}
  HouseInventories inventory;if(p==17)inventory=cityHouseInventories(s);const uint32_t nativeStock=s.foodStock;
  const auto produced=s.foodProduced,consumed=s.foodConsumed;
  t.tick(s);++phases[p];const auto after=cityContextOf(s);
  need(after.goods&&after.goods->savedPhase==s.houseSchedule->phase,"every completed pass saves the next goods phase");
  need(after.accountingCalls==initial.accountingCalls&&after.statisticsCalls==initial.statisticsCalls&&cityCallsUnchanged(after.religion,initial.religion)&&cityCallsUnchanged(after.cycleTail,initial.cycleTail)&&cityCallsUnchanged(after.buildingTotals,initial.buildingTotals),"C1 does not silently enable other city/calendar services");
  need(s.houseSchedule->cycle==initialClock.cycle&&s.houseSchedule->monthPart==initialClock.monthPart&&s.houseSchedule->month==initialClock.month,"C1 retains explicit calendar freeze");
  if(p==12)++availabilityPasses;
  if(p==17){++foodPasses;need(after.foodCalls==before.foodCalls+1&&after.storageCalls==before.storageCalls+1,"natural17 executes complete food/storage census once");
   need(cityHouseInventories(s)==inventory,"food statistics do not consume house inventory");
   need(s.foodStock==nativeStock&&s.foodProduced==produced&&s.foodConsumed==consumed,"food statistics do not alter separate native stock ledger");
  }else need(after.foodCalls==before.foodCalls&&after.storageCalls==before.storageCalls,"food census stays on phase17");
  if(p==25){++employmentPasses;need(after.employmentCalls==before.employmentCalls+1,"actual25 scheduled allocation executed once");}
  else need(after.employmentCalls==before.employmentCalls&&after.workers==before.workers,"non25 pass cannot recompute C1 staffing from v6");
  if(p==50){++wraps;need(s.houseSchedule->phase==0&&s.houseSchedule->cycleWrapTailSkipped,"natural phase50 retains named tail skip");}
  const auto workers=cityCheckJobs(t,s,after,p==25);
  if(nativePopulation(s)>previousPopulation){need(s.populationRuntime->populationArrived-s.populationRuntime->populationDeparted==nativePopulation(s),"actual arrival and population ledger agree");
   if(!firstArrival){firstArrival=s.ticks;need(t.home(s).residents>0&&s.populationRuntime->arrived>0,"natural type11 first arrival happened");
    std::cout<<"{\"checkpoint\":\"c1-natural-first-arrival\",\"source\":"<<std::quoted(t.seed().source)<<",\"tick\":"<<s.ticks<<",\"population\":"<<nativePopulation(s)<<",\"original_behavior_observed\":false}"<<std::endl;}
  }
  if(firstArrival&&!firstStaffed&&p==25&&workers>0){firstStaffed=s.ticks;cityQueries(t,s,well);cityScene(t,s);citySaveStage(t,s,"c1-staffed");}
  if(firstStaffed&&s.ticks-*firstStaffed>=102&&querySaved&&restored.size()==51)break;
  if(!firstArrival&&n+1==4000)throw std::runtime_error("C1 natural first arrival exceeded4000 native ticks");
 }
 need(firstArrival&&firstStaffed&&s.ticks-*firstStaffed>=102,"natural arrival and real scheduled staffing followed by two full51-phase loops");
 need(actions.count(7)&&actions.count(8),"natural type11 road and approach stages captured");need(querySaved,"staffed query-phase checkpoint captured");
 need(restored.size()==51&&std::all_of(phases.begin(),phases.end(),[](unsigned n){return n>0;}),"all51 phases executed and restored once");
 for(unsigned p:{12u,17u,25u,35u,36u,39u,50u})need(phases[p]>=2,"target C/B phases exercised repeatedly");
 const auto final=cityContextOf(s);cityCheckJobs(t,s,final,true);cityScene(t,s);cityQueries(t,s,well);
 need(validateNativeHouseRuntimeReady(t.seed(),s,*t.src.runtime).empty(),"final real-source C1 graph and context load-ready");
 std::cout<<"{\"city_runtime\":true,\"source\":"<<std::quoted(t.seed().source)<<",\"checks\":"<<checks<<",\"steps\":"<<steps<<",\"replays\":"<<replays
  <<",\"phase_coverage\":"<<restored.size()<<",\"phase_denominator\":51,\"availability_passes\":"<<availabilityPasses<<",\"food_passes\":"<<foodPasses
  <<",\"employment_passes\":"<<employmentPasses<<",\"wraps\":"<<wraps<<",\"first_arrival_tick\":"<<*firstArrival<<",\"first_staffed_tick\":"<<*firstStaffed
  <<",\"post_staffing_ticks\":"<<s.ticks-*firstStaffed<<",\"population\":"<<nativePopulation(s)<<",\"scheduled_well_workers\":"<<final.workers.at(size_t(well))
  <<",\"wire\":6,\"original_behavior_observed\":false,\"passed\":true}"<<std::endl;
}
bool cityOutputAllowed(const std::filesystem::path& output){
 const auto allowed=std::filesystem::weakly_canonical("/tmp/house-calendar-c1-20261027"),actual=std::filesystem::weakly_canonical(output);
 auto a=allowed.begin(),b=actual.begin();for(;a!=allowed.end();++a,++b)if(b==actual.end()||*a!=*b)return false;return true;
}
}
int main(int argc,char** argv){try{
 const char* data=std::getenv("EMPEROR_GAMEDATA_ROOT");auto root=ResourceRoot::fromAbsolute(data?data:"");if(!root.configured()){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return 0;}
 // Usage: binary [relative-source] [absolute-output-directory]. The optional
 // --city-runtime spelling also accepts the established source/mode/out form.
 const std::string relative=argc>1?argv[1]:"Save/2026102301/历史战役自动保存_1.sav";
 need(relative.rfind("Save/2026102301/",0)==0||relative.rfind("Save/2026102401/",0)==0,"only the two established source folders");
 std::string error;auto src=loadNativeSessionSource(root,relative,error);need(bool(src),"C1 source:"+error);need(prepareNativeHouseRuntimeSource(root,*src,error),"C1 source resources:"+error);
 const bool mode=argc>2&&std::string(argv[2])=="--city-runtime";const std::filesystem::path output=mode?(argc>3?argv[3]:""):(argc>2?argv[2]:"");
 if(!output.empty())need(output.is_absolute()&&std::filesystem::is_directory(output)&&cityOutputAllowed(output),"existing stage output within /tmp/house-calendar-c1-20261027 required");
 Test t{*src,root,output,0};runCityContext(t);return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<std::endl;return 1;}}
