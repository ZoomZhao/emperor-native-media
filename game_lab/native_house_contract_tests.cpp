// SPDX-License-Identifier: MIT
// Reuse the established real-source fixture, not the implementation under
// test. Its original main is retained under another symbol and is not run.
#define main nativeDryFixtureOriginalMain
#include "native_house_dry_runtime_tests.cpp"
#undef main
#include "native_house_evolution.hpp"
#include "native_session_interaction.hpp"
namespace {
void stageContract(Test& t,NativeSessionState& s,uint8_t cache,uint8_t tail45,uint8_t tail47,uint8_t delay){
 auto& o=t.home(s);o.house->houseWater=cache;o.house->remaining[45]=tail45;o.house->remaining[47]=tail47;o.house->remaining[52]=delay;
 o.automation->cachedWater=cache;o.automation->delay=delay;auto i=t.record(s);i.houseWater=cache;i.house=o.house->remaining;s.strictRecords[t.house]=encodeNativeStrictRecord(i);s.houseSchedule->phase=39;t.canonicalize(s);
}
NativeSessionState contractFixture(Test& t,bool r4){auto s=fixture(t,true);nativeRuntimePolicy(s,nativeMigrantNonemptyTag);if(r4)nativeRuntimePolicy(s,nativeObservedRewaterTag);return s;}
void contracts(Test& t){
 auto r4=contractFixture(t,true);const auto frozen=encodeNativeSession(t.seed(),r4);std::string e;auto c=resolveNativeHouseRuntimeContract(r4,e);need(c&&e.empty(),"R4 resolves");
 need(c->wireVersion==4&&!c->acceptsPopulatedOriginalSource(),"wire4 never widens original source admission");
 need(c->supportsBuildingPhase(21)&&c->supportsBuildingPhase(22)&&c->supportsBuildingPhase(24)&&!c->supportsBuildingPhase(23)&&!c->supportsBuildingPhase(25)&&!c->supportsBuildingPhase(50),"capability table retains current real phase coverage");
 need(c->supportsLiveFigure(28)&&c->supportsLiveFigure(13)&&!c->supportsLiveFigure(11)&&!c->supportsLiveFigure(12),"codec preparations do not enable population figures");
 need(c->houseGrades().count({4,1})&&!c->houseGrades().count({6,3}),"grade table never guesses an unsupported target");
 need(validateNativeHouseRuntimeReady(t.seed(),r4,*t.src.runtime).empty(),"valid R4 current state ready");need(encodeNativeSession(t.seed(),r4)==frozen,"capability/ready are read-only");
 auto r3=r4;r3.runtimePolicies.erase(std::remove(r3.runtimePolicies.begin(),r3.runtimePolicies.end(),nativeObservedRewaterTag),r3.runtimePolicies.end());auto old=resolveNativeHouseRuntimeContract(r3,e);need(old&&old->wireVersion==3&&!old->observedRewatering,"R3 remains V1 and wire3");
 auto conflict=r4;nativeRuntimePolicy(conflict,nativeWellReturnObservedTag);need(!resolveNativeHouseRuntimeContract(conflict,e)&&e=="well_return_policy_conflict","conflicting return policies rejected by shared resolver");
 auto future=r4;future.runtimePolicies.push_back("native_house_water_only_recovery_policy_v99");need(!resolveNativeHouseRuntimeContract(future,e)&&e=="water_recovery_policy_unsupported","future recovery mode rejected");
 auto bad=r4;auto& home=t.home(bad);home.house->remaining[52]=3;auto rec=t.record(bad);rec.house=home.house->remaining;bad.strictRecords[t.house]=encodeNativeStrictRecord(rec);const auto corrupt=encodeNativeSession(t.seed(),bad);need(!corrupt.empty(),"wire-shaped but semantically mismatched fixture exists");
 need(validateNativeHouseRuntimeReady(t.seed(),bad,*t.src.runtime).find("house_delay_mirror_mismatch_")==0,"load-ready rejects mismatched delay before first tick");need(encodeNativeSession(t.seed(),bad)==corrupt,"failed ready never rewrites a loaded state");
 bad=r4;bad.strictContext.reset();need(validateNativeHouseRuntimeReady(t.seed(),bad,*t.src.runtime)=="observed_dry_context_missing","ready requires observed runtime context");
 bad=r4;bad.heldWalkerReasons[1999]="test_hold";need(!validateNativeHouseRuntimeReady(t.seed(),bad,*t.src.runtime).empty(),"dangling held walker rejected at admission");
 bad=r4;bad.figureRuntime->wellWalkerPayloads.push_back("invalid walker bytes");need(!validateNativeHouseRuntimeReady(t.seed(),bad,*t.src.runtime).empty(),"invalid walker payload rejected at admission");
 auto legacy=startNativeSession(t.seed(),6);const auto bytes=encodeNativeSession(t.seed(),legacy);need(!resolveNativeHouseRuntimeContract(legacy,e)&&e=="runtime_requires_v8"&&encodeNativeSession(t.seed(),legacy)==bytes,"no implicit legacy runtime upgrade");
}
void queryParity(Test& t){std::string error;const auto models=loadNativeHouseModels(t.root,t.seed(),error);need(bool(models),"source-bound models:"+error);
 for(bool r4:{false,true}){auto s=contractFixture(t,r4);stageContract(t,s,1,96,0,3);const auto bytes=encodeNativeSession(t.seed(),s);const auto q=queryNativeHouseEvolution(t.src,s,*models);need(q.error.empty()&&q.find(t.house),"query available");const auto& plan=q.find(t.house)->actualPlan;
  auto run=s;t.tick(run);
  if(r4){need(plan.candidate&&plan.direction==0&&plan.error.empty(),"R4 query selects actual recovery V2");need(plan.candidate->inputs.house==std::optional<std::array<uint8_t,61>>{t.home(run).house->remaining},"query/runtime complete house-tail equality");need(t.home(run).house->remaining[52]==0&&!run.heldHouseReasons.count(t.house),"actual R4 clears delay/hold");}
  else{need(!plan.candidate&&plan.error=="observed_dry_downgrade_rewatered_during_delay_unverified","R3 query respects the sticky legacy isolation");need(t.home(run).house->remaining[52]==3&&run.heldHouseReasons.at(t.house)==plan.error,"R3 query/runtime refusal parity");}
  need(encodeNativeSession(t.seed(),s)==bytes,"query cannot write source state");t.replay(run,r4?"R4-query-next":"R3-query-next");
 }
 auto unknown=contractFixture(t,true);stageContract(t,unknown,2,0,96,3);t.tick(unknown);need(unknown.heldHouseReasons.at(t.house)=="observed_dry_downgrade_rewatered_during_delay_unverified","unknown water source stays isolated");
 stageContract(t,unknown,0,0,0,3);const auto held=queryNativeHouseEvolution(t.src,unknown,*models);need(!held.find(t.house)->actualPlan.candidate,"redrying cannot silently waive an unverified R4 history");t.tick(unknown);need(t.home(unknown).house->remaining[52]==3,"retained unverified delay is not advanced on redrying");
 stageContract(t,unknown,1,96,0,3);const auto clear=queryNativeHouseEvolution(t.src,unknown,*models);need(bool(clear.find(t.house)->actualPlan.candidate),"exact known recovery may clear isolation");t.tick(unknown);need(t.home(unknown).house->remaining[52]==0&&!unknown.heldHouseReasons.count(t.house),"known recovery clears once");t.replay(unknown,"R4-known-after-unknown");
}
void previewParity(Test& t){auto s=contractFixture(t,true);auto frozen=encodeNativeSession(t.seed(),s);std::string lastError;const int side=int(t.seed().side);
 const auto compare=[&](const NativeSessionCommand& cmd){const auto preview=previewNativeSessionCommand(t.seed(),s,cmd,t.src.runtime.get());auto copy=s;const auto actual=transactNativeHouseRuntime(t.seed(),copy,cmd,*t.src.runtime);need(preview.committed==actual.committed&&preview.error==actual.error,"preview/commit same result for action"+std::to_string(int(cmd.action)));need(encodeNativeSession(t.seed(),s)==frozen,"preview leaves all bytes unchanged");lastError=actual.error;return actual.committed;};
 for(auto action:{NativeSessionAction::Road,NativeSessionAction::House,NativeSessionAction::Well,NativeSessionAction::Hunter,NativeSessionAction::RoadPath}){
  bool accepted=false;unsigned attempts=0;
  for(int y=0;y+2<side&&!accepted&&attempts<16;++y)for(int x=0;x+2<side&&!accepted&&attempts<16;++x){bool free=true;for(int dy=0;dy<2;++dy)for(int dx=0;dx<2;++dx)free=free&&t.seed().buildable[size_t(y+dy)*side+x+dx]&&!nativeObjectAt(s,x+dx,y+dy)&&!t.seed().sourceRoad[size_t(y+dy)*side+x+dx];if(!free)continue;
   NativeSessionCommand cmd{action,s.revision,x,y,x+1,y};++attempts;accepted=compare(cmd);
  }
  need(accepted,"at least one valid preview/commit for each current tool");
  NativeSessionCommand outside{action,s.revision,-1,-1,-1,-1};need(!compare(outside),"outside scene rejected without state change");
 }
 NativeSessionCommand demolish{NativeSessionAction::Demolish,s.revision};demolish.objectId=t.house;
 const bool runningAccepted=compare(demolish);need(!runningAccepted&&lastError=="pause_before_demolition","running demolition must retain its normal pause gate:"+lastError);
 std::cout<<"{\"demolition_running_reason\":\""<<lastError<<"\"}"<<std::endl;
 t.action(s,NativeSessionAction::Pause);frozen=encodeNativeSession(t.seed(),s);demolish.expectedRevision=s.revision;
 const bool pausedAccepted=compare(demolish);need(pausedAccepted,"paused house demolition preview must execute actual cleanup validation:"+lastError);
 std::cout<<"{\"demolition_paused_accepted\":true}"<<std::endl;
 auto stale=demolish;stale.expectedRevision=0;need(!compare(stale),"stale preview and commit both reject");
 need(previewNativeSessionCommand(t.seed(),s,demolish).error=="runtime_source_unavailable","v8 preview cannot silently use the legacy path");
 need(previewNativeSessionCommand(t.seed(),s,{NativeSessionAction::Tick,s.revision},t.src.runtime.get()).error=="no_construction_tool","preview never advances simulation");
 auto old=startNativeSession(t.seed(),6);NativeSessionCommand command{NativeSessionAction::Road,old.revision,-1,-1};auto copy=old;const auto a=previewNativeSessionCommand(t.seed(),old,command),b=transactNativeSession(t.seed(),copy,command);need(a.committed==b.committed&&a.error==b.error&&old.rulesVersion==6,"legacy preview keeps its actual legacy command implementation");
}
}
int main(int argc,char** argv){try{
 const char* data=std::getenv("EMPEROR_GAMEDATA_ROOT");auto root=ResourceRoot::fromAbsolute(data?data:"");if(!root.configured()){std::cout<<"{\"SKIP\":\"EMPEROR_GAMEDATA_ROOT unset\"}\n";return 0;}
 std::string error;auto src=loadNativeSessionSource(root,argc>1?argv[1]:"Save/2026102301/历史战役自动保存_1.sav",error);need(bool(src),"source:"+error);need(prepareNativeHouseRuntimeSource(root,*src,error),"runtime:"+error);Test t{*src,root,{},0};contracts(t);queryParity(t);previewParity(t);
 std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"steps\":"<<steps<<",\"replay_boundaries\":"<<replays<<",\"B_population_not_enabled\":true}\n";return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<std::endl;return 1;}}
