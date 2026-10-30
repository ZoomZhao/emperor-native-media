// SPDX-License-Identifier: MIT
#include "original_world_checkpoint_test_fixture.hpp"
using namespace game_lab;using namespace game_lab::world_test;
namespace {
void representedCases(){
 std::string error;auto s=fixture();
 // Independent scene/mirror clocks and lagged account values are data, not
 // recomputed caches. No business callback runs in this codec exercise.
 s.city.phase=0;s.city.buildingContext.emplace(OriginalBuildingUpdateScope::AdmittedTicks,true);
 auto& c=*s.city.buildingContext;c.phase=50;c.monthPart=15;c.month=11;c.treasury=123;c.populationTotal=91;c.populationPeak=100;
 s.city.annualOffers.emplace();for(size_t i=0;i<22;++i){s.city.annualOffers->at(i).requestedAmount=i%2?INT32_MIN:INT32_MAX;s.city.annualOffers->at(i).requestedResource=int16_t(i);}
 s.city.figures.services.emplace();s.city.figures.services->allocationSerial=UINT32_MAX;s.city.figures.services->directions.resize(500000,7);
 s.city.figures.services->routeOwners[3]=9;s.city.figures.goods.emplace();s.city.figures.goods->savedPhase=16;
 s.city.figures.freeSlots.erase(s.city.figures.freeSlots.begin());s.city.figures.freeSlots.push_back(1);
 s.continuation.session.randomHistory.cursor=99;s.continuation.admission=UINT32_MAX-3;s.continuation.session.admissionClock=s.continuation.admission;
 s.continuation.initialFigures=s.city.figures;
 auto round=[&](const OriginalWorldSnapshot& value){const auto bytes=encodeOriginalWorldCheckpoint(value,error);need(bool(bytes),error.c_str());const auto restored=decodeOriginalWorldCheckpoint(*bytes,error);need(bool(restored),error.c_str());need(originalWorldPersistentEqual(value,*restored,error),"complete nondefault lagged world exact");return *restored;};
 auto restored=round(s);need(restored.city.phase==0&&restored.city.buildingContext->phase==50&&restored.city.buildingContext->monthPart==15,"legal tail roles never normalized");need(restored.city.figures.freeSlots.back()==1&&restored.city.figures.services->directions.size()==500000,"FIFO and full route table retained");
 auto bad=s;bad.continuation.session.randomHistory.cursor=100;need(!encodeOriginalWorldCheckpoint(bad,error),"out of range RNG cursor refused");
 bad=s;bad.continuation.initialFigures->figures.at(3).amount=4;need(!decodeOriginalWorldCheckpoint(original_world_codec_detail::unchecked(bad),error),"contradictory active pool aliases refused with recomputed hashes");
 // Full runtime/historical messages and accepted/rejected/unsupported/Pending
 // dispositions survive. Restoring IDs must not pay accepted offers twice.
 auto m=fixture();m.city.buildingContext.emplace(OriginalBuildingUpdateScope::AdmittedTicks,true);auto& ctx=*m.city.buildingContext;ctx.month=0;ctx.elapsedYears=3;ctx.scenarioStartYears=1;ctx.treasury=900;
 auto& messages=*m.city.scenarioMessages;messages.count=1;messages.records[0][2]=29;messages.records[149][267]=213;
 auto box=AppCityNotifications::fromSaved(messages);need(bool(box),"saved historical queue");
 for(unsigned i=0;i<4;++i)need(originalBuildingAnnualMessage(ctx,messages,{i==3?0:100,29},uint8_t(i),2)==OriginalBuildingUpdateBlocker::None,"annual message producer");
 need(box->observeCommitted(messages),"all runtime offers observed");
 auto persistent=box->persistent();persistent.live[0].state=AppCityTributeState::Accepted;persistent.live[1].state=AppCityTributeState::Rejected;
 box=AppCityNotifications::restore(persistent,messages);need(bool(box),"valid distinct dispositions");m.notifications=box->persistent();const auto id=box->nextPending();need(bool(id),"pending positive offer");
 restored=round(m);auto inbox=AppCityNotifications::restore(restored.notifications,*restored.city.scenarioMessages);need(bool(inbox)&&!inbox->find(*id),"load IDs fresh");
 const auto pending=*inbox->nextPending();need(inbox->respond(pending,AppCityTributeDecision::Reject,*restored.city.buildingContext)==AppCityTributeResult::Applied,"pending decision once");need(inbox->respond(pending,AppCityTributeDecision::Reject,*restored.city.buildingContext)==AppCityTributeResult::AlreadyCompleted,"decision never repeats");
 bad=m;bad.notifications.live.erase(bad.notifications.live.begin());need(!encodeOriginalWorldCheckpoint(bad,error),"missing accepted disposition cannot be replayed as new");
 // Each currently handled family retains the same represented input and named
 // next-pass refusal/success after restore. Legacy tests exercise rich actions.
 for(uint8_t type:{1,2,11,13,19,20,21,22,23,26,28,29,30,34,35,36,39,40,45,47,54,76}){
  auto v=fixture();auto& f=v.city.figures.figures.at(9);f.alive=1;f.type=type;f.allocationSerial=65535;if(type==76)f.runtimeClass=OriginalFigureClass::Pheasant;
  v.city.figures.highestLiveSlot=9;v.city.figures.freeSlots.erase(std::find(v.city.figures.freeSlots.begin(),v.city.figures.freeSlots.end(),9));
  const auto ids=OriginalKernelIdentities::loaded(v.city.figures.figures,error);need(bool(ids),"family identity");v.ledger=ids->persistent();v.continuation.initialFigures=v.city.figures;
  const auto copy=round(v);const auto a=originalFigureOrderedNextPass(v.city.figures,{}),b=originalFigureOrderedNextPass(copy.city.figures,{});
  need(a.complete==b.complete&&a.firstBlockedSlot==b.firstBlockedSlot&&a.computed==b.computed,"same family input has same next-pass outcome");
  if(a.context&&b.context){original_world_codec_detail::Writer x,y;x.value(*a.context);y.value(*b.context);need(x.bytes==y.bytes,"successful next-pass every field equal");}
  auto ledger=OriginalKernelIdentities::restore(copy.ledger,copy.city.figures.figures,error);need(bool(ledger)&&!ledger->matches(*ids->handle(9)),"restore preserves wire identity but rejects old host handle");
 }
}
void malformedTypes(){using namespace original_world_codec_detail;
 auto rejects=[](const std::string& bytes,auto value){try{Reader r(bytes);r.value(value);return false;}catch(const Failure&){return true;}};
 need(rejects(std::string(1,char(2)),bool{}),"bool2 rejected");need(rejects(std::string(1,char(2)),std::optional<int>{}),"optional2 rejected");
 need(rejects(std::string(4,char(255)),OriginalFigureClass{}),"unknown enum rejected");
 uint64_t bits=0x7ff0000000000000ULL;std::string infinity(reinterpret_cast<const char*>(&bits),8);need(rejects(infinity,double{}),"nonfinite rejected");
 need(rejects(std::string(2,char(255)),static_cast<const char*>(nullptr)),"unknown pointer symbol rejected");
 Writer w;w.raw(500001,4);need(rejects(w.bytes,std::vector<uint8_t>{}),"large count rejected before allocation");
 w.bytes.clear();w.raw(500000,4);need(rejects(w.bytes,std::vector<OriginalFigure>{}),"truncated large vector rejected");
 w.bytes.clear();w.raw(2,4);w.raw(1,4);w.raw(2,4);w.raw(1,4);w.raw(3,4);need(rejects(w.bytes,std::map<int,int>{}),"duplicate map key rejected");
 try{Reader r("");r.charge(originalWorldAllocationLimit,2);need(false,"budget must fail");}catch(const Failure&){need(true,"global allocation budget checked before reserve");}
 auto s=fixture();w.bytes.clear();w.value(s.camera);need(w.bytes==std::string(1,char(0)),"absent optional exact tag");s.camera=OriginalMapCamera{};Writer yes;yes.value(s.camera);need(yes.bytes!=w.bytes,"known zero distinct from absent");
}
}
int main(){try{
 representedCases();malformedTypes();
 size_t visited=0;
 visited+=typedRoundtrip<OriginalFigure>();
 visited+=typedRoundtrip<OriginalFigureSavedServices::FormationImpact>();
 visited+=typedRoundtrip<OriginalFigureRandomHistory>();
 visited+=typedRoundtrip<OriginalFigurePopulation>();
 visited+=typedRoundtrip<OriginalFigureSavedServices>();
 visited+=typedRoundtrip<OriginalFigureSpawnServices>();
 visited+=typedRoundtrip<OriginalFigurePassability>();
 visited+=typedRoundtrip<OriginalFigureHunterLink>();
 visited+=typedRoundtrip<OriginalStorageCompartment>();
 visited+=typedRoundtrip<OriginalFigureBuildingLink::CropDelivery>();
 visited+=typedRoundtrip<OriginalFigureBuildingLink>();
 visited+=typedRoundtrip<OriginalInspectionVisit>();
 visited+=typedRoundtrip<OriginalFigureCropServices>();
 visited+=typedRoundtrip<OriginalFigureStoragePolicy>();
 visited+=typedRoundtrip<OriginalFigureGoodsServices>();
 visited+=typedRoundtrip<OriginalKernelAllocation>();
 visited+=typedRoundtrip<OriginalFigureImageInputs>();
 visited+=typedRoundtrip<OriginalFigureProfile::EmpireTraderSlots>();
 visited+=typedRoundtrip<OriginalFigureUpdateContext>();
 visited+=typedRoundtrip<OriginalWellImageInputs>();
 visited+=typedRoundtrip<OriginalBuildingImageInputs>();
 visited+=typedRoundtrip<OriginalBuildingAnimationInputs>();
 visited+=typedRoundtrip<OriginalBuildingFootprint>();
 visited+=typedRoundtrip<OriginalBuildingUpdateInputs>();
 visited+=typedRoundtrip<OriginalBuildingProductionInputs>();
 visited+=typedRoundtrip<OriginalBuildingHouseCapacityInputs>();
 visited+=typedRoundtrip<OriginalBuildingUpdateState>();
 visited+=typedRoundtrip<OriginalBuildingRoadAccess>();
 visited+=typedRoundtrip<OriginalBuildingFormation>();
 visited+=typedRoundtrip<OriginalBuildingFormationResult>();
 visited+=typedRoundtrip<OriginalBuildingCityEmployment::Row>();
 visited+=typedRoundtrip<OriginalBuildingCityEmployment>();
 visited+=typedRoundtrip<OriginalBuildingUpdateContext::Accounting>();
 visited+=typedRoundtrip<OriginalBuildingUpdateContext::CityDerived>();
 visited+=typedRoundtrip<OriginalBuildingGoodsRecord>();
 visited+=typedRoundtrip<OriginalBuildingUpdateContext::Labor>();
 visited+=typedRoundtrip<OriginalBuildingUpdateContext::Irrigation::Source>();
 visited+=typedRoundtrip<OriginalBuildingUpdateContext::Irrigation>();
 visited+=typedRoundtrip<OriginalBuildingUpdateContext::Statistics>();
 visited+=typedRoundtrip<OriginalBuildingUpdateContext::Religion::Entry>();
 visited+=typedRoundtrip<OriginalBuildingUpdateContext::Religion::Worship>();
 visited+=typedRoundtrip<OriginalBuildingUpdateContext::Religion>();
 visited+=typedRoundtrip<OriginalTerrainState>();
 visited+=typedRoundtrip<OriginalBuildingUpdateContext::ResourceGrowth>();
 visited+=typedRoundtrip<OriginalBuildingUpdateContext::Trade::Partner::Relation>();
 visited+=typedRoundtrip<OriginalBuildingUpdateContext::Trade::Partner>();
 visited+=typedRoundtrip<OriginalBuildingUpdateContext::Trade>();
 visited+=typedRoundtrip<OriginalBuildingUpdateContext::BuildingTotals>();
 visited+=typedRoundtrip<OriginalBuildingUpdateContext::CycleTail>();
 visited+=typedRoundtrip<OriginalBuildingUpdateContext::DisasterServices::FigureEffect>();
 visited+=typedRoundtrip<OriginalBuildingUpdateContext::DisasterServices>();
 visited+=typedRoundtrip<OriginalBuildingUpdateContext>();
 visited+=typedRoundtrip<OriginalCombinedBuilding>();
 visited+=typedRoundtrip<OriginalCityClocks>();
 visited+=typedRoundtrip<OriginalCombinedProcessInputs>();
 visited+=typedRoundtrip<OriginalPostFigureRegistryObject>();
 visited+=typedRoundtrip<OriginalPostFigurePassResult>();
 visited+=typedRoundtrip<OriginalBuildingAnnualOffer>();
 visited+=typedRoundtrip<OriginalBuildingScenarioMessages::Notice>();
 visited+=typedRoundtrip<OriginalBuildingScenarioMessages>();
 visited+=typedRoundtrip<OriginalCombinedState>();
 visited+=typedRoundtrip<OriginalBuildingImageBinding>();
 visited+=typedRoundtrip<OriginalBuildingComponent>();
 visited+=typedRoundtrip<OriginalBuildingPlacement>();
 visited+=typedRoundtrip<OriginalBuildingMarker>();
 visited+=typedRoundtrip<OriginalSceneDepth>();
 visited+=typedRoundtrip<OriginalSceneOverlay>();
 visited+=typedRoundtrip<OriginalFigureImageSelection>();
 visited+=typedRoundtrip<GroundSelection>();
 visited+=typedRoundtrip<GroundMapCell>();
 visited+=typedRoundtrip<OriginalBuildingScene>();
 visited+=typedRoundtrip<OriginalMapTerrain>();
 visited+=typedRoundtrip<OriginalAnimationClock>();
 visited+=typedRoundtrip<AppCitySessionState>();
 visited+=typedRoundtrip<OriginalMapCamera>();
 visited+=typedRoundtrip<EvidenceFigure>();
 visited+=typedRoundtrip<OriginalMarketKnownBase>();
 visited+=typedRoundtrip<OriginalWorldFileBinding>();
 visited+=typedRoundtrip<OriginalWorldSourceBinding>();
 visited+=typedRoundtrip<OriginalWorldIdentityEntry>();
 visited+=typedRoundtrip<OriginalWorldIdentityLedger>();
 visited+=typedRoundtrip<OriginalWorldNotifications>();
 visited+=typedRoundtrip<AppCityTributeRecord>();
 visited+=typedRoundtrip<OriginalWorldPreferences>();
 visited+=typedRoundtrip<OriginalWorldPolicy>();
 visited+=typedRoundtrip<OriginalWorldControllerStatus>();
 visited+=typedRoundtrip<OriginalWorldContinuation>();
 visited+=typedRoundtrip<OriginalWorldScene>();
 visited+=typedRoundtrip<OriginalWorldTerrain>();
 visited+=typedRoundtrip<OriginalWorldSnapshot>();
 auto s=fixture();std::string error;const auto encoded=encodeOriginalWorldCheckpoint(s,error);need(bool(encoded),"whole authored world encodes");const auto decoded=decodeOriginalWorldCheckpoint(*encoded,error);need(bool(decoded),"whole authored world decodes");need(originalWorldPersistentEqual(s,*decoded,error),"all canonical persistent fields equal");
 // Every byte of a small framing document and every section boundary of the
 // complete pool; do not perform a quadratic every-prefix scan of a large city.
 for(size_t n=0;n<888;++n)need(!decodeOriginalWorldCheckpoint(encoded->substr(0,n),error),"every small-header truncation refuses");
 for(size_t n=888;n<encoded->size();n+=std::max(size_t(1),encoded->size()/31))need(!decodeOriginalWorldCheckpoint(encoded->substr(0,n),error),"large document truncation refuses");
 for(unsigned section=0;section<14;++section){const auto at=64+56*section;uint64_t start=0;for(size_t n=0;n<8;++n)start|=uint64_t(uint8_t(encoded->at(at+8+n)))<<(8*n);need(!decodeOriginalWorldCheckpoint(encoded->substr(0,size_t(start)),error),"each section boundary truncates");}
 auto bad=*encoded;bad[16]=2;rehash(bad);need(!decodeOriginalWorldCheckpoint(bad,error),"unknown version with valid digest refuses");
 bad=*encoded;bad[56]=char(255);rehash(bad);need(!decodeOriginalWorldCheckpoint(bad,error),"unknown features refuse");
 bad=*encoded;bad[64]=2;rehash(bad);need(!decodeOriginalWorldCheckpoint(bad,error),"duplicate/missing section refuses");
 bad=*encoded;bad[72]=0;rehash(bad);need(!decodeOriginalWorldCheckpoint(bad,error),"overlap refuses");
 bad=*encoded;bad[900]^=1;need(!decodeOriginalWorldCheckpoint(bad,error),"checksum corruption refuses");
 bad=*encoded;bad+="x";need(!decodeOriginalWorldCheckpoint(bad,error),"trailing data refuses");
 auto dead=s;dead.city.figures.figures.at(7).follower=9;dead.city.figures.figures.at(7).allocationSerial=0;dead.continuation.initialFigures=dead.city.figures;
 const auto deadBytes=encodeOriginalWorldCheckpoint(dead,error);need(bool(deadBytes),"inactive nonzero follower is not a live dangling pair");const auto restored=decodeOriginalWorldCheckpoint(*deadBytes,error);need(bool(restored)&&restored->city.figures.figures.at(7).follower==9,"dead residual follower survives disk bytes unchanged");
 auto unknown=s;unknown.city.process.eventSlots=std::array<uint32_t,8>{{1,0,0,0,0,0,0,0}};unknown.policy.experimental=true;
 const auto u=encodeOriginalWorldCheckpoint(unknown,error);need(bool(u)&&decodeOriginalWorldCheckpoint(*u,error)->city.process.eventSlots->at(0)==1,"nonempty slots remain data, not native pointers");
 unknown.policy.experimental=false;const auto forged=original_world_codec_detail::unchecked(unknown);need(!decodeOriginalWorldCheckpoint(forged,error)&&error=="world_snapshot_policy_promotion","recomputed digests do not promote unknown producer input");
 auto changed=s;changed.city.figures.figures.at(0).alive=1;need(!encodeOriginalWorldCheckpoint(changed,error),"live sentinel refused");
 changed=s;changed.city.figures.freeSlots.push_back(1);need(!decodeOriginalWorldCheckpoint(original_world_codec_detail::unchecked(changed),error),"semantic FIFO corruption refuses even with correct checksums");
 changed=s;changed.ledger.live.push_back({2000,0,0,1});need(!decodeOriginalWorldCheckpoint(original_world_codec_detail::unchecked(changed),error),"out of range ledger refuses");
 std::cout<<"{\"passed\":true,\"codec_checks\":"<<checks<<",\"typed_field_visits\":"<<visited<<",\"typed_records\":90,\"bytes\":"<<encoded->size()<<",\"original_data\":false}\n";
 return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
