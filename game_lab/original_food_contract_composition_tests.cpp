// SPDX-License-Identifier: MIT
// Authored, conditional API composition; no assets, SAV fixture, recipe or 24/25 updater.
#include "original_building_scene.hpp"
#include "original_figure_update.hpp"
#include "native_walker_codec.hpp"
#include "native_city_context.hpp"
#include <iostream>
#include <stdexcept>
using namespace game_lab;
namespace {
size_t checks=0,groups=0;
void require(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
struct Writer : native_city_context_detail::Writer {
    template<class T> void value(T copy){native_city_context_detail::Writer::value(copy);}
};
std::string figureSnapshot(OriginalFigure f){native_walker_detail::Writer w;native_walker_detail::visitWellWalker(w,f);return w.out;}
std::string goodsSnapshot(const OriginalFigureGoodsServices& g){
    Writer w;w.value(g.unavailable);w.value(g.trade7);w.value(g.trade5);w.value(g.warehouse);w.value(g.savedPhase);
    w.value(g.storageSearch);w.value(g.industrySearch);w.value(g.godGate);w.value(g.industryPriority);
    for(const auto& p:g.policies){w.value(p.accept);w.value(p.trade);w.value(p.limit);}return w.out.str();
}
std::string buildingSnapshot(const OriginalFigureBuildingLink& b){
    Writer w;w.value(b.type);w.value(b.alive);w.value(b.workers);w.value(b.suppress);w.value(b.primaryFigure);w.value(b.secondaryFigures);
    w.value(b.incomingFigures);w.value(b.incomingAmount);w.value(b.houseModel);w.value(b.residents);w.value(b.accepting);w.value(b.serviceCount);
    w.value(b.marketGoods);w.value(b.marketQuality);w.value(b.marketDemand);w.value(b.houseGoods);w.value(b.foodQuality);w.value(b.storagePolicy);
    for(const auto* bays:{&b.storage,&b.marketStorage}){w.out<<bays->has_value()<<' ';if(*bays)for(const auto& c:**bays){w.value(c.resource);w.value(c.amount);w.value(c.capacity);}}
    return w.out.str();
}
std::string contextSnapshot(const OriginalFigureUpdateContext& c){
    std::ostringstream s;for(const auto& e:c.figures)s<<e.first<<':'<<figureSnapshot(e.second);
    if(c.buildings)for(const auto& e:*c.buildings)s<<e.first<<':'<<buildingSnapshot(e.second);
    if(c.goods)s<<goodsSnapshot(*c.goods);
    Writer w;w.value(c.cellHeads);w.value(c.freeSlots);w.value(c.highestLiveSlot);w.value(c.crtRandom);w.value(c.crtRandomCalls);
    w.value(c.allocationCount);w.value(c.buildingAllocationCount);w.value(c.speechCounters);
    if(c.services){w.value(c.services->routeOwners);w.value(c.services->directions);w.value(c.services->serviceGrid);w.value(c.services->randomFirst);w.value(c.services->randomSecond);w.value(c.services->allocationSerial);}
    return s.str()+w.out.str();
}
OriginalFigureBuildingLink storage(unsigned type=53){
    OriginalFigureBuildingLink b;b.type=uint16_t(type);b.alive=1;b.workers=1;b.storagePolicy=0;
    b.storage=std::vector<OriginalStorageCompartment>(8,{0,0,400});return b;
}
OriginalFigureGoodsServices goods(){OriginalFigureGoodsServices g;g.policies.resize(1);g.policies[0].limit.fill(3200);g.savedPhase=17;return g;}
unsigned stock(const OriginalFigureBuildingLink& b,unsigned r){unsigned n=0;for(const auto& bay:*(b.storage?b.storage:b.marketStorage))if(bay.resource==r)n+=bay.amount;return n;}
OriginalBuildingUpdateState producer(){
    OriginalBuildingUpdateState s;s.building.slot=7;s.building.type=33;s.building.size=1;s.building.imageInputs.emplace();
    s.inputs.identity=7;s.inputs.alive=1;s.inputs.output=4;s.production.emplace();s.production->operational=1;s.production->workers=8;s.production->enabled=1;return s;
}
OriginalBuildingUpdateContext productionContext(){
    OriginalBuildingUpdateContext c(OriginalBuildingUpdateScope::ProductionPasses,true);c.phase=20;c.cycle=4;c.monthPart=1;c.month=7;
    c.unavailable=0;c.requiredWorkers=8;c.festivalProductionBoost=false;return c;
}
void productionAndDispatch(){
    ++groups;const auto before=producer();auto c=productionContext();const auto p=originalBuildingStateAfterUpdates(before,c,1);
    require(p.state&&p.context&&detail::i16(p.state->production->industry.data())==10,"known producer workforce progresses by10");
    require(detail::i16(before.production->industry.data())==0&&before.production->secondaryStock==0,"production API retains original input; progress is not manufactured food stock");
    for(unsigned mode=0;mode<3;++mode){auto q=before;auto gated=c;if(mode==0)q.production->workers=0;if(mode==1)gated.unavailable=1;if(mode==2){q.production->primaryResource=1;q.production->primaryStock=99;}
        const auto r=originalBuildingStateAfterUpdates(q,gated,1);require(r.state&&r.state->production->industry==q.production->industry,"no labor/unavailable/missing input closes production without progress");}
    auto missing=c;missing.requiredWorkers.reset();const auto no=originalBuildingStateAfterUpdates(before,missing,1);
    require(!no.state&&detail::i16(before.production->industry.data())==0,"missing model input has no committed production prefix");
    // Explicit stock premise, NOT a progress-to-food conversion or completed hunt.
    auto lodge=*p.state;lodge.production->secondaryStock=100;lodge.inputs.workers=8;lodge.inputs.spawnCounter=1;lodge.inputs.operational=1;
    lodge.inputs.hookIndustry.emplace();lodge.inputs.hookIndustry->at(0x5c-4)=3;
    OriginalBuildingUpdateContext dispatch(OriginalBuildingUpdateScope::AdmittedTicks,true);
    dispatch.employmentModel=OriginalBuildingEmployment{{33,8}};dispatch.poolRoadSelected=std::map<size_t,bool>{{7,true}};
    dispatch.poolHookRoadAccess=std::map<size_t,OriginalBuildingRoadAccess>{{7,{}}};dispatch.poolHookRoadAccess->at(7).x=3;dispatch.poolHookRoadAccess->at(7).y=4;
    dispatch.figures.emplace(2000);for(size_t n=0;n<2000;++n){OriginalFigure f;f.slot=n;dispatch.figures->at(n)=f;}
    dispatch.figureServices.emplace();dispatch.figureServices->allocationSerial=17;dispatch.cropBase=0;
    dispatch.figureCellHeads=std::vector<uint16_t>(51984);dispatch.figureStationary.emplace();dispatch.scenarioFigurePlacementCheck=false;
    dispatch.figureSpeechCounts=std::array<uint32_t,359>{};dispatch.figureSpeechCounts->at(254)=3;dispatch.figureSpeechCounters=std::array<int32_t,359>{};
    dispatch.industryCarts=true;dispatch.freeFigureSlots={19};
    for(bool road:{false,true}){auto state=lodge;auto context=dispatch;context.poolRoadSelected->at(7)=road;
        require(originalBuildingHunterDispatchStep(state,context)==OriginalBuildingUpdateBlocker::None,"known dispatch premise accepted");
        if(!road){require(state.production->secondaryStock==100&&state.inputs.primaryFigure==0&&context.allocatedFigures.empty(),"disconnected producer retains stock and allocates no cart");}
        else {const auto& cart=context.figures->at(19);require(state.production->secondaryStock==0&&state.inputs.primaryFigure==19&&cart.type==19&&cart.owner==7&&cart.amount==100&&cart.subtype==4,"normal stocked producer debit and cart allocation agree");
            require(context.dispatchedProduction[4]==100&&context.figureCellHeads->at(915)==19,"dispatch ledger and physical actor cell agree");}}
    auto exhausted=lodge;dispatch.freeFigureSlots.clear();require(originalBuildingHunterDispatchStep(exhausted,dispatch)==OriginalBuildingUpdateBlocker::None&&exhausted.production->secondaryStock==100&&exhausted.inputs.primaryFigure==0,"pool exhaustion does not manufacture a cart or consume source stock");
}
struct Arrival {
    OriginalFigure actor;OriginalFigureUpdateContext context;SG3Archive archive;
    Arrival(){
        archive.version=214;archive.numImages=512;archive.sprites.resize(512);archive.imageIndex.fill(1);archive.bitmaps.resize(1);archive.bitmaps[0].filename="Authored Figure.bmp";
        archive.sprites[0].animationSpriteCount=12;archive.sprites[0].animationDirections=8;
        actor.slot=2;actor.type=19;actor.alive=1;actor.action=11;actor.counter=5;actor.subtype=4;actor.amount=100;actor.delivery=12;actor.owner=7;
        actor.homeX=3;actor.homeY=4;actor.cell=10000;actor.direction=8;actor.previousDirection=0;
        context.figures.emplace(2,actor);context.goods=goods();context.buildings.emplace();auto mill=storage();mill.incomingFigures=1;mill.incomingAmount=100;context.buildings->emplace(12,mill);
    }
    OriginalFigureFirstUpdate update(bool missingPrimary=false) const {auto primary=archive;if(missingPrimary)primary.imageIndex[0]=0;const std::map<uint32_t,const SG3Archive*> refs{{14,&archive},{38,&archive},{39,&primary}};return originalFigureFirstUpdate(actor,refs,&context,0x40);}
};
void carrierToStorage(){
    ++groups;Arrival a;const auto old=contextSnapshot(a.context);const auto full=a.update();
    require(full.state&&full.buildings&&full.goods,"known type19 arrival produces a complete candidate");
    require(contextSnapshot(a.context)==old,"successful single-actor updater never publishes to its const input");
    require(full.state->action==13&&full.state->amount==0&&stock(full.buildings->at(12),4)==100&&full.goods->warehouse[4]==100,"carrier cargo becomes mill physical stock and Goods once");
    require(full.buildings->at(12).incomingFigures==0&&full.buildings->at(12).incomingAmount==0,"original cargo reservation released once");
    Arrival waiting;waiting.actor.counter=4;waiting.context.figures.at(2)=waiting.actor;const auto delayed=waiting.update();
    require(delayed.state&&delayed.state->action==11&&stock(delayed.buildings->at(12),4)==0,"arrival waiting call does not deliver early");
    Arrival partial;partial.context.cropBase=0;partial.context.services.emplace();partial.context.services->directions.assign(500000,0);
    partial.context.goods->storageSearch=std::array<uint8_t,36>{};partial.context.goods->industrySearch=std::array<uint8_t,36>{};partial.context.goods->godGate=0;partial.context.goods->unavailable[4]=1;
    auto& mill=partial.context.buildings->at(12);mill.storage=std::vector<OriginalStorageCompartment>(8,{2,400,400});mill.storage->back()={4,399,400};partial.context.goods->warehouse[4]=399;partial.actor.movementMode=1;
    const auto limited=partial.update();require(limited.state&&limited.state->amount==99&&limited.state->action==6&&stock(limited.buildings->at(12),4)==400,"one-unit physical capacity retains99 cargo rather than deleting it");
    require(limited.goods->warehouse[4]==400&&limited.buildings->at(12).incomingAmount==0&&limited.buildings->at(12).incomingFigures==0,"partial fit accounts one actual unit while releasing original reservation");
    Arrival late;const auto lateOld=contextSnapshot(late.context);const auto bad=late.update(true);
    require(!bad.state&&!bad.buildings&&!bad.goods&&contextSnapshot(late.context)==lateOld,"missing image rejects without exposing computed delivery prefixes");
    Arrival missing;missing.actor.delivery=13;const auto missingOld=contextSnapshot(missing.context);const auto no=missing.update();require(!no.state&&!no.buildings&&contextSnapshot(missing.context)==missingOld,"missing destination produces no transaction");
}
void limitedStorageProtocols(){
    ++groups;for(unsigned kind:{53u,54u})for(unsigned room:{0u,1u,99u,100u}){
        auto b=storage(kind);auto g=goods();b.storage=std::vector<OriginalStorageCompartment>(8,{2,400,400});b.storage->back()={4,400-room,400};g.warehouse[4]=400-room;
        const auto accepted=originalFigureWarehouseDeliver(b,4,100,g);require(accepted&&*accepted==room,"bounded storage returns actual acceptance, including physical shortage");
        require(stock(b,4)==400&&g.warehouse[4]==400,"every accepted unit is physically and statistically credited once");
    }
    auto b=storage(54);auto g=goods();g.policies[0].limit[4]=7;require(originalFigureWarehouseDeliver(b,4,100,g)==7&&stock(b,4)==7&&g.warehouse[4]==7,"policy-limited reception agrees with Goods");
    const auto remainder=originalFigureStorageWithdraw(b,4,10,true,g);require(remainder==3&&stock(b,4)==0&&g.warehouse[4]==0,"withdraw returns unfulfilled remainder, not acceptance");
    require(originalFigureStorageWithdraw(b,4,1,true,g)==1,"empty storage withdrawal leaves whole request unsatisfied");
    for(unsigned mode=0;mode<4;++mode){auto bad=storage();auto pool=goods();if(mode==0)bad.storage->back().capacity=800;if(mode==1)bad.storagePolicy=1;
        const auto before=buildingSnapshot(bad)+goodsSnapshot(pool);const auto r=originalFigureWarehouseDeliver(bad,mode==2?29:4,mode==3?32768:100,pool);
        require(!r&&buildingSnapshot(bad)+goodsSnapshot(pool)==before,"invalid capacity/policy/resource/amount refuses without a prefix");}
    auto denied=storage();auto pool=goods();pool.policies[0].accept[4]=1;const auto before=buildingSnapshot(denied)+goodsSnapshot(pool);
    require(originalFigureWarehouseDeliver(denied,4,100,pool)==0&&buildingSnapshot(denied)+goodsSnapshot(pool)==before,"denied resource acceptance is a pure zero acceptance");
}
struct Retail {
    OriginalFigure actor;OriginalFigureUpdateContext context;
    Retail(){
        context.buildings.emplace();OriginalFigureBuildingLink m;m.type=59;m.alive=1;m.secondaryFigures[0]=7;m.marketQuality=60;m.marketGoods=100;m.serviceCount=299;
        m.marketStorage=std::vector<OriginalStorageCompartment>(6,{0,0,800});m.marketStorage->at(0)={28,100,800};m.marketDemand=std::array<int16_t,7>{};
        OriginalFigureBuildingLink h;h.type=3;h.alive=1;h.residents=20;h.accepting=1;h.size=1;h.houseGoods=std::array<int16_t,12>{};h.houseGoods->at(7)=123;h.foodQuality=20;
        context.buildings->emplace(1,m);context.buildings->emplace(2,h);context.buildings->emplace(3,h);
        for(unsigned col=9;col<=13;++col)context.houseMarketModels[col][1]=10;
        context.services.emplace();context.services->serviceGrid.assign(51984,0x80000000u);context.services->routeOwners[4]=7;context.services->directions={2,4,6};context.services->allocationSerial=99;
        context.terrain.assign(51984,0);context.feature.assign(51984,0);context.buildingOwners.assign(51984,0);context.cropBase=0;context.activeSide=10;context.goods=goods();
        actor.type=23;actor.slot=7;actor.alive=1;actor.owner=1;actor.x=4;actor.y=4;actor.cell=916;actor.progress=19;actor.direction=2;actor.previousDirection=2;actor.roamState=1;actor.roamLimit=1000;
        context.buildingOwners[916]=2;context.buildingOwners[917]=3;context.cellHeads.assign(51984,0);context.cellHeads[916]=7;
        for(size_t n=0;n<10;++n){OriginalFigure f;f.slot=n;context.figures.emplace(n,f);}context.figures.at(7)=actor;context.highestLiveSlot=7;context.freeSlots={5,4};
    }
    bool distribute(){const bool ok=originalFigureEntertainmentSegment(actor,context);if(ok)context.figures.at(7)=actor;return ok;}
    void arrived(){// Explicit retirement boundary; missing walking/return behavior is not invented here.
        actor.alive=2;actor.action=2;actor.route=4;context.figures.at(7)=actor;
    }
    std::string snapshot() const {return figureSnapshot(actor)+contextSnapshot(context);}
};
void retailAndRetirement(){
    ++groups;Retail r;const auto source=goodsSnapshot(*r.context.goods);require(r.distribute(),"known retail segment distributes to two houses");
    auto& b=*r.context.buildings;const unsigned food=b.at(2).houseGoods->at(0)+b.at(3).houseGoods->at(0);
    require(food==40&&stock(b.at(1),28)==60&&food+stock(b.at(1),28)==100,"multi-house service conserves all100 units across recipients and market");
    require(*b.at(2).foodQuality==60&&b.at(2).houseGoods->at(7)==123&&goodsSnapshot(*r.context.goods)==source,"quality follows known service while unrelated words and Goods stay unchanged");
    require(b.at(1).serviceCount==300&&r.context.services->serviceGrid[916]==(0x80000000u|(7u<<21)),"service cap and market lane preserve other service bits");
    r.arrived();require(originalFigureRemoveArrivedWorker(r.actor,r.context),"serviced retailer retires through existing auxiliary0 transaction");
    require(b.at(1).secondaryFigures[0]==0&&!r.actor.alive&&!r.context.figures.at(7).alive&&r.context.services->routeOwners[4]==0&&r.context.cellHeads[916]==0&&r.context.freeSlots.back()==7,"owner, pool, route, cell and FIFO retire together");
    require(stock(b.at(1),28)==60&&*b.at(1).marketQuality==60&&food==40,"retirement cannot recredit spent inventory");
    const auto once=r.snapshot();require(!originalFigureRemoveArrivedWorker(r.actor,r.context)&&r.snapshot()==once,"repeated retirement is pure and never duplicates FIFO");
    for(unsigned position=0;position<3;++position){Retail p;p.context.buildings->at(1).secondaryFigures={0,0};if(position==0)p.context.buildings->at(1).primaryFigure=7;else p.context.buildings->at(1).secondaryFigures[position-1]=7;
        p.arrived();require(originalFigureRemoveArrivedWorker(p.actor,p.context)&&p.context.buildings->at(1).primaryFigure==0&&p.context.buildings->at(1).secondaryFigures==std::array<int16_t,2>{0,0},"all three supported owner links clear at their known boundary");}
    Retail late;require(late.distribute(),"failure setup retains actual known distribution result");late.arrived();late.actor.route=1000;const auto prior=late.snapshot();
    require(!originalFigureRemoveArrivedWorker(late.actor,late.context)&&late.snapshot()==prior,"late retirement route failure preserves already committed distribution and all graph state");
    std::string error;OriginalFigure live;live.type=23;live.alive=1;require(!encodeNativeWellWalker(live,error)&&error=="not_well_walker_type28","composition does not silently admit23 to the Well codec");
}
void distributionRefusals(){
    ++groups;
    constexpr unsigned resources[]{28,19,13,25,23,22,24},indices[]{0,1,6,2,4,5,3};
    for(unsigned lane=0;lane<7;++lane){Retail one;auto& market=one.context.buildings->at(1);auto& house=one.context.buildings->at(2);
        market.marketStorage->at(0)={resources[lane],100,800};const auto cityGoods=goodsSnapshot(*one.context.goods);int visits=0;
        require(originalFigureMarketHouse(market,house,one.context,visits),"all seven known resource lanes can supply a house");
        require(house.houseGoods->at(indices[lane])==10&&stock(market,resources[lane])==90&&visits==1,"each resource debit equals its own exact house-word credit");
        require(house.houseGoods->at(7)==123&&goodsSnapshot(*one.context.goods)==cityGoods,"leaf transfer preserves unrelated house word and separate warehouse Goods");
    }
    Retail mixed;mixed.context.buildings->at(1).marketStorage->at(1)={19,10000,10001};const auto mixedOld=mixed.snapshot();
    require(!mixed.distribute()&&mixed.snapshot()==mixedOld,"later material debit mismatch rolls back an already computed food/quality prefix");
    for(unsigned mode=0;mode<4;++mode){Retail r;if(mode==0)r.context.buildings->at(3).houseModel=99;
        if(mode==1)r.context.buildings->at(1).marketStorage->at(0).amount=10000;
        if(mode==2)r.context.houseMarketModels.erase(12);if(mode==3)r.actor.cell=51984;
        const auto before=r.snapshot();require(!r.distribute()&&r.snapshot()==before,"late house/model/sentinel/grid failure has no multi-house stock, quality, demand or service prefix");}
    Retail empty;empty.context.buildings->at(1).marketStorage->at(0).amount=0;empty.context.buildings->at(1).marketGoods=0;
    require(empty.distribute()&&empty.context.buildings->at(2).houseGoods->at(0)==0&&empty.context.buildings->at(3).houseGoods->at(0)==0,"missing food cannot manufacture house credit");
    Retail full;for(size_t n:{2u,3u})full.context.buildings->at(n).houseGoods->at(0)=40;
    require(full.distribute()&&stock(full.context.buildings->at(1),28)==100,"saturated houses leave market food intact");
}
OriginalBuildingUpdateContext tailFixture(){
    auto c=productionContext();c.scope=OriginalBuildingUpdateScope::AdmittedTicks;c.phase=50;c.monthPart=0;c.cycle=4;
    c.cycleTail.emplace();c.cycleTail->alternateMode=false;c.cycleTail->smoothedHealth=59;c.cycleTail->healthTarget=80;c.cycleTail->productionHistory={3,901,2,603};c.cycleTail->currentHealthRating=147;
    c.labor.emplace();c.labor->difficulty=2;c.labor->health=59;c.populationTotal=400;c.buildingTotals.emplace();c.buildingTotals->staffedProducers[4]=2;c.buildingTotals->staffedProducers[2]=1;
    c.goodsRecords.emplace();c.goods=goods();c.trade.emplace();c.statistics.emplace();c.statistics->currentFaction=0;c.scenarioMonths=0;c.elapsedYears=-2050;c.scenarioStartYears=-2050;c.monthlyServices=true;
    c.terrain=std::vector<uint32_t>(51984);c.terrainOwners=std::vector<uint16_t>(51984);c.activeSide=1;c.cropBase=229;
    OriginalBuildingFootprint b;b.slot=7;b.type=72;b.size=2;b.wellImageInputs.emplace();b.wellImageInputs->byte6f=2;
    OriginalBuildingUpdateInputs i;i.identity=7;i.alive=1;i.word92=123;i.modelUpgrade=2;
    c.buildings=std::map<size_t,OriginalBuildingFootprint>{{7,b}};c.buildingInputs=std::map<size_t,OriginalBuildingUpdateInputs>{{7,i}};c.notificationEffects={1234};c.buildingRedraws={6};return c;
}
std::string tailSnapshot(OriginalBuildingUpdateContext c){
    auto city=captureNativeCityContext(c);Writer w;native_city_context_detail::fields(w,city);w.value(c.phase);w.value(c.cycle);w.value(c.monthPart);w.value(c.month);w.value(c.labor->health);w.value(c.labor->difficulty);
    for(const auto& e:*c.buildingInputs){w.value(e.first);w.value(e.second.alive);w.value(e.second.word92);w.value(e.second.modelUpgrade);w.value(e.second.industry);}
    for(const auto& e:*c.buildings){w.value(e.first);w.value(e.second.type);if(e.second.wellImageInputs)w.value(e.second.wellImageInputs->byte6f);}
    w.value(c.notificationEffects);for(auto n:c.buildingRedraws)w.value(n);return w.out.str();
}
std::string messageSnapshot(OriginalBuildingScenarioMessages m){Writer w;w.value(m.records);w.value(m.count);w.value(m.notificationPending);for(const auto& n:m.notices){w.value(n.id);w.value(n.slot);w.value(n.faction);w.value(n.resource);w.value(n.amount);w.value(n.title);w.value(n.dialogFlags);}return w.out.str();}
void retailThenTail(){
    ++groups;Retail retail;require(retail.distribute(),"tail composition starts with actual known market transaction");retail.arrived();require(originalFigureRemoveArrivedWorker(retail.actor,retail.context),"tail composition starts after actual known retirement");
    const auto retailCommitted=retail.snapshot();auto c=tailFixture();c.goods=retail.context.goods;
    // These contexts have distinct owners. No unimplemented food conversion or city adapter is asserted.
    for(unsigned part:{5u,6u,7u,11u,15u}){auto missing=c;missing.monthPart=part;const auto before=tailSnapshot(missing);
        for(unsigned retry=0;retry<2;++retry)require(originalBuildingCycleTailStep(missing)==OriginalBuildingUpdateBlocker::MonthlyCity&&tailSnapshot(missing)==before&&retail.snapshot()==retailCommitted,"missing monthly/calendar service rolls back tail and cannot undo or replay committed market work");}
    auto noForecast=c;noForecast.buildingTotals.reset();const auto prior=tailSnapshot(noForecast);
    require(originalBuildingCycleTailStep(noForecast)==OriginalBuildingUpdateBlocker::MonthlyCity&&tailSnapshot(noForecast)==prior,"missing food forecast rejects health/date/expiry prefix");
    auto exception=c;exception.buildings->erase(7);const auto before=tailSnapshot(exception);bool threw=false;
    try{(void)originalBuildingCycleTailStep(exception);}catch(const std::out_of_range&){threw=true;}
    require(threw&&tailSnapshot(exception)==before&&retail.snapshot()==retailCommitted,"mid-tail exception preserves previous committed world projections");
    auto replay=c;require(originalBuildingCycleTailStep(c)==OriginalBuildingUpdateBlocker::None&&c.cycle==5&&c.monthPart==1&&c.labor->health==60,"already supported ordinary tail commits once");
    require(originalBuildingCycleTailStep(replay)==OriginalBuildingUpdateBlocker::None&&tailSnapshot(replay)==tailSnapshot(c),"copied in-memory tail premise has identical next result, not a certified world codec");
    require(retail.snapshot()==retailCommitted,"successfully running a tail never replays stock distribution or retirement");
    auto annual=tailFixture();annual.month=1;annual.monthPart=1;auto& own=annual.trade->partners[0];own.alive=1;own.faction=0;own.blocked=1;
    auto& ally=annual.trade->partners[1];ally.alive=1;ally.faction=1;ally.kind=2;ally.enabled=1;ally.relations[0].state=2;ally.relations[0].war=1;own.relations[1].state=2;
    std::array<OriginalBuildingAnnualOffer,22> offers{};offers[1]={100,29};OriginalBuildingScenarioMessages messages;messages.records[0][113]=0xa5;
    const auto annualOld=tailSnapshot(annual),messagesOld=messageSnapshot(messages);
    for(unsigned retry=0;retry<2;++retry)require(originalBuildingCycleTailWithAnnualStep(annual,offers,&messages,uint8_t(2))==OriginalBuildingUpdateBlocker::MonthlyCity&&tailSnapshot(annual)==annualOld&&messageSnapshot(messages)==messagesOld,"computed annual message followed by missing service publishes no context or outbox prefix");
}
}
int main(){try{productionAndDispatch();carrierToStorage();limitedStorageProtocols();retailAndRetirement();distributionRefusals();retailThenTail();
    std::cout<<"{\"passed\":true,\"groups\":"<<groups<<",\"checks\":"<<checks<<",\"authored_conditional_api_composition_only\":true,\"original_gameplay_tested\":false,\"market_24_25_conversion_or_consumption_enabled\":false}\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
