// SPDX-License-Identifier: MIT
// Authored kernel/module graph compositions, not App/original-game qualification.
#include "original_world_checkpoint_test_fixture.hpp"
#include <filesystem>
using namespace game_lab;using namespace game_lab::world_test;
namespace {
size_t disks=0,groups=0;std::filesystem::path output,data;
SG3Archive sprites(){SG3Archive a;a.version=214;a.numImages=512;a.sprites.resize(512);a.imageIndex.fill(1);a.bitmaps.push_back({"Authored Figure.bmp"});for(auto& s:a.sprites){s.width=30;s.height=40;s.type=256;}a.sprites[0].animationSpriteCount=12;a.sprites[0].animationDirections=8;return a;}
void mirrors(OriginalWorldSnapshot& w){
 if(w.city.figures.buildings){
  for(const auto& [slot,b]:*w.city.figures.buildings)if(slot&&b.type){auto found=w.city.buildings.find(slot);if(found==w.city.buildings.end()){
   OriginalCombinedBuilding x{{},OriginalBuildingUpdateContext(OriginalBuildingUpdateScope::AdmittedTicks,true)};
   x.state.building.slot=slot;x.state.building.type=b.type;x.state.building.x=uint16_t(b.x);x.state.building.y=uint16_t(b.y);x.state.building.size=b.size;x.state.inputs.identity=uint32_t(slot);w.city.buildings.emplace(slot,std::move(x));}}
  need(originalCombinedSyncBuildings(w.city).empty(),"existing production shared-field bridge");
 }
 w.terrain.map.flags=w.city.figures.terrain;w.scene.shown.surface.terrain=w.terrain.map.flags;
 w.continuation.chained=true;w.continuation.initialFigures.reset();
 if(w.city.figures.crtRandom)w.continuation.session.crtRandom=*w.city.figures.crtRandom;
}
OriginalKernelIdentities ledger(const OriginalWorldSnapshot& w){std::string e;auto ids=OriginalKernelIdentities::restore(w.ledger,w.city.figures.figures,e);need(bool(ids),e.c_str());return *ids;}
void initial(OriginalWorldSnapshot& w){std::string e;const auto ids=OriginalKernelIdentities::loaded(w.city.figures.figures,e);need(bool(ids),e.c_str());w.ledger=ids->persistent();mirrors(w);}
void figures(OriginalWorldSnapshot& w,const OriginalFigureUpdateContext& context,const std::vector<OriginalKernelAllocation>& events={}){
 auto ids=ledger(w);std::string error;const auto next=ids.advanced(context.figures,events,error);need(bool(next),error.c_str());w.city.figures=context;w.ledger=next->persistent();mirrors(w);++w.commitRevision;
}
void equal(const OriginalWorldSnapshot& a,const OriginalWorldSnapshot& b,const char* why){std::string error;need(originalWorldPersistentEqual(a,b,error),why);}
OriginalWorldSnapshot disk(const OriginalWorldSnapshot& w,const std::string& name){
 std::string error;const auto file=output/(std::to_string(disks++)+"-"+name+".emworld");const auto published=writeOriginalWorldCheckpointNew(file.string(),data.string(),w);
 need(published.state!=OriginalWorldPublication::NotPublished,published.error.c_str());const auto bytes=readOriginalWorldCheckpointFile(file.string(),data.string(),error);need(bool(bytes),error.c_str());const auto restored=decodeOriginalWorldCheckpoint(*bytes,error);need(bool(restored),error.c_str());equal(w,*restored,"every persistent field equals actual disk value");return *restored;
}
OriginalBuildingUpdateContext allocator(const OriginalWorldSnapshot& w,uint32_t serial){
 OriginalBuildingUpdateContext c(OriginalBuildingUpdateScope::AdmittedTicks,true);c.figures.emplace(2000);for(const auto& [slot,f]:w.city.figures.figures)c.figures->at(slot)=f;
 c.figureServices.emplace();c.figureServices->allocationSerial=serial;c.figureServices->randomFirst=1;c.figureServices->randomSecond=16;
 c.figureCellHeads=w.city.figures.cellHeads;c.figureStationary.emplace();c.cropBase=0;c.scenarioFigurePlacementCheck=false;
 c.figureSpeechCounters=std::array<int32_t,359>{};c.figureSpeechCounts=std::array<uint32_t,359>{};c.figureSpeechCounts->at(271)=3;c.freeFigureSlots={9,8,10};return c;
}
void publishAllocation(OriginalWorldSnapshot& w,OriginalBuildingUpdateContext c,size_t previousEvents){
 auto f=w.city.figures;for(const auto& actor:*c.figures)f.figures.at(actor.slot)=actor;f.services=c.figureServices;f.cellHeads=*c.figureCellHeads;f.freeSlots=c.freeFigureSlots;f.highestLiveSlot=c.highestFigureSlot;
 const std::vector<OriginalKernelAllocation> events(c.allocationEvents.begin()+previousEvents,c.allocationEvents.end());w.city.buildingContext=c;figures(w,f,events);
}
void constructorWrap(){++groups;for(uint32_t serial:{uint32_t(65535),UINT32_MAX}){
 auto w=fixture();w.city.buildingContext=allocator(w,serial);initial(w);
 auto construct=[](OriginalWorldSnapshot& value){auto c=*value.city.buildingContext;const auto old=c.allocationEvents.size();const auto slot=originalBuildingAllocateService(c,28,0,3,4,0);need(slot&&*slot,"actual service constructor succeeds");publishAllocation(value,std::move(c),old);return *slot;};
 need(construct(w)==9&&w.city.figures.figures.at(9).allocationSerial==65535,"first constructor writes16bit65535");const auto old=ledger(w).handle(9);auto restored=disk(w,"W08-construct-wrap");
 const auto fresh=ledger(restored);need(old&&!fresh.matches(*old),"disk restore invalidates old host handle without changingwire serial");
 need(construct(w)==8&&construct(restored)==8,"next actual constructor pops sameFIFO slot");
 need(w.city.figures.figures.at(8).allocationSerial==0&&*w.city.figures.services->allocationSerial==(serial==UINT32_MAX?1u:65537u),"next constructor haslegal zero serial andfull32bit counter");equal(w,restored,"constructor continuation matchesall world fields andgeneration ledger");
 }}
void ordered(OriginalWorldSnapshot& w,const std::map<uint32_t,const SG3Archive*>& refs){
 const auto pass=originalFigureOrderedNextPass(w.city.figures,refs);need(pass.complete&&pass.context,"actual orderedpass completes");figures(w,*pass.context,pass.context->allocationEvents);
}
void drawSort(){++groups;auto w=fixture();auto a=sprites();const std::map<uint32_t,const SG3Archive*> refs{{38,&a}};
 for(size_t n:{1u,2u}){auto& f=w.city.figures.figures.at(n);f.type=76;f.runtimeClass=OriginalFigureClass::Pheasant;f.alive=1;f.action=31;f.herdTarget=uint16_t(n);f.cell=1000;f.progress=n==1?20:0;f.direction=0;f.next=n==1?2:0;}
 w.city.figures.highestLiveSlot=2;w.city.figures.freeSlots={9,8};w.city.figures.cellHeads[1000]=1;initial(w);
 need(originalFigureSortDrawCell(w.city.figures,1000)&&w.city.figures.cellHeads[1000]==2&&w.city.figures.figures.at(2).next==1&&w.city.figures.figures.at(1).next==0,"actual drawing sort reorders two actors");
 auto restored=disk(w,"W10-postsort");ordered(w,refs);ordered(restored,refs);equal(w,restored,"postsort disk continuation retains heads/next/imageinputs andall worldfields");
}
void hunterHistory(){++groups;auto w=fixture();auto group=sprites(),projectiles=group;
 projectiles.imageIndex[0x6d-1]=101;projectiles.imageIndex[0x6c-1]=201;for(unsigned n:{100u,200u}){projectiles.sprites[n].animationSpriteCount=2;projectiles.sprites[n].animationDirections=32;projectiles.sprites[n].animationSpeedId=2;}
 const std::map<uint32_t,const SG3Archive*> refs{{14,&projectiles},{38,&group},{39,&group}};
 auto& f=w.city.figures;f.services.emplace();f.services->directions.assign(500000,0);f.services->allocationSerial=17;f.services->randomFirst=1;f.services->randomSecond=16;f.crtRandom=1;f.animationFlags=std::array<bool,51>{};f.animationFlags->at(2)=true;f.cropBase=0;f.terrain.assign(51984,0x80);f.buildingOwners.assign(51984,0);
 OriginalFigure one;one.type=11;one.alive=1;one.image=38u<<14;one.progress=20;
 auto hunter=one;hunter.slot=2;hunter.type=40;hunter.action=7;hunter.target=1;hunter.owner=9;hunter.x=10;hunter.y=10;hunter.cell=2290;hunter.subX=hunter.subY=200;
 auto prey=one;prey.slot=1;prey.type=76;prey.action=32;prey.x=5;prey.y=15;prey.cell=3425;prey.updateOwner=1;
 f.figures.at(1)=prey;f.figures.at(2)=hunter;f.cellHeads[2290]=2;f.cellHeads[3425]=1;f.freeSlots={1999,1998};f.highestLiveSlot=2;f.buildings.emplace();OriginalFigureBuildingLink station;station.type=0x3a;station.alive=1;f.buildings->emplace(9,station);f.models[40][6]=5;f.models[40][7]=30;initial(w);
 ordered(w,refs);need(w.city.figures.primaryImageInputs.at(1).action==32&&w.city.figures.figures.at(1).action==18,"actual later Hunter writes earlier prey after itsimage write");
 auto restored=disk(w,"W16-late-hunter");need(restored.city.figures.primaryImageInputs.at(1).action==32&&restored.city.figures.figures.at(1).action==18,"disk retains distinct write-timeimage andcurrent prey state");
 ordered(w,refs);ordered(restored,refs);equal(w,restored,"Hunter/projectile/prey next orderedpass allfields equal afterdisk");
}
OriginalFigureGoodsServices goods(){OriginalFigureGoodsServices g;g.policies.resize(1);g.policies[0].limit.fill(3200);g.savedPhase=17;return g;}
OriginalFigureBuildingLink storage(){OriginalFigureBuildingLink b;b.type=53;b.alive=1;b.workers=1;b.storagePolicy=0;b.storage=std::vector<OriginalStorageCompartment>(8,{0,0,400});return b;}
unsigned stock(const OriginalFigureBuildingLink& b,unsigned resource){unsigned total=0;for(const auto& c:*b.storage)if(c.resource==resource)total+=c.amount;return total;}
void oneUpdate(OriginalWorldSnapshot& w,size_t slot,const std::map<uint32_t,const SG3Archive*>& refs,uint32_t flags){
 const auto update=originalFigureFirstUpdate(w.city.figures.figures.at(slot),refs,&w.city.figures,flags);need(bool(update.state),"actual single-actor updater completes");auto f=w.city.figures;f.figures.at(slot)=*update.state;
 for(const auto& [target,actor]:update.linkedWrites)f.figures.at(target)=actor;
 if(update.services)f.services=update.services;if(update.buildings)f.buildings=update.buildings;if(update.goods)f.goods=update.goods;if(update.cellHeads)f.cellHeads=*update.cellHeads;if(update.freeSlots)f.freeSlots=*update.freeSlots;
 if(update.crtRandom)f.crtRandom=update.crtRandom;f.crtRandomCalls=update.crtRandomCalls;
 f.primaryImageInputs[slot]=originalFigureImageInputs(*update.state);figures(w,f,update.allocationEvents);
}
void carrierPartial(){++groups;auto w=fixture();auto a=sprites();const std::map<uint32_t,const SG3Archive*> refs{{14,&a},{38,&a},{39,&a}};auto& f=w.city.figures;
 auto& actor=f.figures.at(2);actor.type=19;actor.alive=1;actor.action=11;actor.counter=4;actor.subtype=4;actor.amount=100;actor.delivery=12;actor.owner=7;actor.homeX=3;actor.homeY=4;actor.cell=10000;actor.direction=8;actor.previousDirection=0;actor.movementMode=1;
 f.highestLiveSlot=2;f.freeSlots={9,8};f.cellHeads[10000]=2;f.cropBase=0;f.services.emplace();f.services->directions.assign(500000,0);f.services->allocationSerial=42;f.goods=goods();f.goods->storageSearch=std::array<uint8_t,36>{};f.goods->industrySearch=std::array<uint8_t,36>{};f.goods->godGate=0;f.goods->unavailable[4]=1;
 f.buildings.emplace();OriginalFigureBuildingLink owner;owner.type=33;owner.alive=1;owner.outputResource=4;owner.primaryFigure=2;f.buildings->emplace(7,owner);f.terrain[10000]=0x40;auto mill=storage();mill.incomingFigures=1;mill.incomingAmount=100;mill.storage=std::vector<OriginalStorageCompartment>(8,{2,400,400});mill.storage->back()={4,399,400};f.buildings->emplace(12,mill);f.goods->warehouse[4]=399;initial(w);
 oneUpdate(w,2,refs,0x40);need(w.city.figures.figures.at(2).counter==5&&stock(w.city.figures.buildings->at(12),4)==399,"actual arrival waits beforedelivery");auto restored=disk(w,"W19-before-partial");
 oneUpdate(w,2,refs,0x40);oneUpdate(restored,2,refs,0x40);equal(w,restored,"actual partial arrival next result fullworld equal");
 need(w.city.figures.figures.at(2).amount==99&&w.city.figures.figures.at(2).action==6&&stock(w.city.figures.buildings->at(12),4)==400&&w.city.figures.goods->warehouse[4]==400,"one actual unit accepted,99cargo retained withmatching Goods");
 need(w.city.figures.buildings->at(12).incomingAmount==0&&w.city.figures.buildings->at(12).incomingFigures==0,"reservation released once");
 auto carried=disk(w,"W19-residual-cargo");oneUpdate(w,2,refs,0x40);oneUpdate(carried,2,refs,0x40);equal(w,carried,"residual99 follow-up preserves route/FIFO/stock/Goods");
}
void serviceRetire(){++groups;auto w=fixture();auto a=sprites();const std::map<uint32_t,const SG3Archive*> refs{{38,&a}};auto& c=w.city.figures;
 OriginalFigure f;f.slot=7;f.type=23;f.alive=1;f.owner=1;f.action=2;f.route=4;f.cell=916;f.x=f.destinationX=4;f.y=f.destinationY=4;f.direction=8;f.progress=20;c.figures.at(7)=f;c.highestLiveSlot=7;c.freeSlots={5,4};c.cellHeads[916]=7;c.services.emplace();c.services->routeOwners[4]=7;c.services->directions={2,4,6};c.services->allocationSerial=99;c.goods=goods();c.models[23][15]=1;c.modelSpeeds[23]=0;
 c.buildings.emplace();OriginalFigureBuildingLink market;market.type=59;market.alive=1;market.secondaryFigures[0]=7;market.marketQuality=60;market.marketGoods=60;market.marketStorage=std::vector<OriginalStorageCompartment>(6,{0,0,800});market.marketStorage->at(0)={28,60,800};c.buildings->emplace(1,market);initial(w);
 // Action2 is the existing return handler at its home endpoint, not an invented
 // route-completion loop. Execute it before saving the retirement boundary.
 oneUpdate(w,7,refs,0);need(w.city.figures.figures.at(7).alive==2,"actual type23 home arrival marks retirement");auto restored=disk(w,"W18-returned-retailer");
 auto retire=[](OriginalWorldSnapshot& value){auto context=value.city.figures;auto actor=context.figures.at(7);need(originalFigureRemoveArrivedWorker(actor,context),"actual retirement succeeds");figures(value,context);};
 retire(w);retire(restored);equal(w,restored,"retirement afterdisk matchesall route/cell/owner/FIFO/Goods fields");
 need(!w.city.figures.figures.at(7).alive&&w.city.figures.buildings->at(1).secondaryFigures[0]==0&&w.city.figures.services->routeOwners[4]==0&&w.city.figures.cellHeads[916]==0&&w.city.figures.freeSlots==std::vector<size_t>({5,4,7}),"retirement jointly clears link/route/cell andappendsFIFO once");
 auto retired=disk(w,"W18-retired");auto repeat=[](OriginalWorldSnapshot& value){auto c=value.city.figures;auto f=c.figures.at(7);need(!originalFigureRemoveArrivedWorker(f,c),"repeatcleanup refuses");original_world_codec_detail::Writer before,after;before.value(value.city.figures);after.value(c);need(before.bytes==after.bytes,"repeatcleanup retains all context fields");};repeat(w);repeat(retired);equal(w,retired,"repeat cleanup does notduplicateFIFO or stock");
}
void workerArrival(){++groups;auto w=fixture();auto a=sprites();const std::map<uint32_t,const SG3Archive*> refs{{38,&a}};auto& c=w.city.figures;
 auto& worker=c.figures.at(2);worker.type=11;worker.alive=1;worker.owner=0;worker.home=12;worker.action=8;worker.x=worker.y=1;worker.cell=229;worker.subX=worker.subY=20;worker.employmentAmount=2;worker.route=4;
 c.cellHeads[229]=2;c.highestLiveSlot=2;c.freeSlots={5,4};c.cropBase=0;c.modelSpeeds[11]=0;c.houseCapacities[0]=8;c.services.emplace();c.services->allocationSerial=17;c.services->routeOwners[4]=2;c.services->population.emplace();c.services->population->total=10;c.services->population->peak=11;
 c.buildings.emplace();OriginalFigureBuildingLink house;house.type=3;house.alive=1;house.worker=2;house.houseModel=0;house.residents=6;house.freeSlots=2;house.laborDisabled=false;house.accepting=1;c.buildings->emplace(12,house);
 OriginalFigureBuildingLink owner;owner.worker=2;c.buildings->emplace(0,owner);initial(w);
 auto before=disk(w,"W18-before-worker-arrival");oneUpdate(w,2,refs,0);oneUpdate(before,2,refs,0);equal(w,before,"actual workerarrival afterdisk preserveswholeworld");
 need(w.city.figures.figures.at(2).alive==2&&w.city.figures.buildings->at(12).residents==8&&w.city.figures.buildings->at(12).worker==0&&w.city.figures.services->population->total==12&&w.city.figures.services->population->lastArrival==2,"workerarrival updatesunique household/population authority once");
 auto arrived=disk(w,"W18-arrived-worker");auto retire=[](OriginalWorldSnapshot& v){auto c=v.city.figures;auto actor=c.figures.at(2);need(originalFigureRemoveArrivedWorker(actor,c),"actualarrived worker cleanup");figures(v,c);};retire(w);retire(arrived);equal(w,arrived,"worker retirement afterdisk preservesroute/FIFO/population");
 need(w.city.figures.services->routeOwners[4]==0&&w.city.figures.cellHeads[229]==0&&w.city.figures.freeSlots==std::vector<size_t>({5,4,2})&&w.city.figures.buildings->at(0).worker==0,"worker cleanup clearsroute/cell/owner andappends2after5,4");
 auto retired=disk(w,"W18-next-worker-constructor");auto next=[](OriginalWorldSnapshot& v){auto c=allocator(v,*v.city.figures.services->allocationSerial);c.figureServices=v.city.figures.services;c.terrain=v.city.figures.terrain;c.freeFigureSlots=v.city.figures.freeSlots;c.highestFigureSlot=v.city.figures.highestLiveSlot;c.figureSpeechCounts->at(265)=3;const auto slot=originalBuildingAllocateMigrant(c,11,1,1);need(slot&&*slot==5,"next actualmigrant constructor respects existingFIFO");publishAllocation(v,std::move(c),0);};next(w);next(retired);equal(w,retired,"retired world disk nextconstructor preservespopulation andsame allocation");need(w.city.figures.services->population->total==12,"next constructor neverduplicates priorarrival population");
}
OriginalBuildingUpdateContext tail(){
 OriginalBuildingUpdateContext c(OriginalBuildingUpdateScope::AdmittedTicks,true);c.phase=50;c.monthPart=1;c.cycle=4;c.month=1;
 c.cycleTail.emplace();c.cycleTail->alternateMode=false;c.cycleTail->smoothedHealth=59;c.cycleTail->healthTarget=80;c.cycleTail->productionHistory={3,901,2,603};c.cycleTail->currentHealthRating=147;
 c.labor.emplace();c.labor->difficulty=2;c.labor->health=59;c.populationTotal=400;c.buildingTotals.emplace();c.buildingTotals->staffedProducers[4]=2;c.buildingTotals->staffedProducers[2]=1;
 c.goodsRecords.emplace();c.goods.emplace();c.trade.emplace();c.statistics.emplace();c.statistics->currentFaction=0;c.scenarioMonths=0;c.elapsedYears=-2050;c.scenarioStartYears=-2050;c.monthlyServices=true;
 c.terrain=std::vector<uint32_t>(51984);c.terrainOwners=std::vector<uint16_t>(51984);c.activeSide=1;c.cropBase=229;c.buildings.emplace();c.buildingInputs.emplace();
 auto& own=c.trade->partners[0];own.alive=1;own.faction=0;own.blocked=1;own.relations[1].state=2;
 auto& ally=c.trade->partners[1];ally.alive=1;ally.faction=1;ally.kind=2;ally.enabled=1;ally.relations[0].state=2;ally.relations[0].war=1;return c;
}
OriginalBuildingUpdateBlocker annual(OriginalWorldSnapshot& w){
 auto box=AppCityNotifications::restore(w.notifications,*w.city.scenarioMessages);need(bool(box),"restore dispositionvalue before annual candidate");
 const auto r=originalBuildingCycleTailWithAnnualStep(*w.city.buildingContext,w.city.annualOffers,&*w.city.scenarioMessages,uint8_t(2));
 if(r==OriginalBuildingUpdateBlocker::None){need(box->observeCommitted(*w.city.scenarioMessages),"observe onlyfully committed annual message");w.notifications=box->persistent();++w.commitRevision;}
 return r;
}
void annualRetry(){++groups;auto w=fixture();w.city.buildingContext=tail();w.city.annualOffers.emplace();w.city.annualOffers->at(1)={100,29};w.city.scenarioMessages->records[0][113]=0xa5;initial(w);
 const auto before=w;need(annual(w)==OriginalBuildingUpdateBlocker::MonthlyCity,"actual annual candidate failslate afternotice computation");equal(before,w,"lateannual failure preserveswholeworld includingoffers/notifications/RNG/clock");auto restored=disk(w,"W26-late-refusal");
 need(annual(w)==OriginalBuildingUpdateBlocker::MonthlyCity&&annual(restored)==OriginalBuildingUpdateBlocker::MonthlyCity,"diskrestored failed annual retries withsame refusal");equal(before,w,"second latefailure retainsall originalfields");equal(w,restored,"failed retry afterdisk allfields equal");
 // The missing service is supplied explicitly to both callers as a new known
 // test input; disk restoration itself never supplies this prerequisite.
 w.city.buildingContext->trade->partners[1].relationTimers.resize(22);restored.city.buildingContext->trade->partners[1].relationTimers.resize(22);
 need(annual(w)==OriginalBuildingUpdateBlocker::None&&annual(restored)==OriginalBuildingUpdateBlocker::None,"same explicitservice correction commitsannual retry");equal(w,restored,"successful retry wholeworld/context/externalmessages/dispositions equal");
 need(w.city.scenarioMessages->count==1&&w.city.scenarioMessages->records[0][113]==0xa5&&w.notifications.live.size()==1&&w.notifications.live.front().state==AppCityTributeState::Pending,"single pending annualreceipt andretainedpadding");
 auto committed=disk(w,"W26-committed-annual");need(annual(w)==OriginalBuildingUpdateBlocker::None&&annual(committed)==OriginalBuildingUpdateBlocker::None,"next supported ordinarytail afterannual");equal(w,committed,"ordinarytail afterdisk never repeatsannual message");need(w.city.scenarioMessages->count==1&&w.notifications.live.size()==1,"once-only messagecount andinbox");
}
void destruction(){++groups;auto w=fixture();auto& f=w.city.figures;f.cropBase=0;f.terrain.assign(51984,0);f.buildingOwners.assign(51984,0);f.buildingOwners[1144]=7;f.services.emplace();f.services->allocationSerial=37;f.services->randomFirst=f.services->randomSecond=0;f.freeSlots={19,18};f.buildings.emplace();
 OriginalFigureBuildingLink link;link.type=33;link.alive=1;link.cell=1144;link.x=4;link.y=5;link.size=1;link.fireRisk=999;f.buildings->emplace(7,link);
 OriginalCombinedBuilding b{{},OriginalBuildingUpdateContext(OriginalBuildingUpdateScope::AdmittedTicks,true)};b.state.building.slot=7;b.state.building.type=33;b.state.building.size=1;b.state.building.x=4;b.state.building.y=5;b.state.building.imageInputs.emplace();b.state.building.imageInputs->gridIndex=1144;b.state.inputs.identity=7;b.state.inputs.alive=1;b.state.inputs.fireRisk=999;w.city.buildings.emplace(7,b);
 OriginalBuildingUpdateContext c(OriginalBuildingUpdateScope::AdmittedTicks,true);c.evolutionDisabled=false;c.engineRegion=0;c.buildings=std::map<size_t,OriginalBuildingFootprint>{{7,b.state.building}};c.buildingInputs=std::map<size_t,OriginalBuildingUpdateInputs>{{7,b.state.inputs}};c.productionInputs.emplace();c.buildingModels.emplace();c.buildingModels->operator[](33)[7]=1;
 c.terrain=f.terrain;c.terrainOwners=f.buildingOwners;c.terrainRandom=std::vector<uint8_t>(51984,1);c.cropBase=0;c.activeSide=228;c.disasters.emplace();c.disasters->freeBuildingSlots=std::vector<size_t>{8};c.disasters->buildingSerial=37;c.disasters->ruinImageBase=(3u<<14)|100;c.disasters->effectPlacementCulled=true;
 c.resourceGrowth.emplace();auto& surface=c.resourceGrowth->surface;surface.terrain=f.terrain;surface.image.resize(51984);surface.subTile.resize(51984,0x9f);surface.draw.resize(51984);surface.feature.resize(51984,4);
 c.irrigation.emplace();c.irrigation->image.resize(51984);c.irrigation->owners.resize(51984);w.city.buildingContext=c;w.city.phase=44;w.terrain.map.activeSide=228;w.scene.shown.surface=surface;w.scene.shown.liveRecords=1;w.scene.shown.placed.push_back({b.state.building,{ },4,5,{}});initial(w);
 auto phase=[](OriginalWorldSnapshot& value){auto ids=ledger(value);const auto error=originalCombinedBuildingPhase(value.city);need(error.empty(),error.c_str());std::string e;const auto advanced=ids.advanced(value.city.figures.figures,value.city.buildingContext->allocationEvents,e);need(bool(advanced),e.c_str());value.ledger=advanced->persistent();mirrors(value);++value.commitRevision;};
 phase(w);need(w.city.buildingContext->disasters->destructions==1&&w.city.buildingContext->disasters->createdBuildings.size()==1&&w.city.buildingContext->disasters->destroyedBuildings==std::vector<size_t>{7},"actual phase44 replaces original withindependent ruin");need(w.city.buildings.at(7).state.inputs.alive==4&&w.city.buildings.at(8).state.building.type==161&&w.city.figures.buildingOwners[1144]==8,"tombstone/ruin/terrain ownership committed byactualcombined bridge");
 // Run the actual scene installation bridge over the actual admitted result.
 // The descriptor copies its public phase outputs, not a fabricated ruin.
 auto general=sprites();SG3Archive terrainArchive;terrainArchive.version=213;terrainArchive.numImages=512;terrainArchive.sprites.resize(512);for(auto& sprite:terrainArchive.sprites){sprite.type=30;sprite.width=78;sprite.height=40;sprite.uncompressedLength=3200;}
 OriginalSceneTimeline timeline;timeline.complete=true;timeline.context=w.city.figures;timeline.combinedState=w.city;timeline.createdBuildings=1;timeline.destroyedBuildings=1;
 OriginalFigureProfile pool;pool.status=OriginalFigureStatus::Complete;pool.cellHeads=w.city.figures.cellHeads;for(const auto& [slot,actor]:w.city.figures.figures)pool.slots.push_back(actor);
 OriginalBuildingProfile buildings;buildings.status=OriginalBuildingStatus::Complete;const std::map<uint32_t,const SG3Archive*> refs;
 need(installOriginalSceneTimeline(w.scene.shown,w.terrain.map,pool,refs,buildings,general,timeline,&terrainArchive),"actualdestroyed/ruin sceneinstallation completes");
 need(w.scene.shown.placed.size()==1&&w.scene.shown.placed.front().footprint.type==161&&std::string(w.scene.shown.placed.front().binding.archive)=="DATA/China_Terrain.sg3","actual ruinselector usesTerrain symbol");
 auto restored=disk(w,"W20-actual-ruin-scene");w.city.phase=restored.city.phase=46;phase(w);phase(restored);equal(w,restored,"next emptybuildingphase afterdisk doesnot replaydestruction orallocate another ruin");
 need(w.city.buildingContext->disasters->destructions==1&&w.city.buildingContext->disasters->buildingSerial==38&&w.city.figures.buildingOwners[1144]==8,"ruin serial/ownership andonce-only destruction preserved");
}
}
int main(int argc,char** argv){try{need(argc==2,"explicit authored output required");output=std::filesystem::absolute(argv[1]);std::filesystem::create_directories(output);data=output/"authored-data";std::filesystem::create_directory(data);
 constructorWrap();drawSort();hunterHistory();carrierPartial();serviceRetire();workerArrival();annualRetry();destruction();
 std::cout<<"{\"passed\":true,\"groups\":"<<groups<<",\"disk_cases\":"<<disks<<",\"checks\":"<<checks<<",\"original_data\":false,\"App_or_game_launched\":false,\"cases\":[\"W08\",\"W10\",\"W16\",\"W18\",\"W19\",\"W20\",\"W26\"]}\n";return 0;
 }catch(const std::exception& e){std::cerr<<"group="<<groups<<" disk="<<disks<<" "<<e.what()<<'\n';return 1;}}
