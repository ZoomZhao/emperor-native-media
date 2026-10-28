// SPDX-License-Identifier: MIT
// C1 integration contracts over an authored in-memory template only.
// No source SAV, game assets, screenshots, GUI, or original observations.
#include "native_city_context.hpp"
#include "native_house_runtime.hpp"
#include "native_session_feedback.hpp"
#include "native_session_presentation.hpp"
#include <functional>
#include <iostream>
#include <stdexcept>

using namespace game_lab;
namespace {
size_t checks=0,restoredPhases=0,largeCityBytes=0;
void need(bool v,const std::string& why){++checks;if(!v)throw std::runtime_error(why);}
std::string packed(const NativeCityContextState& s){std::string e;auto b=encodeNativeCityContext(s,e);need(bool(b),"city encode:"+e);return *b;}
NativeCityContextState context(const NativeSessionState& s){std::string e;need(bool(s.cityContext),"city payload exists");auto c=decodeNativeCityContext(*s.cityContext,e);need(bool(c),"city decode:"+e);return *c;}
void phase(NativeSessionState& s,uint8_t p){s.houseSchedule->phase=p;auto c=context(s);if(c.goods)c.goods->savedPhase=p;s.cityContext=packed(c);}
OriginalTerrainState flat(){
 OriginalTerrainState s;constexpr auto n=OriginalTerrainState::cells;
 s.image.assign(n,0);s.draw.assign(n,0);s.terrain.assign(n,0);s.feature.assign(n,0);
 s.subTile.assign(n,0);s.randomByte.assign(n,0);s.gF05160.assign(n,0);s.gEAC3F0.assign(n,0);
 s.roadDistance.assign(n,0);s.fertility.assign(n,0);s.gBEBF3A.assign(n,0);s.gBF8A4A.assign(n,0);return s;
}
struct Fixture {
 NativeSessionSeed seed;NativeHouseRuntimeResources resources;
 Fixture(){
  seed.source="authored-city-contract.sav";seed.sourceRevision=17;seed.modelRevision=29;seed.side=12;
  seed.initialFunds=1000;seed.roadCost=2;seed.houseCost=15;seed.wellCost=25;seed.hunterCost=32;
  seed.houseCapacity=7;seed.laborPercent=50;seed.hunterWorkers=2;seed.wellWorkers=4;seed.laborPriority.fill(3);
  seed.buildable.assign(144,1);seed.sourceRoad.assign(144,0);
  for(unsigned y=0;y<12;++y)seed.sourceRoad[y*12+1]=1;
  for(unsigned x=0;x<12;++x)seed.sourceRoad[12+x]=1;
  seed.houseCapacities={{0,7},{1,14},{2,14},{4,21}};
  for(int model=0;model<15;++model){std::array<int32_t,24> row{};row[0]=-99;row[1]=100;row[17]=model==1?14:7;seed.houseModels.emplace(model,row);}
  seed.sourceClock=NativeHouseClockSeed{0,3,2,4,false};seed.waterOverride=false;
  auto& t=resources.strictTemplate;t.sourceRevision=seed.sourceRevision;t.modelRevision=seed.modelRevision;t.side=seed.side;
  t.rebuiltSurface=flat();t.sourceProfile.status=OriginalBuildingStatus::Complete;t.sourceProfile.parsedRecords=4000;t.sourceProfile.savedView=0;
  auto& city=t.emptyCity;city.buildingContext.emplace(OriginalBuildingUpdateScope::AdmittedTicks,true);auto& c=*city.buildingContext;auto& f=city.figures;
  const uint32_t base=((228-seed.side)/2)*229;
  for(unsigned y=0;y<12;++y)for(unsigned x=0;x<12;++x)if(seed.sourceRoad[y*12+x]){t.rebuiltSurface.terrain[base+228*y+x]=0x40;t.rebuiltSurface.roadDistance[base+228*y+x]=255;}
  f.cropBase=base;f.activeSide=seed.side;f.terrain=t.rebuiltSurface.terrain;f.buildingOwners.assign(51984,0);
  f.feature.assign(51984,0);f.randomByte.assign(51984,0);f.elevation.assign(51984,0);f.cellHeads.assign(51984,0);f.buildings.emplace();
  f.passability.emplace();f.passability->spawn.side=seed.side;f.passability->spawn.cropBase=base;
  for(auto& p:f.passability->spawn.spawns)p={1,0};f.passability->cells.assign(51984,0);f.passability->fullMovementMasks=true;
  f.services.emplace();f.services->population=OriginalFigurePopulation{};f.services->directions.assign(500000,0);f.services->serviceGrid.assign(51984,0);
  f.services->randomHistory=OriginalFigureRandomHistory{};f.services->fireCount=0;f.services->criminalCount=0;f.services->allocationSerial=0;
  f.speechCounters=std::array<int32_t,359>{};f.speechCounts=std::array<uint32_t,359>{};f.speechCounts->fill(1);
  f.soundEffectsEnabled=false;f.buildingEditMode=false;f.waterOverride=false;f.festivalVenue=0;f.roadblockGate=0;
  for(size_t slot=0;slot<2000;++slot){OriginalFigure dead;dead.slot=slot;f.figures.emplace(slot,dead);}
  for(size_t slot=1999;slot>0;--slot)f.freeSlots.push_back(slot);
  c.phase=0;c.cycle=3;c.monthPart=2;c.month=4;c.cropBase=base;c.activeSide=12;c.terrain=f.terrain;
  c.terrainOwners=std::vector<uint16_t>(51984,0);c.terrainRandom=std::vector<uint8_t>(51984,0);c.roadEndpoints=std::array<int16_t,4>{1,0,1,11};
  c.resourceGrowth.emplace();c.resourceGrowth->surface=t.rebuiltSurface;c.resourceGrowth->owners.assign(51984,0);
  c.buildingInputs.emplace();c.buildings.emplace();c.productionInputs.emplace();c.buildingLinks.emplace();c.houses.emplace();c.formations.emplace();
  c.figureServices=f.services;c.figureSpeechCounters=f.speechCounters;c.figureSpeechCounts=f.speechCounts;c.figureModels=f.models;c.figureStationary.emplace();
  c.buildingHookCountdown=0;c.highestBuildingSlot=0;c.highestEverBuildingSlot=0;c.buildingRegistrySize=4000;c.goodsRegistryLast=0;
  c.labor.emplace();c.labor->sentiment=50;c.labor->health=50;c.labor->difficulty=2;c.labor->wageBand=3;c.labor->populationNotices=std::array<bool,10>{};
  c.cityEmployment.emplace();c.cityEmployment->shortageNotice=false;c.cityEmployment->unemploymentNotice=false;for(auto& row:c.cityEmployment->rows)row.priority=3;
  c.unemploymentTrend=0;c.shortageNoticeCountdown=0;c.elapsedYears=0;c.scenarioStartYears=0;c.populationTotal=0;c.populationPeak=0;c.treasury=1000;
  c.employmentModel=OriginalBuildingEmployment{{33,2},{72,4}};c.unavailableResources=std::array<int16_t,36>{};c.empireProduction=std::array<bool,36>{};c.buildingUnavailable.emplace();
  c.accounting.emplace();c.cityDerived.emplace();c.statistics.emplace();c.statistics->currentFaction=0;c.statistics->religionBonusEnabled=false;
  c.religion.emplace();c.religion->previousRating=0;c.religion->selected=-1;
  c.cycleTail.emplace();c.cycleTail->alternateMode=false;c.cycleTail->healthTarget=50;c.cycleTail->smoothedHealth=50;c.cycleTail->waterMode=0;
  c.trade.emplace();for(unsigned n=0;n<22;++n)c.trade->partners[n].faction=uint8_t(n);
  c.buildingTotals.emplace();c.goods.emplace();c.goods->policies.resize(1);c.goods->storageSearch=std::array<uint8_t,36>{};c.goods->industrySearch=std::array<uint8_t,36>{};c.goods->godGate=0;
  c.goodsRecords=std::vector<OriginalBuildingGoodsRecord>(1);c.storageRoadStatus.emplace();c.serviceCoverage=std::array<int32_t,11>{};c.scenarioMonths=0;c.scenarioFoodOverride=0;
  c.houseModels=seed.houseModels;c.houseModelCapacity=std::map<int,int>{{0,7},{1,14},{2,14},{4,21}};c.houseTaxModel=std::map<int,int>{{0,1},{1,1}};
  c.evolutionDisabled=false;c.forceHouseEvolution=false;c.monthlyServices=false;c.scenarioFigurePlacementCheck=false;
  c.additionalWalkers=false;c.industryCarts=false;c.stallSuppliers=false;
  f.goods=c.goods;city.rebuiltMovementTerrain=t.rebuiltSurface.terrain;
  // Saved per-record contexts carry local scalar inputs, never the city-wide
  // registries. Phase36 calls the local API; an empty shared registry would
  // correctly refuse because it does not contain that record's own slot.
  OriginalBuildingUpdateContext local(OriginalBuildingUpdateScope::AdmittedTicks,true);
  local.phase=0;local.cycle=3;local.monthPart=2;local.month=4;local.terrainFlags=0;local.typeDisabled=false;
  for(uint16_t type:{2,3,4,5,7,33,72})t.contextPrototypes.emplace(type,local);
 }
 NativeSessionState start(bool c1=true) const{
  std::string e;auto s=startNativeHouseRuntime(seed,resources,e,NativeWellReturnPolicy::Hold,true,true,true,true,c1);
  need(bool(s),"authored runtime factory:"+e);return *s;
 }
 std::string bytes(const NativeSessionState& s) const{auto b=encodeNativeSession(seed,s);need(!b.empty(),"public session encodes:"+validateNativeSession(seed,s));return b;}
 NativeStrictConversion strict(const NativeSessionState& s)const{auto c=nativeToStrict(seed,s,resources.strictTemplate,nativeStrictPersisted(s));need(bool(c.city),"authored bridge:"+c.refusal);return c;}
 void action(NativeSessionState& s,NativeSessionAction a,int x=0,int y=0)const{auto r=transactNativeHouseRuntime(seed,s,{a,s.revision,x,y},resources);need(r.committed,"command:"+r.error);}
 void canonicalize(NativeSessionState& s)const{auto c=strict(s);auto p=strictToNative(seed,s,*c.city,c.isolation,resources.strictTemplate);need(bool(p.state),"authored canonicalize:"+p.refusal);s=*p.state;}
};
// Independent frozen pre-C runtime schema, without calling its field visitor.
std::string zeroes(size_t n){std::string s;while(n--)s+="0 ";return s;}
NativeSessionState oldState(unsigned v){NativeSessionState s;s.rulesVersion=8;if(v>=2){s.figureRuntime.emplace();s.observedDry.emplace();s.runtimePolicies={nativeObservedDryTag};}if(v>=3)s.runtimePolicies.push_back(nativeMigrantNonemptyTag);if(v>=4)s.runtimePolicies.push_back(nativeObservedRewaterTag);if(v>=5){s.runtimePolicies.push_back(nativeImmigrationTag);s.populationRuntime.emplace();s.populationRuntime->labor.difficulty=0;s.populationRuntime->labor.wageBand=0;}return s;}
std::string oldGolden(unsigned v){
 if(v==1)return "RUNTIME8 1\nschedule 0 \nfigures 0 \nhook 0 \nheldHouses 0 \nheldWalkers 0 \nstrictRecords 0 \nstrictContext 0 \npolicies 0 \nobjects 0\nENDRUNTIME8\n";
 std::string s="RUNTIME8 "+std::to_string(v)+"\nschedule 0 \nfigures 1 walkers 0 \ncellHeads 0 \nfreeFIFO 0 \nhighest 0 \nservices formation "+zeroes(200)+"\nrouteOwners "+zeroes(1000)+"\ndirections 0 \nrandomFirst 0 \nrandomSecond 0 \nring 0 \ncursor 0 \nprevious 0 \nfires 0 \ncriminals 0 \nallocation 0 \nserviceGrid 0 \n\nspeechCounters "+zeroes(359)+"\nspeechCounts "+zeroes(359)+"\nspeechChoices 0 \n\nhook 0 \nheldHouses 0 \nheldWalkers 0 \nstrictRecords 0 \nstrictContext 0 \npolicies "+std::to_string(v-1)+" ";
 const char* tags[]={"native_house_observed_dry_type4_to3_v1","native_migrant_nonempty_rehousing_policy_v1","native_house_water_only_recovery_policy_v1","native_population_immigrant11_policy_v1"};
 for(unsigned n=0;n<v-1;++n)s+='"'+std::string(tags[n])+"\" ";
 s+="\nobservedDry 1 pendingExcess 0 \ndepartedSinceMeal 0 \narrivedSinceMeal 0 \n\nmigrantPayloads 0 \n";
 if(v==5){s+="populationRuntime 1 labor ";for(const char* n:{"sentiment","health","migration","incoming","outgoing","inRemainder","outRemainder","inDelay","outDelay","arrivals","departures","unplaced","yearArrivals","spacing","statisticsPending","populationSum","sampleCount","averagePopulation","growthCursor","deathCursor","lastPopulationDelta","excessDepartureTotal","difficulty","wageBand","populationNotices","hostileCount"})s+=std::string(n)+" 0 \n";
  s+="formationLabor 0 0 0 0 \n\nemployment rows ";for(unsigned row=0;row<10;++row)for(const char* n:{"demand","allocated","preferred","preferredAllocated","priority"})s+=std::string(n)+" 0 \n";
  s+="\n";for(const char* n:{"available","employed","unemployed","unemploymentPercent","shortage","shortageNotice","unemploymentNotice"})s+=std::string(n)+" 0 \n";
  s+="\n";for(const char* n:{"admitted","arrived","unplaced","populationArrived","populationDeparted"})s+=std::string(n)+" 0 \n";s+="\nimmigrantPayloads 0 \n";
 }
 return s+"objects 0\nENDRUNTIME8\n";
}
std::string runtimeWire(const NativeSessionState& s){std::ostringstream o;native_runtime_wire::encode(o,s);need(bool(o),"runtime wire encodes");return o.str();}
void legacy(){for(unsigned v=1;v<=5;++v){auto s=oldState(v);const auto b=oldGolden(v);need(runtimeWire(s)==b,"wire1..5 independent golden:"+std::to_string(v));
 auto stale=s;stale.cityContext=packed(NativeCityContextState{});std::istringstream in(b);need(native_runtime_wire::decode(in,stale),"old runtime decode");need(!stale.cityContext&&!nativeUsesCityContext(stale)&&runtimeWire(stale)==b,"old runtime clears stale C and never upgrades");
 auto hidden=s;hidden.cityContext=packed(NativeCityContextState{});std::ostringstream out;native_runtime_wire::encode(out,hidden);need(out.fail()&&out.str().empty(),"untagged C cannot hide in legacy wire");}}
void stamp(NativeCityContextState& c,int n){
 c.accounting->taxReceived=n;c.cityDerived->food=n+1;c.statistics->emptyCycles=n+2;c.religion->worship[1].entries[1]={0,150,1,4,n%100};
 c.cycleTail->healthTarget=n%100;c.cycleTail->productionHistory={3,n+3,2,n+4};c.trade->partners[2].reserves[0]=n+5;
 c.buildingTotals->producers[35]=n+6;c.goods->warehouse[35]=uint32_t(n+7);c.goods->trade5[35]=uint32_t(n+8);c.goods->trade7[35]=uint32_t(n+12);c.goodsRecords->at(0).mode[19]=2;
 c.serviceCoverage->at(4)=n%100;c.millAvailable[2]=n+9;c.millCandidates[99]=int16_t(n%100);c.unemploymentTrend=n%100;c.shortageNoticeCountdown=n%7;c.dispatchedProduction[3]=n+10;c.foodCalls=unsigned(n+11);
}
std::string goodsOnlyWire(const OriginalFigureGoodsServices& goods){
 OriginalBuildingUpdateContext c(OriginalBuildingUpdateScope::AdmittedTicks,true);c.goods=goods;return packed(captureNativeCityContext(c));
}
void goodsSourceFactory(Fixture& t){
 auto missing=t.resources;auto& empty=missing.strictTemplate;
 empty.emptyCity.figures.goods.reset();empty.emptyCity.buildingContext->goods.reset();empty.cityGoodsSource.reset();
 std::string e;auto rejected=startNativeHouseRuntime(t.seed,missing,e,NativeWellReturnPolicy::Hold,true,true,true,true,true);
 need(!rejected&&e=="city_goods_source_required","missing immutable Goods input has named C1 refusal");
 auto old=startNativeHouseRuntime(t.seed,missing,e,NativeWellReturnPolicy::Hold,true,true,true,true,false);
 need(old&&!old->cityContext&&nativeHouseRuntimeWireVersion(*old)==5,"B factory never starts requiring C-only Goods input");
 OriginalFigureGoodsServices source;source.policies.resize(2);source.policies[1].accept[4]=2;source.policies[1].trade[7]=5;source.policies[1].limit[4]=1200;
 source.unavailable[4]=1;source.industryPriority[3]=17;source.godGate=3;source.savedPhase=7;
 source.warehouse[4]=501;source.warehouse[35]=801;source.trade5[7]=602;source.trade7[9]=703;
 const auto immutable=goodsOnlyWire(source);auto expected=source;expected.warehouse.fill(0);expected.trade5.fill(0);expected.trade7.fill(0);expected.savedPhase=t.seed.sourceClock->phase;
 for(bool direct:{false,true}){auto resources=missing;auto& tpl=resources.strictTemplate;
  if(direct)tpl.emptyCity.figures.goods=source;else tpl.cityGoodsSource=source;
  auto state=startNativeHouseRuntime(t.seed,resources,e,NativeWellReturnPolicy::Hold,true,true,true,true,true);
  need(bool(state),std::string(direct?"figure Goods source":"immutable-only Goods fallback")+" constructor:"+e);
  const auto saved=context(*state);need(saved.goods&&goodsOnlyWire(*saved.goods)==goodsOnlyWire(expected),"new owned Goods clears all36 stock columns and preserves source policy inputs");
  need(state->foodStock==0&&state->foodProduced==0&&state->foodConsumed==0,"discarded original Goods stock cannot credit native food ledger");
  need(std::count(state->runtimePolicies.begin(),state->runtimePolicies.end(),"native_city_goods_empty_owned_pool_v1")==1,"empty owned Goods constructor is explicitly tagged");
  const auto& retained=direct?tpl.emptyCity.figures.goods:tpl.cityGoodsSource;
  need(retained&&goodsOnlyWire(*retained)==immutable&&!tpl.emptyCity.buildingContext->goods,"factory never mutates immutable Goods input or absent legacy mirror");
  if(!direct)need(!tpl.emptyCity.figures.goods,"fallback success does not retrofit Goods into B figure template");
  const auto rebuilt=nativeToStrict(t.seed,*state,tpl,nativeStrictPersisted(*state));need(bool(rebuilt.city),"fallback state bridge:"+rebuilt.refusal);
  need(rebuilt.city->figures.goods&&rebuilt.city->buildingContext->goods&&goodsOnlyWire(*rebuilt.city->figures.goods)==goodsOnlyWire(expected)&&goodsOnlyWire(*rebuilt.city->buildingContext->goods)==goodsOnlyWire(expected),"fallback restored canonical and shared Goods are identical");
 }
 need(goodsOnlyWire(source)==immutable,"independent authored Goods source remains unchanged");
}
void noticeFactory(Fixture& t){
 auto resources=t.resources;auto& source=*resources.strictTemplate.emptyCity.buildingContext->cityEmployment;source.shortageNotice.reset();source.unemploymentNotice.reset();std::string e;
 auto old=startNativeHouseRuntime(t.seed,resources,e,NativeWellReturnPolicy::Hold,true,true,true,true,false);
 need(old&&old->populationRuntime&&!old->populationRuntime->employment.shortageNotice.has_value()&&!old->populationRuntime->employment.unemploymentNotice.has_value(),"same missing-notice template keeps B process inputs absent");
 auto current=startNativeHouseRuntime(t.seed,resources,e,NativeWellReturnPolicy::Hold,true,true,true,true,true);
 need(current&&current->populationRuntime&&current->populationRuntime->employment.shortageNotice==std::optional<bool>(true)&&current->populationRuntime->employment.unemploymentNotice==std::optional<bool>(true)&&std::count(current->runtimePolicies.begin(),current->runtimePolicies.end(),"native_city_employment_notice_native_init_v1")==1,"C1 fills missing notices with explicitly tagged native true inputs");
 auto known=t.start();need(known.populationRuntime->employment.shortageNotice==std::optional<bool>(false)&&known.populationRuntime->employment.unemploymentNotice==std::optional<bool>(false)&&std::count(known.runtimePolicies.begin(),known.runtimePolicies.end(),"native_city_employment_notice_native_init_v1")==0,"known false process notices remain false without native backfill");
}
void poisonRebuild(Fixture& t){auto s=t.start();auto saved=context(s);stamp(saved,71);
 saved.goods->policies[0].accept[4]=2;saved.goods->policies[0].trade[5]=7;saved.goods->policies[0].limit[4]=800;saved.goods->industryPriority[3]=17;saved.goods->unavailable[4]=1;
 s.cityContext=packed(saved);const auto before=t.bytes(s);
 auto poisoned=t.resources.strictTemplate;auto poison=captureNativeCityContext(*poisoned.emptyCity.buildingContext);stamp(poison,13);applyNativeCityContext(poison,*poisoned.emptyCity.buildingContext);poisoned.emptyCity.figures.goods=poison.goods;
 std::string restoreError;auto restored=decodeNativeSession(t.seed,before,restoreError);need(restored&&restored->cityContext==s.cityContext,"restore retained C inventory payload:"+restoreError);
 const auto decoded=restored?context(*restored):NativeCityContextState{};
 need(decoded.goods&&decoded.goods->warehouse[35]==78&&decoded.goods->trade5[35]==79&&decoded.goods->trade7[35]==83&&decoded.goods->policies[0].accept[4]==2&&decoded.goods->policies[0].trade[5]==7&&decoded.goods->industryPriority[3]==17,"decode preserves nonzero warehouse/trade5/trade7 and policy/priority");
 const auto resumed=nativeToStrict(t.seed,*restored,poisoned,nativeStrictPersisted(*restored));need(bool(resumed.city),"decoded inventory rebuild:"+resumed.refusal);
 need(resumed.city->figures.goods&&resumed.city->buildingContext->goods&&goodsOnlyWire(*resumed.city->figures.goods)==goodsOnlyWire(*saved.goods)&&goodsOnlyWire(*resumed.city->buildingContext->goods)==goodsOnlyWire(*saved.goods),"restore/rebuild does not repeat new-city stock clearing and keeps complete Goods policy inputs");
 const auto c=nativeToStrict(t.seed,s,poisoned,nativeStrictPersisted(s));need(bool(c.city),"poisoned template rebuild:"+c.refusal);
 need(packed(captureNativeCityContext(*c.city->buildingContext))==s.cityContext,"all C fields beat template poison");
 need(c.city->figures.goods->warehouse[35]==78&&c.city->buildingContext->goods->warehouse[35]==78,"canonical and mirror goods restored");
 auto p=strictToNative(t.seed,s,*c.city,c.isolation,poisoned);need(bool(p.state)&&p.state->cityContext==s.cityContext,"C survives strict projection");need(t.bytes(s)==before,"rebuild is read-only");
 for(unsigned mode=0;mode<6;++mode){auto bad=s;if(mode==0)bad.cityContext.reset();if(mode==1)bad.runtimePolicies.erase(std::remove(bad.runtimePolicies.begin(),bad.runtimePolicies.end(),nativeCityContextTag),bad.runtimePolicies.end());if(mode==2)bad.runtimePolicies.push_back("native_city_context_contract_v99");if(mode==3)bad.cityContext="NATIVE-CITY-CONTEXT 99\n";if(mode==4){auto wrong=context(bad);wrong.goods->savedPhase=(bad.houseSchedule->phase+1)%51;bad.cityContext=packed(wrong);}if(mode==5)bad.runtimePolicies.push_back(nativeCityContextTag);need(!validateNativeSession(t.seed,bad).empty()&&encodeNativeSession(t.seed,bad).empty(),"invalid C policy/payload/phase/duplicate refuses publication");}
 auto old=t.start(false);need(nativeHouseRuntimeWireVersion(old)==5&&!old.cityContext,"B factory remains wire5");const auto bytes=t.bytes(old);std::string e;auto back=decodeNativeSession(t.seed,bytes,e);need(back&&t.bytes(*back)==bytes&&!back->cityContext,"old public archive never upgrades");
}
void largeOuterWire(Fixture& t){
 auto s=t.start();auto saved=context(s);saved.trade.emplace();
 // Typed inactive child arrays are preserved payload, not active diplomacy.
 for(auto& p:saved.trade->partners){p.alive=0;p.relationTimers.resize(22);for(auto& child:p.relationTimers)for(auto& timers:child)timers.fill(INT32_MIN);}
 s.cityContext=packed(saved);largeCityBytes=s.cityContext->size();need(largeCityBytes>262144,"legitimate C payload exceeds old generic string limit");
 const auto bytes=t.bytes(s);std::string e;auto back=decodeNativeSession(t.seed,bytes,e);need(back&&back->cityContext==s.cityContext&&t.bytes(*back)==bytes,"wire6 outer reader preserves large legal city context exactly:"+e);
 std::ostringstream encoded;encoded<<std::quoted(std::string(262145,'x'));std::istringstream in(encoded.str());native_runtime_wire::Reader reader{in};std::string ordinary;reader.value(ordinary);
 need(!reader.good,"large C allowance does not widen ordinary reason/string budget");
}
void goodsSamePass(Fixture& t){
 auto s=t.start();auto saved=context(s);saved.goods->warehouse[35]=91;s.cityContext=packed(saved);const auto before=t.bytes(s);
 auto c=t.strict(s);auto changed=*c.city;changed.figures.goods->warehouse[35]=123;
 need(changed.buildingContext->goods->warehouse[35]==91,"independently stale building goods mirror staged");
 auto projected=strictToNative(t.seed,s,changed,c.isolation,t.resources.strictTemplate);need(bool(projected.state),"canonical figure goods capture:"+projected.refusal);
 need(context(*projected.state).goods->warehouse[35]==123&&t.bytes(s)==before,"figure goods wins at publication without mutating base");
 auto rebuilt=t.strict(*projected.state);need(rebuilt.city->figures.goods->warehouse[35]==123&&rebuilt.city->buildingContext->goods->warehouse[35]==123,"post-figure goods mirror survives reconstruction");
 s=*projected.state;s.paused=false;phase(s,1);saved=context(s);saved.goods->storageSearch->at(2)=7;saved.goods->industrySearch->at(3)=6;s.cityContext=packed(saved);
 t.action(s,NativeSessionAction::Tick);auto after=context(s);need(after.goods->savedPhase==2&&after.goods->storageSearch->at(2)==0&&after.goods->industrySearch->at(3)==0,"new phase2 cache reset persists from same ordered pass");
 need(after.goods->warehouse[35]==123,"process-cache reset preserves stock accounting");
}
NativeSessionState occupied(Fixture& t,uint64_t& well){
 auto s=t.start();t.action(s,NativeSessionAction::House,2,3);const auto id=s.objects.back().id;t.action(s,NativeSessionAction::Well,2,6);well=s.objects.back().id;
 auto& o=*std::find_if(s.objects.begin(),s.objects.end(),[&](const auto& x){return x.id==id;});
 // Synthetic population history at an exact supported grade, not an observed
 // original first-arrival or type4 allocation experiment.
 o.residents=14;o.house->type=4;o.house->model=1;o.house->imageWord20=14;o.house->houseWater=0;o.house->remaining[13]=50;o.house->remaining[15]=1;
 std::string e;auto i=decodeNativeStrictRecord(s.strictRecords.at(id),e);need(bool(i),"house record");i->model=1;i->residents=14;i->free=0;i->house=o.house->remaining;i->distance=1;i->roadX=1;i->roadY=3;i->available=1;i->operational=100;i->enabled=1;s.strictRecords[id]=encodeNativeStrictRecord(*i);
 auto w=decodeNativeStrictRecord(s.strictRecords.at(well),e);need(bool(w),"well record");w->distance=1;w->roadX=1;w->roadY=6;w->available=1;w->operational=100;w->enabled=1;s.strictRecords[well]=encodeNativeStrictRecord(*w);
 auto c=decodeNativeStrictContext(*s.strictContext,e);need(bool(c),"population mirror");c->population->total=c->population->peak=14;*c->nativePopulation=14;s.strictContext=encodeNativeStrictContext(*c);
 auto& p=*s.populationRuntime;p.admitted=p.arrived=p.populationArrived=14;s.observedDry->arrivedSinceMeal=14;t.canonicalize(s);return s;
}
void replayPhase(Fixture& t,NativeSessionState s,uint8_t p){phase(s,p);s.paused=false;const auto b=t.bytes(s);std::string e;auto reloaded=decodeNativeSession(t.seed,b,e);need(bool(reloaded),"phase reload:"+e);
 auto a=transactNativeHouseRuntime(t.seed,s,{NativeSessionAction::Tick,s.revision},t.resources);auto r=transactNativeHouseRuntime(t.seed,*reloaded,{NativeSessionAction::Tick,reloaded->revision},t.resources);
 need(a.committed&&r.committed,"phase replay "+std::to_string(p)+":"+a.error+":"+r.error);need(t.bytes(s)==t.bytes(*reloaded),"next whole state byte equality at phase"+std::to_string(p));++restoredPhases;
 auto c=t.strict(s);need(c.city->figures.goods&&c.city->buildingContext->goods&&c.city->figures.goods->savedPhase==s.houseSchedule->phase&&c.city->buildingContext->goods->savedPhase==s.houseSchedule->phase,"goods phase/mirror advance in same pass");
}
void foodAndRecovery(Fixture& t){uint64_t well=0;auto s=occupied(t,well);auto saved=context(s);saved.goods->warehouse[35]=91;s.cityContext=packed(saved);
 const auto inventory=s.objects.front().house->remaining;const auto stock=s.foodStock;phase(s,17);s.paused=false;
 t.action(s,NativeSessionAction::Tick);auto c=context(s);need(c.cityDerived&&c.cityDerived->food==50&&c.cityDerived->foodConsumption==3&&c.cityDerived->foodMonths==16,"normalized17 reads food word0, not neighbour material");
 need(s.objects.front().house->remaining==inventory&&s.foodStock==stock&&s.foodProduced==0&&s.foodConsumed==0,"17 aggregates without consuming either inventory");
 need(c.goods->warehouse[35]==91&&c.goods->savedPhase==18,"17 preserves high resource and publishes next phase");
 for(uint8_t p:{uint8_t(12),uint8_t(17),uint8_t(25),uint8_t(21),uint8_t(22),uint8_t(28),uint8_t(35),uint8_t(36),uint8_t(50)})replayPhase(t,s,p);
 std::string e;const auto contract=resolveNativeHouseRuntimeContract(s,e);need(bool(contract),"C contract");
 for(uint32_t p:{12u,17u,25u})need(contract->supportsBuildingPhase(p),"C1 three admitted city phases");
 for(uint32_t p:{1u,8u,13u,16u,18u,20u,32u,33u,50u})need(!contract->supportsBuildingPhase(p),"unclosed city/calendar phase stays unsupported");
 for(uint8_t p:{uint8_t(8),uint8_t(13),uint8_t(16),uint8_t(18),uint8_t(33)}){auto held=s;phase(held,p);const auto before=context(held);t.action(held,NativeSessionAction::Tick);auto after=context(held);need(after.goodsPoolCalls==before.goodsPoolCalls&&after.statisticsCalls==before.statisticsCalls&&after.storageCalls==before.storageCalls&&after.religion->calls==before.religion->calls&&after.buildingTotals->calls==before.buildingTotals->calls,"C1 never silently enables another city service");}
 auto frozen=s;phase(frozen,50);const auto clock=*frozen.houseSchedule;t.action(frozen,NativeSessionAction::Tick);need(frozen.houseSchedule->phase==0&&frozen.houseSchedule->monthPart==clock.monthPart&&frozen.houseSchedule->cycle==clock.cycle&&frozen.houseSchedule->month==clock.month,"C1 retains explicit unknown calendar freeze");need(context(frozen).cycleTail->calls==context(s).cycleTail->calls,"C1 cannot execute health/month tail");
}
void staffing(Fixture& t){uint64_t well=0;auto s=occupied(t,well);s.populationRuntime->employment.available=2;phase(s,25);s.paused=false;t.action(s,NativeSessionAction::Tick);
 const auto scheduled=planNativeLabor(t.seed,s);const auto* job=scheduled.find(well);need(job&&job->workers==2,"phase25 saves partial scheduled Well workers");auto saved=context(s);need(saved.workers.at(well)==2,"scheduled snapshot matches actual worker result");
 auto legacy=s;legacy.cityContext.reset();legacy.runtimePolicies.erase(std::remove(legacy.runtimePolicies.begin(),legacy.runtimePolicies.end(),nativeCityContextTag),legacy.runtimePolicies.end());auto old=planNativeLabor(t.seed,legacy);need(old.find(well)&&old.find(well)->workers==4,"old v6 would allocate a different worker count");
 const auto before=t.bytes(s);for(unsigned n=0;n<4;++n){auto plan=planNativeLabor(t.seed,s);auto feedback=queryNativeCityFeedback(t.seed,s);need(plan.error.empty()&&plan.find(well)->workers==2&&feedback.error.empty()&&feedback.find(well)&&feedback.find(well)->workers==2,"queries retain scheduled workers");}need(t.bytes(s)==before,"repeated query does not dispatch or mutate");
 auto c=t.strict(s);need(c.city->buildings.at(size_t(well)).state.inputs.workers==2&&c.city->figures.buildings->at(size_t(well)).workers==2,"rebuild retains worker record and link");t.canonicalize(s);need(t.bytes(s)==before,"strict projection does not overwrite phase25 with v6");replayPhase(t,s,19);
 auto mismatch=s;auto bad=context(mismatch);bad.workers[size_t(well)]=3;mismatch.cityContext=packed(bad);
 need(!nativeToStrict(t.seed,mismatch,t.resources.strictTemplate,nativeStrictPersisted(mismatch)).city,"worker snapshot cannot silently disagree with strict record");
}
void rollback(Fixture& t){uint64_t well=0;auto s=occupied(t,well);s.paused=false;phase(s,19);
 for(unsigned n=0;n<3;++n)t.action(s,NativeSessionAction::Tick);need(s.ticks==3,"rollback fixture is immediately before tax and meal");phase(s,17);
 auto broken=t.resources;broken.strictTemplate.emptyCity.buildingContext->scenarioFoodOverride=1;
 const auto before=t.bytes(s);auto r=transactNativeHouseRuntime(t.seed,s,{NativeSessionAction::Tick,s.revision},broken);
 need(!r.committed&&!r.error.empty(),"unsupported food scenario refuses transaction");need(t.bytes(s)==before,"failed strict phase rolls back tax/food/revision/all city bytes");
 auto good=s;auto okay=transactNativeHouseRuntime(t.seed,good,{NativeSessionAction::Tick,good.revision},t.resources);need(okay.committed&&good.taxCycles==1&&good.taxReceived==14&&good.ticks==4,"same valid boundary really assesses tax before C17");
}
void materialScheduleIsolation(){
 Fixture t;auto& model=t.seed.houseModels.at(2);model.fill(0);model[0]=-1;model[1]=1;model[9]=1;model[17]=14;
 // Complete immutable inputs for two full authored scheduler loops. Eight
 // available workers against a20-worker Well exercise real partial staffing.
 t.seed.wellWorkers=20;auto& source=*t.resources.strictTemplate.emptyCity.buildingContext;
 source.houseModels=t.seed.houseModels;source.employmentModel->at(72)=20;source.walkersDisabled=false;source.buildingModels.emplace();
 for(uint16_t type:{2,3,4,5,7,33,72})source.buildingModels->emplace(type,std::array<int32_t,13>{});
 uint64_t well=0;auto s=occupied(t,well);const uint64_t heldHouse=s.objects.front().id;t.action(s,NativeSessionAction::House,2,9);const uint64_t otherHouse=s.objects.back().id;
 std::string e;
 for(auto& o:s.objects)if(o.house){const bool high=o.id==heldHouse;o.residents=high?14:7;o.house->type=high?5:3;o.house->model=high?2:0;o.house->imageWord20=int16_t(o.residents);o.house->remaining.fill(0);
  need(originalHouseSetGoodsWord(o.house->remaining.data(),o.house->remaining.size(),0,high?50:0)&&originalHouseSetGoodsWord(o.house->remaining.data(),o.house->remaining.size(),1,high?3:0),"authored independent food/material words");
  auto i=decodeNativeStrictRecord(s.strictRecords.at(o.id),e);need(bool(i),"material fixture record:"+e);i->model=int16_t(o.house->model);i->residents=int16_t(o.residents);i->free=0;i->house=o.house->remaining;i->modelDesirability=0;i->houseWater=0;i->houseInUse=1;i->roadX=1;i->roadY=int16_t(o.y);i->distance=1;i->available=1;i->enabled=1;i->operational=100;s.strictRecords[o.id]=encodeNativeStrictRecord(*i);
 }
 auto extra=decodeNativeStrictContext(*s.strictContext,e);need(bool(extra),"material population context");extra->population->total=extra->population->peak=21;*extra->nativePopulation=21;s.strictContext=encodeNativeStrictContext(*extra);
 auto& population=*s.populationRuntime;population.admitted=population.arrived=population.populationArrived=21;population.labor.sentiment=36;s.observedDry->arrivedSinceMeal=21;
 s.houseSchedule->monthPart=0;phase(s,39);t.canonicalize(s);
 const auto view=t.strict(s);const auto& high=view.city->buildings.at(size_t(heldHouse)).state;const auto baselineGoods=originalHouseGoodsFromPayload(*high.inputs.house);
 need(high.building.type==5&&high.inputs.model==2&&baselineGoods&&baselineGoods->at(0)==50&&baselineGoods->at(1)==3,"legal type5/model2 nonzero material counterexample");
 NativeSessionSource querySource{t.seed,OriginalSaveResult{},std::make_shared<const NativeHouseRuntimeResources>(t.resources)};
 NativeHouseModels queryModels;queryModels.sourceRevision=t.seed.sourceRevision;queryModels.modelRevision=t.seed.modelRevision;queryModels.difficulty=2;queryModels.house=t.seed.houseModels;queryModels.building=*source.buildingModels;
 const auto queryMaterial=[&](const NativeSessionState& state,bool held){const auto before=t.bytes(state);const auto report=queryNativeHouseEvolution(querySource,state,queryModels);
  need(report.error.empty()&&report.find(heldHouse)&&report.find(otherHouse),"pure C1 material query projects both houses:"+report.error);const auto& waiting=*report.find(heldHouse);
  need(waiting.actualPlan.error=="native_city_material_schedule_unmapped"&&!waiting.actualPlan.candidate&&!waiting.actualPlan.candidateContext,"query returns same material reason without partial candidates");
  need(held?waiting.runtimeHold=="native_city_material_schedule_unmapped":waiting.runtimeHold.empty(),"query distinguishes not-yet-held and retained material isolation");
  const auto text=nativeHouseEvolutionText(waiting);need(text.summary.find("物资时序待接通")!=std::string::npos&&text.detail.find("物资消耗时序尚未接通，保持当前住宅和库存；其他已支持的供水与升降级继续运行。")!=std::string::npos,"UI names the specific material wait and continuing supported updates");
  need(text.detail.find("此住宅类型的后续升降级尚未接通")==std::string::npos&&text.detail.find("全部住宅")==std::string::npos,"material UI does not substitute a generic all-housing refusal");
  const auto& other=*report.find(otherHouse);need(other.actualPlan.error.empty()&&other.actualPlan.candidate&&other.actualPlan.candidateContext,"query keeps the other house's supported plan available");
  need(t.bytes(state)==before,"material query and presentation preserve every session byte");
 };
 queryMaterial(s,false);
 const auto refs=t.resources.archiveRefs();const auto contract=resolveNativeHouseRuntimeContract(s,e);need(bool(contract),"material C1 contract");auto inputs=contract->phaseInputs(s,&refs);
 unsigned helperCases=0;
 for(uint32_t part:{0u,7u})for(bool disabled:{false,true}){auto c=*view.city->buildingContext;c.monthPart=part;c.evolutionDisabled=disabled;
  const auto raw=planNativeHouseEvolution(high,c,high.inputs.houseWater);need(raw.candidate&&raw.candidateContext&&raw.direction==std::optional<int>(0)&&originalHouseGoodsFromPayload(*raw.candidate->inputs.house)->at(1)==2,"strict direction0 still debits material at part0/7");
  auto legacyInputs=inputs;legacyInputs.cityContextContract=false;const auto legacy=planNativePhaseHouseEvolution(high,c,legacyInputs);
  need(legacy.candidate&&legacy.candidateContext&&originalHouseGoodsFromPayload(*legacy.candidate->inputs.house)->at(1)==2,"no-C generic phase plan preserves prior material debit");
  const auto guarded=planNativePhaseHouseEvolution(high,c,inputs);need(!guarded.candidate&&!guarded.candidateContext&&guarded.error=="native_city_material_schedule_unmapped","C1 discards both material-changing candidates with named isolation");
  need(originalHouseGoodsFromPayload(*high.inputs.house)==baselineGoods,"pure material guards preserve complete original inventory");++helperCases;
 }
 s.paused=false;unsigned phase39Passes=0,employmentPasses=0,foodPasses=0;std::optional<uint64_t> previous39;
 for(unsigned n=0;n<=102;++n){const unsigned p=s.houseSchedule->phase;const auto before=context(s);
  std::string priorRecord,priorGoods;std::optional<NativeSessionObject> priorHouse;std::optional<native_phase_detail::Geometry> priorShape;
  if(p==39){priorRecord=s.strictRecords.at(heldHouse);const auto object=std::find_if(s.objects.begin(),s.objects.end(),[&](const auto& o){return o.id==heldHouse;});need(object!=s.objects.end(),"material held object exists");priorHouse=*object;
   const auto prior=t.strict(s);priorShape=native_phase_detail::geometry(prior.city->buildings.at(size_t(heldHouse)).state.building);need(bool(before.goods),"material phase has complete Goods state");auto normalized=*before.goods;normalized.savedPhase=0;priorGoods=goodsOnlyWire(normalized);
  }
  t.action(s,NativeSessionAction::Tick);const auto after=context(s);
  const auto current=decodeNativeStrictRecord(s.strictRecords.at(heldHouse),e);need(current&&originalHouseGoodsFromPayload(*current->house)==baselineGoods,"whole runtime keeps food50/material3 through every intervening phase");
  need(s.heldHouseReasons.count(heldHouse)&&s.heldHouseReasons.at(heldHouse)=="native_city_material_schedule_unmapped"&&!s.heldHouseReasons.count(otherHouse),"material isolation remains scoped to its one house");
  need(nativePopulation(s)==21&&s.houseSchedule->monthPart==0&&s.figureRuntime->immigrantPayloads.empty(),"whole city remains coherent with its explicit frozen calendar");
  if(p==39){if(previous39)need(s.ticks-*previous39==51,"material hold repeats only after a complete51-phase scheduler loop");previous39=s.ticks;++phase39Passes;
   const auto object=std::find_if(s.objects.begin(),s.objects.end(),[&](const auto& o){return o.id==heldHouse;});need(object!=s.objects.end()&&object->house&&priorHouse->house,"material held house remains present");
   need(s.strictRecords.at(heldHouse)==priorRecord&&object->house->type==priorHouse->house->type&&object->house->model==priorHouse->house->model&&object->residents==priorHouse->residents&&object->house->remaining==priorHouse->house->remaining,"blocked phase39 has no partial type/model/population/full61-byte-tail or record commit");
   const auto committed=t.strict(s);need(native_phase_detail::sameGeometry(*priorShape,native_phase_detail::geometry(committed.city->buildings.at(size_t(heldHouse)).state.building))&&committed.city->figures.services->population&&committed.city->figures.services->population->total==21,"blocked phase39 preserves house shape and canonical population21");
   need(bool(after.goods),"material phase retains complete Goods state");auto normalized=*after.goods;normalized.savedPhase=0;need(goodsOnlyWire(normalized)==priorGoods,"material rejection publishes no stock/policy/cache Goods prefix beyond next phase");
   queryMaterial(s,true);
   need(after.houseEvolutionCalls==before.houseEvolutionCalls+1,"unblocked other house still executes its normal phase39");
  }
  if(p==25){++employmentPasses;const auto plan=planNativeLabor(t.seed,s);need(after.employmentCalls==before.employmentCalls+1&&plan.error.empty()&&plan.find(well)&&plan.find(well)->workers==8&&after.workers.at(size_t(well))==8,"material hold does not stop actual phase25 partial staffing");}
  else need(after.workers==before.workers,"material isolation does not trigger an off-schedule staffing rewrite");
  if(p==17){++foodPasses;need(after.cityDerived&&after.cityDerived->food==50,"read-only food census retains independent food word50");}
 }
 need(helperCases==4&&phase39Passes==3&&employmentPasses==2&&foodPasses==2,"four helper boundaries and three whole-runtime material holds completed");
 const auto wire=t.bytes(s);auto restored=decodeNativeSession(t.seed,wire,e);need(restored&&t.bytes(*restored)==wire&&restored->heldHouseReasons.at(heldHouse)=="native_city_material_schedule_unmapped","material hold and inventory survive complete save/restore");
 t.canonicalize(*restored);need(t.bytes(*restored)==wire,"material hold rebuild preserves full committed state");
 std::cout<<"{\"material_helper_cases\":"<<helperCases<<",\"material_phase39_passes\":"<<phase39Passes<<",\"scheduler_ticks\":103,\"synthetic_only\":true}"<<std::endl;
}
void cityWaterOnlyTransitions(){
 Fixture t;auto& lower=t.seed.houseModels.at(0);lower.fill(0);lower[0]=-99;lower[1]=100;lower[17]=7;
 auto& upper=t.seed.houseModels.at(1);upper.fill(0);upper[0]=-99;upper[1]=100;upper[3]=1;upper[17]=14;
 auto& source=*t.resources.strictTemplate.emptyCity.buildingContext;source.houseModels=t.seed.houseModels;
 const size_t grid=size_t(*source.cropBase)+2+228u*3;source.terrainRandom->at(grid)=1;
 NativeSessionSource src{t.seed,OriginalSaveResult{},std::make_shared<const NativeHouseRuntimeResources>(t.resources)};
 NativeHouseModels models;models.sourceRevision=t.seed.sourceRevision;models.modelRevision=t.seed.modelRevision;models.difficulty=2;models.house=t.seed.houseModels;
 unsigned cases=0;
 for(bool wet:{false,true}){uint64_t well=0;auto s=occupied(t,well);auto& o=s.objects.front();const uint64_t house=o.id;
  o.house->houseWater=uint8_t(wet);o.house->remaining[45]=wet?96:0;o.house->remaining[47]=0;o.house->remaining[52]=wet?3:9;
  o.automation->cachedWater=uint8_t(wet);o.automation->delay=wet?3:9;
  std::string e;auto record=decodeNativeStrictRecord(s.strictRecords.at(house),e);need(bool(record),"C1 water record:"+e);record->houseWater=uint8_t(wet);record->house=o.house->remaining;s.strictRecords[house]=encodeNativeStrictRecord(*record);
  s.houseSchedule->monthPart=0;phase(s,39);t.canonicalize(s);const auto inventory=originalHouseGoodsFromPayload(*record->house);const auto beforeQuery=t.bytes(s);
  const auto report=queryNativeHouseEvolution(src,s,models);need(report.error.empty()&&report.find(house),"C1 water query:"+report.error);const auto& q=*report.find(house);
  need(q.actualPlan.error.empty()&&q.actualPlan.candidate&&q.actualPlan.candidateContext&&q.actualPlan.direction==std::optional<int>(wet?0:-1),"C1 retains its supported water-only candidate");
  const auto expected=*q.actualPlan.candidate;need(expected.building.type==(wet?4:3)&&expected.inputs.model==(wet?1:0)&&expected.inputs.residents==14&&expected.inputs.house->at(52)==0&&originalHouseGoodsFromPayload(*expected.inputs.house)==inventory,"water query predicts exact grade/delay reset without material debit");
  const auto text=nativeHouseEvolutionText(q);need(text.summary.find("物资时序待接通")==std::string::npos&&text.detail.find("物资消耗时序尚未接通")==std::string::npos,"supported water-only UI cannot report material waiting");
  need(t.bytes(s)==beforeQuery,"water-only query and presentation preserve all session bytes");
  s.paused=false;t.action(s,NativeSessionAction::Tick);const auto& actual=s.objects.front();const auto committed=decodeNativeStrictRecord(s.strictRecords.at(house),e);need(bool(committed),"committed C1 water record");
  need(actual.house->type==expected.building.type&&actual.house->model==uint16_t(expected.inputs.model)&&actual.residents==14&&actual.house->remaining==*expected.inputs.house&&s.strictRecords.at(house)==encodeNativeStrictRecord(expected.inputs),"actual water tick matches complete query candidate and strict record");
  need(actual.house->remaining[52]==0&&actual.automation->delay==0&&originalHouseGoodsFromPayload(actual.house->remaining)==inventory&&nativePopulation(s)==14,"water-only transition preserves food/material inventory and population");
  need(!s.heldHouseReasons.count(house),"material guard does not isolate supported C1 water changes");
  if(wet)need(actual.house->type==4&&actual.house->model==1&&s.observedDry->pendingExcess.empty(),"known wet delay3 clears without changing grade or excess accounting");
  else need(actual.house->type==3&&actual.house->model==0&&s.observedDry->pendingExcess.count(house)&&s.observedDry->pendingExcess.at(house)==7,"dry delay9 performs supported downgrade and retains pending excess7");
  const auto strict=t.strict(s);need(strict.city->buildings.at(size_t(house)).state.building.type==expected.building.type&&strict.city->figures.buildings->at(size_t(house)).houseModel==expected.inputs.model,"water transition reaches canonical grade/model mirrors");
  ++cases;
 }
 need(cases==2,"C1 dry and wet native transitions completed");std::cout<<"{\"c1_water_native_cases\":"<<cases<<",\"synthetic_only\":true,\"original_behavior_observed\":false}"<<std::endl;
}
}
int main(){try{legacy();Fixture fixture;goodsSourceFactory(fixture);noticeFactory(fixture);poisonRebuild(fixture);largeOuterWire(fixture);goodsSamePass(fixture);foodAndRecovery(fixture);staffing(fixture);rollback(fixture);materialScheduleIsolation();cityWaterOnlyTransitions();
 std::cout<<"{\"city_contract_checks\":"<<checks<<",\"restored_phases\":"<<restoredPhases<<",\"legacy_wire_goldens\":5,\"large_city_bytes\":"<<largeCityBytes<<",\"synthetic_only\":true,\"original_behavior_observed\":false,\"passed\":true}\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
