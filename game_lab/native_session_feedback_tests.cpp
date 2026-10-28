// SPDX-License-Identifier: MIT
#include "native_session_presentation.hpp"
#include "native_session_source.hpp"
#include "native_house_runtime_source.hpp"
#include "native_house_runtime.hpp"
#include "native_session_io.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace game_lab;
namespace {
size_t checks=0,paired=0;
void need(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
NativeSessionSeed seed(){NativeSessionSeed v;v.source="authored.sav";v.sourceRevision=1;v.modelRevision=2;v.side=16;v.initialFunds=2000;v.roadCost=2;v.houseCost=15;v.wellCost=25;v.hunterCost=32;v.houseCapacity=7;v.buildable.assign(256,1);v.sourceRoad.assign(256,0);return v;}
void act(const NativeSessionSeed& seed,NativeSessionState& s,NativeSessionAction a,int x=0,int y=0){const auto r=transactNativeSession(seed,s,{a,s.revision,x,y});need(r.committed,r.error.c_str());}
NativeCityFeedback inspect(const NativeSessionSeed& seed,const NativeSessionState& s){const auto bytes=encodeNativeSession(seed,s);auto q=queryNativeCityFeedback(seed,s);need(q.error.empty(),q.error.c_str());for(int repeat=0;repeat<3;++repeat){for(const auto&o:s.objects)(void)nativeObjectDescription(seed,s,o.id,q);(void)nativeCityFeedbackSummary(s,q);(void)nativeCityFeedbackExplanation(s,q);}need(encodeNativeSession(seed,s)==bytes,"feedback is pure including pause/RNG/counters");return q;}
void compareNextMeal(const NativeSessionSeed& seed,const NativeSessionState& before){auto q=inspect(seed,before);auto after=before;if(after.paused)act(seed,after,NativeSessionAction::Resume);for(unsigned n=0;n<q.untilMeal;++n)act(seed,after,NativeSessionAction::Tick);for(const auto&o:before.objects)if(o.kind==NativeObjectKind::House){const auto i=std::find_if(after.objects.begin(),after.objects.end(),[&](const auto&x){return x.id==o.id;});need(i!=after.objects.end()&&i->residents==o.residents+(q.find(o.id)->arrivalPlanned?1u:0u),"forecast equals real next meal arrival");++paired;}if(before.rulesVersion>=3)need(after.foodStock==q.food.stock&&after.lastFood.produced==q.food.cycle.produced&&after.lastFood.demand==q.food.cycle.demand&&after.lastFood.fed==q.food.cycle.fed,"food planner receipt matches committed update");}
void build(const NativeSessionSeed& seed,NativeSessionState& s){act(seed,s,NativeSessionAction::House,2,2);act(seed,s,NativeSessionAction::Road,1,2);act(seed,s,NativeSessionAction::Well,4,2);act(seed,s,NativeSessionAction::Road,4,1);act(seed,s,NativeSessionAction::Hunter,6,2);act(seed,s,NativeSessionAction::Road,6,1);}
void run(){auto model=seed();auto s=startNativeSession(model,5);act(model,s,NativeSessionAction::House,2,2);auto q=inspect(model,s);need(!q.find(1)->connected&&!q.find(1)->watered&&!q.find(1)->foodShortage,"all structural causes without inventing allocation for ineligible house");compareNextMeal(model,s);need(nativeObjectDescription(model,s,1,q).find("断路、缺水")!=std::string::npos,"multiple reasons visible in priority order");
act(model,s,NativeSessionAction::Road,1,2);q=inspect(model,s);need(q.find(1)->connected&&!q.find(1)->watered,"only road restored");compareNextMeal(model,s);
act(model,s,NativeSessionAction::Well,4,2);q=inspect(model,s);need(!q.find(3)->connected&&!q.find(1)->watered,"disconnected well cannot supply");
act(model,s,NativeSessionAction::Road,4,1);q=inspect(model,s);need(q.find(3)->connected&&q.find(1)->watered&&q.find(1)->foodShortage,"water restored but actual food missing");compareNextMeal(model,s);
act(model,s,NativeSessionAction::Hunter,6,2);act(model,s,NativeSessionAction::Road,6,1);q=inspect(model,s);need(q.foodStock==0&&q.food.cycle.produced==4&&!q.find(1)->foodShortage&&q.find(1)->arrivalPlanned,"zero stock with current production is not missing food");need(q.untilMeal==2&&q.untilTax==4,"fresh cadence");compareNextMeal(model,s);
act(model,s,NativeSessionAction::Resume);act(model,s,NativeSessionAction::Tick);q=inspect(model,s);need(q.untilMeal==1&&q.untilTax==3,"odd tick waits exactly one update");compareNextMeal(model,s);act(model,s,NativeSessionAction::Tick);act(model,s,NativeSessionAction::Pause);q=inspect(model,s);need(q.tax.cycle.income==1&&q.tax.cycle.eligible==1,"tax basis is current residents only");
act(model,s,NativeSessionAction::HuntingPause);q=inspect(model,s);need(!q.find(5)->productionEnabled&&nativeObjectDescription(model,s,5,q).find("狩猎已暂停")!=std::string::npos,"explicit production pause reason");compareNextMeal(model,s);act(model,s,NativeSessionAction::HuntingResume);
// Existing demand consumes shared stock before arrivals, then native ID order.
auto scarce=startNativeSession(model,5);for(int x:{2,4,6,8,10}){act(model,scarce,NativeSessionAction::House,x,2);act(model,scarce,NativeSessionAction::Road,x,1);}act(model,scarce,NativeSessionAction::Well,6,6);act(model,scarce,NativeSessionAction::Road,5,6);act(model,scarce,NativeSessionAction::Hunter,10,4);act(model,scarce,NativeSessionAction::Road,12,4);q=inspect(model,scarce);need(q.food.cycle.arrivals==4&&q.find(9)->foodShortage&&!q.find(1)->foodShortage,"last eligible house explains food allocation priority");compareNextMeal(model,scarce);
// Full occupancy is naturally reached with enough actual food production.
auto full=startNativeSession(model,5);build(model,full);act(model,full,NativeSessionAction::Hunter,6,4);act(model,full,NativeSessionAction::Road,5,4);act(model,full,NativeSessionAction::Resume);for(int n=0;n<14;++n){compareNextMeal(model,full);act(model,full,NativeSessionAction::Tick);}q=inspect(model,full);need(q.find(1)->full&&!q.find(1)->arrivalPlanned&&nativeObjectDescription(model,full,1,q).find("已满员")!=std::string::npos,"full occupancy visible and stops additions");
act(model,full,NativeSessionAction::HuntingPause);for(int n=0;n<10;++n)act(model,full,NativeSessionAction::Tick);q=inspect(model,full);need(q.food.cycle.fed<q.food.cycle.demand&&q.find(1)->full,"full remains primary while service summary exposes unmet demand");
// Demolition immediately changes the same shared road/water predicates.
act(model,full,NativeSessionAction::Pause);NativeSessionCommand remove{NativeSessionAction::Demolish,full.revision};remove.objectId=2;need(transactNativeSession(model,full,remove).committed,"remove house road");q=inspect(model,full);need(!q.find(1)->connected&&q.tax.cycle.disconnected==7&&q.tax.cycle.eligible==0,"road loss removes tax without removing residents");
// Stock cap applies before consumption, rather than pretending full stores produce.
auto stores=startNativeSession(model,5);act(model,stores,NativeSessionAction::Hunter,6,2);act(model,stores,NativeSessionAction::Road,6,1);act(model,stores,NativeSessionAction::Resume);for(int n=0;n<50;++n)act(model,stores,NativeSessionAction::Tick);q=inspect(model,stores);need(q.foodStock==100&&q.food.cycle.produced==0&&nativeObjectDescription(model,stores,1,q).find("库存已满")!=std::string::npos,"full storage pauses production before meal");
// All legacy versions retain actual byte format and conditions.
for(unsigned v=1;v<=5;++v){auto old=startNativeSession(model,v);act(model,old,NativeSessionAction::House,2,2);act(model,old,NativeSessionAction::Road,1,2);q=inspect(model,old);need(q.find(1)->arrivalPlanned==(v==1),"legacy v1 ignores water and food, others retain version");compareNextMeal(model,old);std::string error;const auto bytes=encodeNativeSession(model,old);auto back=decodeNativeSession(model,bytes,error);need(back&&encodeNativeSession(model,*back)==bytes,"all versions roundtrip unchanged after queries");}
auto rich=model;rich.initialFunds=INT32_MAX;rich.roadCost=rich.houseCost=rich.wellCost=rich.hunterCost=1;auto overflow=startNativeSession(rich,5);build(rich,overflow);act(rich,overflow,NativeSessionAction::Hunter,6,4);act(rich,overflow,NativeSessionAction::Road,5,4);act(rich,overflow,NativeSessionAction::Resume);for(int i=0;i<11;++i)act(rich,overflow,NativeSessionAction::Tick);q=inspect(rich,overflow);need(q.mealBlockedByTax&&q.tax.error=="tax_overflow"&&nativeCityFeedbackSummary(overflow,q).find("不更新")!=std::string::npos,"tax overflow cannot promise arrivals");const auto frozen=encodeNativeSession(rich,overflow);need(!transactNativeSession(rich,overflow,{NativeSessionAction::Tick,overflow.revision}).committed&&frozen==encodeNativeSession(rich,overflow),"forecast tax refusal matches actual atomic refusal");
auto bad=s;bad.funds=-1;need(queryNativeCityFeedback(model,bad).error=="invalid_balance","malformed state not guessed");auto badSeed=model;badSeed.side=0;need(!queryNativeCityFeedback(badSeed,s).error.empty(),"invalid geometry refuses safely");need(!q.find(UINT64_MAX),"out-of-range identity never substitutes object");
}
void real(int argc,char**argv){
 const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return;}
 need(argc==3,"one relative SAV source required");std::string error;auto source=loadNativeSessionSource(ResourceRoot::fromAbsolute(env),argv[2],error);need(bool(source),error.c_str());const auto& model=source->seed;
 unsigned x=0,y=0;bool found=false;for(unsigned py=0;py+5<=model.side&&!found;++py)for(unsigned px=1;px+6<=model.side&&!found;++px){bool flat=true;for(unsigned dy=0;dy<5;++dy)for(int dx=-1;dx<6;++dx)flat&=model.buildable[size_t(py+dy)*model.side+px+dx]&&!model.sourceRoad[size_t(py+dy)*model.side+px+dx];if(flat){x=px;y=py;found=true;}}
 need(found,"real source legal service area");auto s=startNativeSession(model,5);act(model,s,NativeSessionAction::House,x,y);compareNextMeal(model,s);act(model,s,NativeSessionAction::Road,x-1,y);compareNextMeal(model,s);act(model,s,NativeSessionAction::Well,x,y+2);act(model,s,NativeSessionAction::Road,x-1,y+2);compareNextMeal(model,s);act(model,s,NativeSessionAction::Hunter,x+2,y+2);act(model,s,NativeSessionAction::Road,x+3,y+1);compareNextMeal(model,s);act(model,s,NativeSessionAction::Hunter,x+4,y+2);act(model,s,NativeSessionAction::Road,x+5,y+1);act(model,s,NativeSessionAction::Resume);
 for(unsigned n=0;n<2*model.houseCapacity;++n){compareNextMeal(model,s);act(model,s,NativeSessionAction::Tick);}act(model,s,NativeSessionAction::Pause);need(inspect(model,s).find(1)->full,"real model capacity reached");
 const auto bytes=encodeNativeSession(model,s);auto back=decodeNativeSession(model,bytes,error);need(back&&encodeNativeSession(model,*back)==bytes,"query leaves real-source session roundtrip exact");
 std::cout<<"{\"passed\":true,\"real_sources\":1,\"real_source_checks\":"<<checks<<",\"forecast_vs_committed_houses\":"<<paired<<",\"model_capacity\":"<<model.houseCapacity<<",\"GUI\":false,\"original_behavior_claim\":false}\n";
}

void checkpoint(const std::string& path,const std::string& fixtureDir={}){
 const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");
 if(!env||!*env){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return;}
 const auto root=ResourceRoot::fromAbsolute(env);std::string error;
 const auto raw=readNativeSessionFile(path,root.configuredDirectory(),error);need(bool(raw),error.c_str());
 const auto ref=nativeSessionReference(*raw);need(bool(ref),"checkpoint reference");
 auto src=loadNativeSessionSource(root,ref->source,error);need(bool(src),error.c_str());
 need(prepareNativeHouseRuntimeSource(root,*src,error),error.c_str());
 auto state=decodeNativeSession(src->seed,*raw,error);need(bool(state),error.c_str());
 need(validateNativeHouseRuntimeReady(src->seed,*state,*src->runtime).empty(),"input ready before feedback");
 const auto frozen=encodeNativeSession(src->seed,*state);auto q=inspect(src->seed,*state);
 need(q.population.scheduled==nativeUsesImmigration(*state),"policy identity");
 if(q.population.scheduled){
  need(nativeCityFeedbackSummary(*state,q).find("预计新入住")==std::string::npos,"no meal population forecast");
  need(nativeCityFeedbackExplanation(*state,q).find("再按住宅编号")==std::string::npos,"no obsolete arrival explanation");
  need(nativeCategorySummary(src->seed,*state,0,q).find("入住受阻")==std::string::npos,"no water or meal proxy refusal");
  for(const auto& f:q.objects)if(f.kind==NativeObjectKind::House){
   need(!f.foodShortage,"scheduled house not denied by empty food arrival list");
   need(bool(f.housing),"scheduled housing flow exists");
   const auto text=nativeObjectDescription(src->seed,*state,f.id,q);
   need(text.find("更新后入住+1")==std::string::npos,"no fake one-person ETA");
   need(text.find("入住受阻：")==std::string::npos,"no obsolete per-house causes");
  }
  auto authored=*state;
  auto object=std::find_if(authored.objects.begin(),authored.objects.end(),[](const auto& o){return o.kind==NativeObjectKind::House;});
  if(object!=authored.objects.end()){
   // Authored valid cache counterexample over a native checkpoint, not an
   // original-save edit or evidence of new original behavior.
   object->house->houseWater=1;if(object->automation)object->automation->cachedWater=1;
   auto rec=decodeNativeStrictRecord(authored.strictRecords.at(object->id),error);need(bool(rec),"authored strict record");
   rec->houseWater=1;authored.strictRecords.at(object->id)=encodeNativeStrictRecord(*rec);
   auto corrected=inspect(src->seed,authored);const auto* f=corrected.find(object->id);
   need(f&&f->watered&&!f->foodShortage,"known false-food counterexample corrected");
   need(encodeNativeSession(src->seed,*state)==frozen,"authored fixture leaves input state intact");
   auto wrong=authored;rec->migrant=rec->migrant?0:1;wrong.strictRecords.at(object->id)=encodeNativeStrictRecord(*rec);
   need(!queryNativeCityFeedback(src->seed,wrong).error.empty(),"mismatched reservation unevaluable");
   auto unknown=authored;unknown.objects.at(size_t(object-authored.objects.begin())).house->houseWater.reset();
   need(!queryNativeCityFeedback(src->seed,unknown).error.empty(),"unknown cache not coerced dry");
   if(q.population.houses.size()==1){
    // Authored population boundary, with the native receipt equations kept
    // balanced. These are query tests, not claims of original observations.
    const auto boundary=[&](uint32_t people,bool excess){auto s=authored;auto& h=s.objects.at(size_t(object-authored.objects.begin()));const auto delta=people-h.residents;
     h.residents=people;h.house->type=3;h.house->model=0;h.house->imageWord20=int16_t(people);
     auto r=decodeNativeStrictRecord(s.strictRecords.at(h.id),error);need(bool(r),"boundary record");r->model=0;r->residents=int16_t(people);r->free=int16_t(*nativeHouseCapacity(src->seed,s,h))-int16_t(people);s.strictRecords.at(h.id)=encodeNativeStrictRecord(*r);
     s.populationRuntime->admitted+=delta;s.populationRuntime->arrived+=delta;s.populationRuntime->populationArrived+=delta;s.observedDry->arrivedSinceMeal+=delta;
     if(excess)s.observedDry->pendingExcess[h.id]=people-*nativeHouseCapacity(src->seed,s,h);
     return s;};
    const auto capacity=*nativeHouseCapacity(src->seed,authored,*object);
    if(object->residents<=capacity){auto full=boundary(capacity,false);auto fq=inspect(src->seed,full);const auto text=nativeResidentFlowText(*fq.find(object->id));
     need(text.find(fq.find(object->id)->housing->incoming?"当前容量已满":"已满员")!=std::string::npos,"full capacity preserves incoming action priority");}
    if(src->seed.houseCapacities.count(1)&&src->seed.houseCapacities.at(1)>capacity&&state->figureRuntime->immigrantPayloads.empty()){
     const auto excess=boundary(capacity+1,true);const auto eq=inspect(src->seed,excess);need(nativeResidentFlowText(*eq.find(object->id)).find("超容量")!=std::string::npos,"pending excess not mislabeled as ordinary full house");
    }
   }
   for(unsigned water:{0u,1u,2u}){
    auto cache=authored;auto& h=cache.objects.at(size_t(object-authored.objects.begin()));h.house->houseWater=uint8_t(water);h.automation->cachedWater=uint8_t(water);
    auto r=decodeNativeStrictRecord(cache.strictRecords.at(h.id),error);need(bool(r),"water fixture record");r->houseWater=uint8_t(water);cache.strictRecords.at(h.id)=encodeNativeStrictRecord(*r);
    const auto cq=inspect(src->seed,cache);need(cq.find(h.id)->watered==(water!=0),"feedback uses phase28 cache, not decayed service tail or geometry");
   }
   // Food facts remain independent of this policy's housing reservation.
   for(unsigned food:{0u,100u}){auto foodCase=authored;foodCase.foodStock=food;
    foodCase.foodConsumed=std::max(foodCase.foodConsumed,uint64_t(foodCase.lastFood.produced));foodCase.foodProduced=foodCase.foodConsumed+food;
    if(foodCase.foodProduced>uint64_t(nativeFoodCapacity)*(foodCase.ticks/2))continue; // no invented pre-tick production
    const auto fq=inspect(src->seed,foodCase);need(!fq.find(object->id)->foodShortage&&fq.find(object->id)->arrivalPlanned==corrected.find(object->id)->arrivalPlanned,"stock alone cannot deny or invent an incoming actor");}
   if(!fixtureDir.empty()&&state->figureRuntime->immigrantPayloads.empty()){
    need(validateNativeHouseRuntimeReady(src->seed,authored,*src->runtime).empty(),"authored cache checkpoint ready");
    need(writeNativeSessionNew(fixtureDir+"/cache-counterexample.encity",root.configuredDirectory(),encodeNativeSession(src->seed,authored),error),error.c_str());
   }
  }
  if(!state->figureRuntime->immigrantPayloads.empty()){
   auto duplicate=*state;duplicate.figureRuntime->immigrantPayloads.push_back(duplicate.figureRuntime->immigrantPayloads.front());
   need(!queryNativeCityFeedback(src->seed,duplicate).error.empty(),"duplicate actor unevaluable");
   for(unsigned action:{6u,7u,8u}){
    auto stage=*state;auto w=decodeNativeImmigrant(stage.figureRuntime->immigrantPayloads.front(),error);need(bool(w),"actor decode");
    w->action=uint8_t(action);const auto payload=encodeNativeImmigrant(*w,error);need(bool(payload),"authored codec action");stage.figureRuntime->immigrantPayloads.front()=*payload;
    auto sq=inspect(src->seed,stage);const auto* h=sq.find(uint64_t(w->home));
    need(h&&h->housing&&h->housing->incoming&&h->housing->incoming->action==action,"exact actor action preserved");
    need(nativeResidentFlowText(*h).find(action==6?"出发等待":action==7?"途中":"正在入户")!=std::string::npos,"actor stage visible");
    stage.heldWalkerReasons[w->slot]="authored_route_pending";const auto held=inspect(src->seed,stage);
    need(nativeResidentFlowText(*held.find(uint64_t(w->home))).find("迁入等待处理")!=std::string::npos,"held journey not described as active movement");
   }
   if(state->figureRuntime->immigrantPayloads.size()==1){
    auto migrated=*state;auto w=decodeNativeImmigrant(migrated.figureRuntime->immigrantPayloads.front(),error);need(bool(w),"rehousing fixture actor");w->type=13;w->action=7;
    migrated.figureRuntime->immigrantPayloads.clear();auto payload=encodeNativeMigrant(*w,error);need(bool(payload),error.c_str());migrated.figureRuntime->migrantPayloads.push_back(*payload);
    const auto mq=inspect(src->seed,migrated);need(mq.find(uint64_t(w->home))->housing->incoming->type==13,"saved nonempty rehousing is distinct from a new offer");
    auto r=decodeNativeStrictRecord(migrated.strictRecords.at(uint64_t(w->home)),error);need(bool(r),"released rehousing fixture");r->migrant=0;migrated.strictRecords.at(uint64_t(w->home))=encodeNativeStrictRecord(*r);
    w->home=0;w->action=6;payload=encodeNativeMigrant(*w,error);need(bool(payload),error.c_str());migrated.figureRuntime->migrantPayloads.back()=*payload;
    const auto waiting=inspect(src->seed,migrated);need(waiting.population.waitingRehousing==w->employmentAmount&&waiting.population.incomingPeople==0,"unassigned emigrants not counted as reserved or lost");
   }
   auto cancelled=*state;auto w=decodeNativeImmigrant(cancelled.figureRuntime->immigrantPayloads.front(),error);need(bool(w),"cancel actor");
   const auto r=transactNativeHouseRuntime(src->seed,cancelled,{NativeSessionAction::Demolish,cancelled.revision,0,0,0,0,uint64_t(w->home)},*src->runtime);
   need(r.committed,r.error.c_str());const auto cq=inspect(src->seed,cancelled);
   need(!cq.find(uint64_t(w->home))&&cq.population.incomingPeople<q.population.incomingPeople,"actual target cancellation immediately removes reservation from feedback");
   need(validateNativeHouseRuntimeReady(src->seed,cancelled,*src->runtime).empty(),"cancelled query state remains ready");
  }
  for(const auto& f:q.objects)if(f.kind==NativeObjectKind::House&&f.full&&!f.housing->incoming)
   need(nativeResidentFlowText(f).find("已满员")!=std::string::npos,"actual full house reported without a fake arrival");
 }
 for(const auto& f:q.objects)if(f.kind==NativeObjectKind::Well){
  need(bool(f.wellFlow),"v8 well has committed flow facts");const auto text=nativeObjectDescription(src->seed,*state,f.id,q);
  need(text.find("v6不影响供水范围")==std::string::npos&&text.find(nativeUsesCityContext(*state)?"已保存派工":"当前策略派工")!=std::string::npos,"well is not described as old radius-only supply");
  if(f.wellFlow->walker){
   if(f.wellFlow->walker->action==2)need(text.find("返回水井")!=std::string::npos,"actual saved return action is not active delivery");
   if(f.wellFlow->walker->action==1)need(text.find("行路服务")!=std::string::npos,"actual saved outward action visible");
   auto held=*state;held.heldWalkerReasons[f.wellFlow->walker->slot]="authored_route_pending";const auto hq=inspect(src->seed,held);need(nativeWellFlowText(*hq.find(f.id)).find("等待路线确认")!=std::string::npos,"held water walker cannot imply active delivery");
   auto missing=*state;missing.figureRuntime->wellWalkerPayloads.clear();need(!queryNativeCityFeedback(src->seed,missing).error.empty(),"owner cannot refer to a missing water walker");
  }
 }
 // Real source/model compatibility and early v8 are separate from B policy.
 for(unsigned v:{1u,2u,3u,4u,5u,6u}){auto old=startNativeSession(src->seed,v);need(!inspect(src->seed,old).population.scheduled,"old native mode unchanged");}
 auto early=startNativeHouseRuntime(src->seed,*src->runtime,error);need(bool(early),error.c_str());
 need(!inspect(src->seed,*early).population.scheduled,"early v8 has old growth policy");
 if(!fixtureDir.empty()){
  auto old=startNativeSession(src->seed,5);need(writeNativeSessionNew(fixtureDir+"/legacy-v5.encity",root.configuredDirectory(),encodeNativeSession(src->seed,old),error),error.c_str());
 }
 need(encodeNativeSession(src->seed,*state)==frozen,"checkpoint query byte purity");
 std::cout<<"{\"passed\":true,\"checkpoint_feedback_checks\":"<<checks<<",\"scheduled\":"<<(q.population.scheduled?"true":"false")<<",\"incoming_people\":"<<q.population.incomingPeople<<",\"GUI\":false,\"original_behavior_claim\":false}\n";
}

}
int main(int argc,char**argv){try{if((argc==3||argc==4)&&std::string(argv[1])=="--checkpoint"){checkpoint(argv[2],argc==4?argv[3]:"");return 0;}if(argc>1){real(argc,argv);return 0;}run();std::cout<<"{\"passed\":true,\"authored_checks\":"<<checks<<",\"forecast_vs_committed_houses\":"<<paired<<",\"GUI\":false,\"original_data\":false}\n";}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
