#include "city_game_state.hpp"
#include "app/city_goal_feedback_adapter.hpp"
#include "warehouse_roads_reference.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace game_lab;
namespace ref=warehouse_review;
namespace {
constexpr size_t oldBytes=65580, warehouseBytes=131152;
constexpr size_t stockOffset=oldBytes, ledgerOffset=oldBytes+16384*4;
std::filesystem::path root,legacyRoot;
std::string context;
unsigned sequence=0;
int failures=0;
void require(bool yes,const std::string& message) { if(!yes)throw std::runtime_error(message+" ["+context+"]"); }
template<class F>void test(const char* name,const char* source,F run) {
    context.clear();try{run();std::cout<<"PASS\t"<<name<<'\t'<<source<<"\tverified\n";}
    catch(const std::exception& e){++failures;std::cout<<"FAIL\t"<<name<<'\t'<<source<<'\t'<<e.what()<<'\n';}
}
void write(const std::filesystem::path& path,const std::vector<uint8_t>& bytes) {
    std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());require(bool(file),"own fixture write");
}
std::vector<uint8_t> read(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary);require(bool(file),"own fixture read");return {(std::istreambuf_iterator<char>(file)),{}};
}
void word(std::vector<uint8_t>& b,size_t at,uint32_t n) {for(unsigned i=0;i<4;++i)b.at(at+i)=static_cast<uint8_t>(n>>(8*i));}
uint32_t word(const std::vector<uint8_t>& b,size_t at) {
    uint32_t n=0;for(unsigned i=0;i<4;++i)n|=uint32_t(b.at(at+i))<<(8*i);return n;
}
void build(CitySimulation& city,unsigned x,unsigned y,BuildingType type) {
    std::string error;require(city.build(x,y,type,error),"authored build: "+error);
}
void remove(CitySimulation& city,unsigned x,unsigned y) {
    std::string error;require(city.demolish(x,y,error),"authored demolition: "+error);
}
ref::Grid grid(const CitySimulation& city) {
    ref::Grid g;
    for(unsigned y=0;y<128;++y)for(unsigned x=0;x<128;++x) {
        const auto i=ref::tile(x,y);g.buildings[i]=static_cast<uint8_t>(city.buildingAt(x,y));
        if(g.buildings[i]==ref::warehouse)g.stocks[i]=city.warehouseStockAt(x,y);
    }
    return g;
}
std::array<int,8> values(const CityLogisticsMonth& m) {
    return {m.beginStored,m.produced,m.delivered,m.noRouteDiscard,m.fullDiscard,m.consumed,m.unmetDemand,m.endStored};
}
std::array<int,8> values(const ref::Expected& m) {
    return {m.beginStored,m.produced,m.delivered,m.noRouteDiscard,m.fullDiscard,m.consumed,m.unmetDemand,m.endStored};
}
void invariant(const CitySimulation& city) {
    const auto bytes=city.saveSnapshot();require(bytes.size()==warehouseBytes && word(bytes,4)==4,"v4 exact serialized length/version");
    require(city.ruleset()==CityRuleset::RoadWarehouses,"explicit v4 ruleset");
    const auto g=grid(city);const auto routes=ref::settle(g,0,0);const auto overview=city.logisticsOverview();
    int sum=0,count=0;
    for(unsigned i=0;i<ref::cells;++i) {
        if(g.buildings[i]==ref::warehouse){++count;require(g.stocks[i]>=0 && g.stocks[i]<=600,"each actual warehouse bounded0..600");sum+=g.stocks[i];}
        require(word(bytes,stockOffset+4*i)==uint32_t(g.stocks[i]),"serialized every warehouse/non-warehouse stock agrees with public valid-tile inventory");
    }
    require(sum==city.stats().foodSupply && count==city.stats().totalWarehouses && count*600==city.stats().grainStorageCapacity,
        "global food is exactly sum of real stocks; capacity and warehouse counts");
    require(overview.warehouses==count && overview.capacity==count*600 && overview.stored==sum &&
        overview.connectedFarms==int(routes.connectedFarms) && overview.disconnectedFarms==int(routes.disconnectedFarms),"independent overview counts and four-neighbor topology");
    const ref::Roads network(g);
    for(unsigned i=0;i<ref::cells;++i)if(g.buildings[i]==ref::farm) {
        bool reachable=false;
        for(unsigned j=0;j<ref::cells;++j)if(g.buildings[j]==ref::warehouse && network.connects(i,j)){reachable=true;break;}
        require(city.farmHasWarehouseRoute(i%128,i/128)==reachable,"route query matches independent cardinal road components");
    }
    const auto& m=city.lastLogisticsMonth();const auto v=values(m);
    for(auto n:v)require(n>=0,"nonnegative last-month fields; undocumented maximum not invented");
    if(m.valid)require(int64_t(m.beginStored)+m.produced==int64_t(m.endStored)+m.consumed+m.noRouteDiscard+m.fullDiscard &&
        int64_t(m.produced)==int64_t(m.delivered)+m.noRouteDiscard+m.fullDiscard && m.endStored==sum,"both conservation identities and actual end stock");
    else require(v==std::array<int,8>{},"invalid ledger has eight zero fields");
    CitySimulation loaded;require(loaded.loadSnapshot(bytes) && loaded.saveSnapshot()==bytes,"v4 canonical full byte roundtrip");
}
int expectedProduction(const CitySimulation& city,int month) {
    const auto& s=city.stats();if(s.totalFarms<=0 || s.neededWorkers<=0 || s.availableWorkers<=0)return 0;
    const int64_t filled=std::min(s.availableWorkers,s.neededWorkers);
    const int64_t regular=int64_t(s.totalFarms)*FarmProductionRules::regularYield*filled/s.neededWorkers;
    const bool autumn=month>=FarmProductionRules::firstAutumnMonth && month<=FarmProductionRules::lastAutumnMonth;
    const int64_t bonus=autumn?int64_t(s.totalFarms)*FarmProductionRules::autumnBonus*filled/s.neededWorkers:0;
    return int(regular+bonus);
}
ref::Expected month(CitySimulation& city,const std::string& stage) {
    context=stage;const auto bytes=city.saveSnapshot();const auto events=city.events();
    const auto name=std::to_string(sequence++)+"-"+stage;write(root/(name+".before.emp"),bytes);
    const int next=city.stats().month==12?1:city.stats().month+1;
    const int production=expectedProduction(city,next);
    require(city.farmYieldForMonth(next)==production,"existing precise pooled harvest helper, including all warehouse jobs");
    const auto expected=ref::settle(grid(city),production,city.stats().population);
    const auto projected=city.nextMonthLogistics();const auto projectedStats=city.nextMonthStats();
    const auto feedback=evaluateCityGoals(makeCityFeedbackSnapshot(city,CityHarvestRule::RoadWarehousesV1));
    require(projected.valid && values(projected)==values(expected),"independent ledger agrees with next-month logistics");
    require(feedback.valid && feedback.nextMonth.known && feedback.nextMonth.foodAfter==expected.endStored,"goal feedback uses delivered/consumed inventory, not total crop");
    require(city.saveSnapshot()==bytes && city.events()==events,"all projections/feedback preserve source bytes/events");
    city.advanceMonth();write(root/(name+".after.emp"),city.saveSnapshot());
    const auto& actual=city.lastLogisticsMonth();
    require(actual.valid && values(actual)==values(expected),"actual complete monthly ledger matches independent transport and consumption");
    require(grid(city).stocks==expected.stocks && city.stats().foodSupply==expected.endStored,"row-major per-warehouse outcomes exact");
    require(city.stats().population==projectedStats.population && city.stats().housingCapacity==projectedStats.housingCapacity &&
        city.stats().treasury==projectedStats.treasury && city.stats().foodSupply==projectedStats.foodSupply,"projection matches real housing/famine/tax month");
    invariant(city);return expected;
}
// Canonical self-authored v4 fixture: only stocks, matching global total, optional month
// and explicitly invalid/all-zero ledger. No private memory writes or original-game bytes.
void stocks(CitySimulation& city,const std::vector<std::pair<unsigned,int>>& requested,int monthValue=0) {
    auto bytes=city.saveSnapshot();require(bytes.size()==warehouseBytes,"stock fixture requires v4 layout");
    for(unsigned i=0;i<ref::cells;++i)word(bytes,stockOffset+4*i,0);
    int total=0;for(const auto& entry:requested){word(bytes,stockOffset+4*entry.first,uint32_t(entry.second));total+=entry.second;}
    word(bytes,20,uint32_t(total));for(unsigned i=0;i<9;++i)word(bytes,ledgerOffset+4*i,0);
    if(monthValue)word(bytes,28,uint32_t(monthValue));
    require(city.loadSnapshot(bytes),"canonical stated-layout stock fixture accepted");invariant(city);
}
CitySimulation village() {
    CitySimulation city(CityRuleset::RoadWarehouses);city.setSpeed(0);
    for(unsigned i=0;i<6;++i)build(city,40+3*i,50,BuildingType::House);
    month(city,"bootstrap-workers");return city;
}
void initializationAndMaps() {
    require(CityWarehouseRules::capacity==600 && CityWarehouseRules::initialX==5 && CityWarehouseRules::initialY==2,"declared initial rules constants");
    CitySimulation city(CityRuleset::RoadWarehouses);invariant(city);
    require(city.buildingAt(5,2)==BuildingType::Warehouse && city.warehouseStockAt(5,2)==400 && city.stats().treasury==5000 &&
        city.stats().neededWorkers==2,"one preplaced official warehouse,400 grain,5000 treasury,no construction fee,2 jobs");
    const int treasury=city.stats().treasury;month(city,"initial-empty-month");require(city.stats().treasury==treasury-1,"warehouse monthly upkeep1");
    build(city,5,3,BuildingType::Road);build(city,6,3,BuildingType::Road);build(city,6,2,BuildingType::Farm);
    const auto zero=month(city,"connected-farm-zero-workers");require(zero.produced==0 && zero.delivered==0,"no workers cannot invent farm crop");
    CityMap map;map.width=map.height=128;map.tiles.assign(ref::cells,200);map.tiles[ref::tile(7,8)]=0;
    require(city.loadFromMap(map),"single-land warehouse map");invariant(city);
    require(city.buildingAt(7,8)==BuildingType::Warehouse && city.warehouseStockAt(7,8)==400 && city.stats().treasury==5000,"water initial tile falls back to first row-major land");
    const auto before=city.saveSnapshot();const auto events=city.events();map.tiles.assign(ref::cells,200);
    require(!city.loadFromMap(map) && city.saveSnapshot()==before && city.events()==events,"all-water rejection preserves existing entire city/events");
}
void topologyAndReconnection() {
    auto city=village();build(city,6,2,BuildingType::Farm); // Directly touches the initial warehouse, no road.
    auto result=month(city,"direct-adjacency-no-road");require(result.produced>0 && result.delivered==0 && result.noRouteDiscard==result.produced,"direct farm-store adjacency never transports");
    remove(city,6,2);build(city,6,5,BuildingType::Farm);
    build(city,5,3,BuildingType::Road);build(city,6,4,BuildingType::Road);
    result=month(city,"diagonal-road-components");require(result.noRouteDiscard==result.produced,"diagonal road cells cannot join");
    build(city,6,3,BuildingType::Road);result=month(city,"cardinal-road-connect");require(result.delivered>0 && result.noRouteDiscard==0,"cardinal detour restores next-month transport");
    remove(city,6,3);result=month(city,"bridge-cut");require(result.delivered==0 && result.noRouteDiscard==result.produced,"cut bridge discards current crop immediately");
    build(city,6,3,BuildingType::Road);result=month(city,"bridge-rejoined");require(result.delivered<=result.produced && result.noRouteDiscard==0,"rejoin has no old crop backlog");
    build(city,125,127,BuildingType::Farm);build(city,126,127,BuildingType::Road);build(city,127,127,BuildingType::Warehouse);
    require(city.farmHasWarehouseRoute(125,127),"last tile cardinal route");month(city,"last-cell-second-component");
}
void fairFarmSharesAndMultipleStores() {
    auto city=village();build(city,5,3,BuildingType::Road);build(city,6,3,BuildingType::Road);build(city,6,2,BuildingType::Farm);
    build(city,9,2,BuildingType::Farm); // Later row-major farm is intentionally in another component.
    const int next=city.stats().month==12?1:city.stats().month+1;
    require(expectedProduction(city,next)%2!=0,"fixture exercises a pooled remainder, not only divisible yield");
    auto result=month(city,"pooled-odd-remainder");
    require(result.delivered==(result.produced+1)/2 && result.noRouteDiscard==result.produced/2,"first row-major farm gets first remainder unit");
    build(city,7,3,BuildingType::Road);build(city,8,3,BuildingType::Road);build(city,9,3,BuildingType::Road);build(city,9,4,BuildingType::Warehouse);
    stocks(city,{{ref::tile(5,2),599},{ref::tile(9,4),0}},7);
    result=month(city,"multiple-stores-row-priority");require(result.delivered>1 && result.fullDiscard==0,"remaining crop spills into second reachable store");
    remove(city,8,3);stocks(city,{{ref::tile(5,2),600},{ref::tile(9,4),0}},7);
    result=month(city,"separate-road-components");require(result.fullDiscard>0 && result.delivered>0,"full store cannot spill into a different component's empty store");
}
void fullBeforeConsumeAndDemolition() {
    auto city=village();build(city,5,3,BuildingType::Road);build(city,6,3,BuildingType::Road);build(city,6,2,BuildingType::Farm);
    stocks(city,{{ref::tile(5,2),600}},7);
    auto result=month(city,"full-before-consumption");
    require(result.produced>0 && result.delivered==0 && result.fullDiscard==result.produced && result.consumed>0 && result.endStored<600,
        "production discarded while full before same-month consumption; no early consumption space");
    std::string message;const auto before=city.saveSnapshot();const auto events=city.events();
    require(!city.demolish(5,2,message) && city.saveSnapshot()==before && city.events()==events,"nonempty warehouse demolition refuses all state mutation");
    stocks(city,{});const auto treasury=city.stats().treasury;
    require(city.demolish(5,2,message) && city.stats().treasury==treasury-2 && city.stats().totalWarehouses==0 && city.stats().foodSupply==0,"empty warehouse demolition costs existing2 only");
    result=month(city,"no-warehouse-after-demolition");require(result.delivered==0 && result.noRouteDiscard==result.produced,"no virtual grain pool after last store removal");
    const auto oldMoney=city.stats().treasury;const auto oldJobs=city.stats().neededWorkers;
    build(city,5,2,BuildingType::Warehouse);
    require(city.stats().treasury==oldMoney-60 && city.stats().neededWorkers==oldJobs+2 && city.warehouseStockAt(5,2)==0,"new empty store costs60 and adds2 jobs");
    month(city,"rebuilt-empty-warehouse");
}
void migration() {
    std::string message;
    auto reject=[&](CitySimulation& city,unsigned x,unsigned y,const std::string& label){context=label;const auto b=city.saveSnapshot();const auto e=city.events();
        write(root/(label+".before.emp"),b);require(!city.migrateToRoadWarehouses(x,y,message) && city.saveSnapshot()==b && city.events()==e,"rejected conversion preserves entire city/events");};
    CitySimulation v1;reject(v1,7,7,"migration-v1");CitySimulation v4(CityRuleset::RoadWarehouses);reject(v4,7,7,"migration-already-v4");
    CitySimulation base(CityRuleset::StaffedServices);base.setSpeed(3);base.tick();
    reject(base,128,0,"migration-x-outside");reject(base,0,128,"migration-y-outside");
    auto occupied=base;build(occupied,7,7,BuildingType::House);reject(occupied,7,7,"migration-occupied");
    auto insufficient=base;auto bytes=base.saveSnapshot();word(bytes,8,59);require(insufficient.loadSnapshot(bytes),"canonical low-funds v3 fixture");reject(insufficient,7,7,"migration-funds59");
    auto overflow=base;bytes=base.saveSnapshot();word(bytes,20,601);require(overflow.loadSnapshot(bytes),"canonical601 v3 grain fixture");reject(overflow,7,7,"migration-stock601");
    auto wet=base;CityMap map;map.width=map.height=128;map.tiles.assign(ref::cells,0);map.tiles[ref::tile(7,7)]=200;
    require(wet.loadFromMap(map),"own v3 water map");reject(wet,7,7,"migration-water");
    for(int grain:{0,400,600}) {
        auto city=base;bytes=base.saveSnapshot();word(bytes,8,60);word(bytes,20,uint32_t(grain));require(city.loadSnapshot(bytes),"canonical conversion boundary input");
        const auto old=city.stats();const auto tick=city.monthAccumulator();
        require(city.migrateToRoadWarehouses(7,7,message),"explicit v3 conversion succeeds at funds60 and stock<=600");invariant(city);
        require(city.warehouseStockAt(7,7)==grain && city.stats().foodSupply==grain && city.stats().treasury==0 && city.stats().neededWorkers==old.neededWorkers+2 &&
            city.stats().year==old.year && city.stats().month==old.month && city.stats().speed==old.speed && city.monthAccumulator()==tick,"grain moved once,60 charged,2jobs added,clock/tick retained");
        write(root/("migration-success-"+std::to_string(grain)+".emp"),city.saveSnapshot());
    }
    CitySimulation downgrade(CityRuleset::StaffedServices);build(downgrade,10,10,BuildingType::Well);
    build(downgrade,13,10,BuildingType::House);build(downgrade,13,11,BuildingType::Road);downgrade.advanceMonth();
    const auto prior=downgrade.stats();require(prior.population==6 && downgrade.serviceEffectiveRadius(BuildingType::Well)==3,"natural v3 threshold conversion fixture");
    require(downgrade.migrateToRoadWarehouses(7,7,message),"conversion adds actual competing warehouse jobs");invariant(downgrade);
    require(downgrade.stats().housingCapacity<prior.housingCapacity && downgrade.stats().population<=downgrade.stats().housingCapacity &&
        downgrade.stats().foodSupply==prior.foodSupply && downgrade.stats().treasury==prior.treasury-60,"conversion stabilizes real housing, not only serialized version");
    // Actual challenge conversion guard belongs to App; CitySimulation has no session parameter.
    std::cout<<"SCOPE\tchallenge-conversion-guard\tAPI contract: App must reject active/terminal challenge conversions\tmodel-only suite cannot prove UI guard; inspect separate frozen-App static review receipt\n";
}
void invalidSnapshotsAndLegacy() {
    CitySimulation live(CityRuleset::RoadWarehouses);const auto good=live.saveSnapshot();const auto events=live.events();std::string error;
    auto reject=[&](const std::vector<uint8_t>& bytes,const std::string& label){context=label;write(root/(label+".emp"),bytes);
        require(!live.loadSnapshot(bytes) && live.saveSnapshot()==good && live.events()==events,"malformed direct load preserves entire target");
        require(!loadCityState(live,root/(label+".emp"),error) && live.saveSnapshot()==good && live.events()==events,"malformed ordinary-file city load preserves target");
        CityScenarioSession session;CityScenarioAwardBook book{{42}};const auto receipt=book.completedRuns;
        require(!loadCityGameState(live,session,book,root/(label+".emp"),error) && live.saveSnapshot()==good && live.events()==events && !session.active() && book.completedRuns==receipt,
            "malformed unified free-state load preserves city/receipts/session");};
    auto bad=good;bad.pop_back();reject(bad,"v4-truncated");bad=good;bad.push_back(0);reject(bad,"v4-tail");
    bad=good;word(bad,4,2);reject(bad,"v4-undefined-version");
    bad=good;word(bad,stockOffset,1);word(bad,20,401);reject(bad,"nonwarehouse-stock");
    bad=good;word(bad,stockOffset+4*ref::tile(5,2),601);word(bad,20,601);reject(bad,"warehouse-over600");
    bad=good;word(bad,stockOffset+4*ref::tile(5,2),0xffffffffu);reject(bad,"negative-warehouse-stock");
    bad=good;word(bad,20,399);reject(bad,"global-stock-mismatch");
    bad=good;word(bad,ledgerOffset,2);reject(bad,"invalid-ledger-flag");
    bad=good;word(bad,ledgerOffset,0);word(bad,ledgerOffset+4,1);reject(bad,"invalid-ledger-nonzero-fields");
    bad=good;word(bad,ledgerOffset,1);word(bad,ledgerOffset+4,400);word(bad,ledgerOffset+8,1);word(bad,ledgerOffset+32,400);reject(bad,"valid-ledger-conservation-broken");
    bad=good;word(bad,ledgerOffset,1);word(bad,ledgerOffset+4,399);word(bad,ledgerOffset+32,399);reject(bad,"valid-ledger-current-stock-mismatch");
    bad=good;word(bad,20,401);word(bad,stockOffset+4*ref::tile(5,2),401);word(bad,ledgerOffset,1);
    word(bad,ledgerOffset+4,400);word(bad,ledgerOffset+24,0xffffffffu);word(bad,ledgerOffset+32,401);
    reject(bad,"negative-consumption-otherwise-conserved");
    unsigned count=0;
    for(const auto& file:std::filesystem::directory_iterator(legacyRoot)) {
        const auto bytes=read(file.path());if(bytes.size()!=oldBytes || word(bytes,4)!=1)continue;
        CitySimulation restored(CityRuleset::RoadWarehouses);CityScenarioSession session;CityScenarioAwardBook book{{42}};
        require(decodeCityGameState(bytes,restored,session,book,error) && restored.saveSnapshot()==bytes && restored.ruleset()==CityRuleset::LegacyServices && !session.active(),"30 old v1 files retain exact old rule and65580 bytes");
        auto control=restored;for(unsigned n=0;n<12;++n){control.advanceMonth();restored.advanceMonth();require(control.saveSnapshot()==restored.saveSnapshot(),"v1 future continuation");}++count;
    }
    require(count==30,"all30 previously captured old files");
    for(auto rules:{CityRuleset::LegacyServices,CityRuleset::StaffedServices}) {
        CitySimulation old(rules);const auto bytes=old.saveSnapshot();require(bytes.size()==oldBytes,"v1/v3 unchanged length");
        CitySimulation copy(CityRuleset::RoadWarehouses);require(copy.loadSnapshot(bytes) && copy.saveSnapshot()==bytes && copy.ruleset()==rules,"ordinary v1/v3 loading is not migration");
        bad=bytes;bad.push_back(0);reject(bad,rules==CityRuleset::LegacyServices?"v1-tail":"v3-tail");
        bad=bytes;bad.at(44+2*ref::cells+ref::tile(5,2))=8;reject(bad,rules==CityRuleset::LegacyServices?"v1-warehouse-enum":"v3-warehouse-enum");
    }
}
void saveContinueAndFeedbackUnknown() {
    auto continuous=village();build(continuous,5,3,BuildingType::Road);build(continuous,6,3,BuildingType::Road);build(continuous,6,2,BuildingType::Farm);
    auto resumed=continuous;std::string error;unsigned checkpoints=0;
    for(unsigned step=0;step<80;++step) {
        auto input=[&](CitySimulation& city){if(step==17)remove(city,6,3);if(step==29)build(city,6,3,BuildingType::Road);
            if(step%13==0)city.setSpeed((step/13)%4);if(step%9==0)city.advanceMonth();else city.tick();};
        input(continuous);input(resumed);require(continuous.saveSnapshot()==resumed.saveSnapshot(),"same input through all stocks,ledger,speed,tick,road changes");invariant(resumed);
        if(step%5==0) {
            const auto file=root/("continuation-"+std::to_string(step)+".emp");CityScenarioSession none;CityScenarioAwardBook book;
            require(saveCityGameState(resumed,none,book,file,error) && read(file)==resumed.saveSnapshot(),"free v4 unified save writes complete131152-byte EMP1");
            CitySimulation fresh;CityScenarioSession freshSession;CityScenarioAwardBook freshBook;
            require(loadCityGameState(fresh,freshSession,freshBook,file,error) && !freshSession.active(),"full v4 unified ordinary-file load into new instances");
            require(fresh.saveSnapshot()==continuous.saveSnapshot(),"every persisted v4 field exact after new-instance load");resumed=std::move(fresh);++checkpoints;
        }
    }
    require(checkpoints==16,"sixteen full v4 file checkpoints");
    CityFeedbackSnapshot missing;missing.harvestRule=CityHarvestRule::RoadWarehousesV1;
    require(!evaluateCityGoals(missing).nextMonth.known,"missing authoritative v4 prediction gives unknown");
    const auto mismatch=evaluateCityGoals(makeCityFeedbackSnapshot(resumed,CityHarvestRule::StaffedServicesV1));
    require(!mismatch.nextMonth.known,"conflicting v3 predictor cannot silently forecast v4");
    CityScenarioSession challenge;CityScenarioAwardBook book;require(challenge.begin(selfAuthoredCityScenarios()[0].spec,900,{resumed.stats().year,resumed.stats().month},book).ok,"ordinary existing scenario config");
    require(encodeCityGameState(resumed,challenge,book,error).empty(),"v4 free-build cannot silently become v3 ECG1 challenge");
    std::cout<<"OBSERVE\tv4-continuation\tdeclared v4 layout and bounded full free-state I/O\t80 identical inputs,16 fresh-instance file checkpoints\n";
}
// These are format-boundary records, not claims of reachable historical production.
// The final API explicitly defines conservative inclusive load bounds independently of map history.
std::vector<uint8_t> boundaryRecord(int stored,const std::array<int,8>& ledger) {
    CitySimulation seed(CityRuleset::RoadWarehouses);auto bytes=seed.saveSnapshot();
    constexpr size_t buildings=44+2*ref::cells, levels=buildings+ref::cells;
    for(unsigned i=0;i<ref::cells;++i){bytes[buildings+i]=0;bytes[levels+i]=0;word(bytes,stockOffset+4*i,0);}
    int remaining=stored;unsigned index=0;
    do {
        bytes[buildings+index]=8;bytes[levels+index]=1;
        const int amount=std::min(600,remaining);word(bytes,stockOffset+4*index,uint32_t(amount));remaining-=amount;++index;
    } while(remaining>0 && index<ref::cells);
    require(remaining==0,"boundary fixture physical per-warehouse capacity");word(bytes,20,uint32_t(stored));
    word(bytes,ledgerOffset,1);for(unsigned i=0;i<8;++i)word(bytes,ledgerOffset+4+4*i,uint32_t(ledger[i]));
    return bytes;
}
void finalLedgerBounds() {
    constexpr int grainMax=9830400, harvestMax=573440, demandMax=78644;
    CitySimulation target(CityRuleset::RoadWarehouses);const auto stable=target.saveSnapshot();const auto events=target.events();std::string error;
    auto accept=[&](const std::string& label,const std::vector<uint8_t>& bytes){context=label;const auto file=root/(label+".emp");write(file,bytes);
        CitySimulation direct;require(direct.loadSnapshot(bytes) && direct.saveSnapshot()==bytes,"inclusive canonical format endpoint accepted exactly");
        CitySimulation ordinary;require(loadCityState(ordinary,file,error) && ordinary.saveSnapshot()==bytes,"inclusive endpoint ordinary-file load");
        CitySimulation unified;CityScenarioSession session;CityScenarioAwardBook book{{42}};
        require(loadCityGameState(unified,session,book,file,error) && unified.saveSnapshot()==bytes && !session.active() && book.completedRuns==std::vector<uint64_t>{42},"inclusive endpoint unified load retains free mode/receipt history");};
    auto reject=[&](const std::string& label,const std::vector<uint8_t>& bytes){context=label;const auto file=root/(label+".emp");write(file,bytes);
        require(!target.loadSnapshot(bytes) && target.saveSnapshot()==stable && target.events()==events,"overbound direct rejection and whole-state preservation");
        require(!loadCityState(target,file,error) && target.saveSnapshot()==stable && target.events()==events,"overbound ordinary-file rejection");
        CityScenarioSession session;CityScenarioAwardBook book{{42}};
        require(!loadCityGameState(target,session,book,file,error) && target.saveSnapshot()==stable && target.events()==events && !session.active() && book.completedRuns==std::vector<uint64_t>{42},"overbound unified rejection preserves all live objects");};
    accept("bound-zero",boundaryRecord(0,{0,0,0,0,0,0,0,0}));
    accept("bound-grain-max",boundaryRecord(grainMax,{grainMax,0,0,0,0,0,0,grainMax}));
    accept("bound-produced-max",boundaryRecord(401,{400,harvestMax,1,harvestMax-1,0,0,0,401}));
    accept("bound-delivered-max",boundaryRecord(harvestMax,{0,harvestMax,harvestMax,0,0,0,0,harvestMax}));
    accept("bound-no-route-max",boundaryRecord(400,{400,harvestMax,0,harvestMax,0,0,0,400}));
    accept("bound-full-discard-max",boundaryRecord(400,{400,harvestMax,0,0,harvestMax,0,0,400}));
    accept("bound-consumed-max",boundaryRecord(0,{demandMax,0,0,0,0,demandMax,0,0}));
    accept("bound-unmet-max",boundaryRecord(400,{400,0,0,0,0,0,demandMax,400}));
    accept("bound-demand-sum-max",boundaryRecord(0,{demandMax/2,0,0,0,0,demandMax/2,demandMax/2,0}));
    reject("bound-begin-plus-one",boundaryRecord(grainMax,{grainMax+1,0,0,0,0,1,0,grainMax}));
    reject("bound-produced-plus-one",boundaryRecord(401,{400,harvestMax+1,1,harvestMax,0,0,0,401}));
    reject("bound-delivered-plus-one",boundaryRecord(harvestMax+1,{0,harvestMax+1,harvestMax+1,0,0,0,0,harvestMax+1}));
    reject("bound-no-route-plus-one",boundaryRecord(400,{400,harvestMax+1,0,harvestMax+1,0,0,0,400}));
    reject("bound-full-discard-plus-one",boundaryRecord(400,{400,harvestMax+1,0,0,harvestMax+1,0,0,400}));
    reject("bound-consumed-plus-one",boundaryRecord(0,{demandMax+1,0,0,0,0,demandMax+1,0,0}));
    reject("bound-unmet-plus-one",boundaryRecord(400,{400,0,0,0,0,0,demandMax+1,400}));
    reject("bound-demand-sum-plus-one",boundaryRecord(0,{demandMax/2+1,0,0,0,0,demandMax/2+1,demandMax/2,0}));
    auto endOver=boundaryRecord(grainMax,{grainMax,0,0,0,0,0,0,grainMax});
    word(endOver,20,grainMax+1);word(endOver,ledgerOffset+4,grainMax+1);word(endOver,ledgerOffset+32,grainMax+1);
    reject("bound-end-plus-one-and-physical-stock-mismatch",endOver);
    std::cout<<"OBSERVE\tfinal-ledger-format-bounds\tAPI-FROZEN inclusive conservative load bounds; reachable history not asserted\t9 accepted endpoints and9 rejected overbounds via3 load APIs; delivered/loss/end constraints are coupled\n";
}
void finalQueryContracts() {
    for(auto rules:{CityRuleset::LegacyServices,CityRuleset::StaffedServices,CityRuleset::RoadWarehouses}) {
        CitySimulation city(rules);const auto initial=city.lastLogisticsMonth();
        require(!initial.valid && values(initial)==std::array<int,8>{},"initial/non-v4 ledger invalid and all zero");
        build(city,20,20,BuildingType::Farm);build(city,21,20,BuildingType::Road);build(city,22,20,BuildingType::House);
        const auto stable=city.saveSnapshot();const auto events=city.events();
        for(const auto& point:std::vector<std::pair<uint32_t,uint32_t>>{{0,0},{20,20},{21,20},{22,20},{128,0},{0,128},{0xffffffffu,0xffffffffu}}) {
            require(city.warehouseStockAt(point.first,point.second)==0,"nonwarehouse and OOB inventory default0");
            if(point!=std::pair<uint32_t,uint32_t>{20,20})require(!city.farmHasWarehouseRoute(point.first,point.second),"nonfarm and OOB route default false");
        }
        const auto overview=city.logisticsOverview();
        if(rules!=CityRuleset::RoadWarehouses) {
            require(!city.farmHasWarehouseRoute(20,20) && overview.warehouses==0 && overview.capacity==0 && overview.connectedFarms==0 && overview.disconnectedFarms==0 && overview.stored==city.stats().foodSupply,"non-v4 defaults keep old food pool visible but no warehouse/routes");
            city.advanceMonth();require(!city.lastLogisticsMonth().valid && values(city.lastLogisticsMonth())==std::array<int,8>{},"non-v4 month never fabricates logistics record");
        } else require(city.saveSnapshot()==stable && city.events()==events,"v4 queries are read-only");
    }
    CitySimulation full(CityRuleset::RoadWarehouses);build(full,5,3,BuildingType::Road);build(full,6,3,BuildingType::Farm);
    stocks(full,{{ref::tile(5,2),600}});require(full.farmHasWarehouseRoute(6,3) && full.logisticsOverview().connectedFarms==1,"full warehouse retains structural route independent of capacity/labor");
    CitySimulation converted(CityRuleset::StaffedServices);std::string message;require(converted.migrateToRoadWarehouses(7,7,message),"conversion fixture");
    require(!converted.lastLogisticsMonth().valid && values(converted.lastLogisticsMonth())==std::array<int,8>{},"conversion resets logistics valid=false/eight zero fields");
}
} // namespace
int main(int argc,char** argv) {
    if(argc!=3)return 2;root=argv[1];legacyRoot=argv[2];std::filesystem::create_directories(root);
    test("initial-official-warehouse-map-fallback","WAREHOUSE_ROADS_API.md initial cell/400/5000/2jobs/maintenance; first land and all-water atomicity",initializationAndMaps);
    test("four-neighbor-routes-cut-rejoin-boundary","WAREHOUSE_ROADS_API.md roads-only components,no diagonal/direct link,no backlog",topologyAndReconnection);
    test("fair-pooled-shares-multiple-stores-components","WAREHOUSE_ROADS_API.md pooled exact helper,farm/warehouse row priority and capacity",fairFarmSharesAndMultipleStores);
    test("full-before-consume-no-cache-demolition","WAREHOUSE_ROADS_API.md input before old-population consumption; nonempty reject; emptycost2,newcost60/jobs2",fullBeforeConsumeAndDemolition);
    test("explicit-v3-conversion-atomic-boundaries","WAREHOUSE_ROADS_API.md v3-only,empty land,food<=600,treasury>=60,atomic conversion",migration);
    test("v4-malformed-v1-v3-preservation","WAREHOUSE_ROADS_API.md exact131152/65580 layouts,zero nonwarehouse,canonical ledger,transactional load",invalidSnapshotsAndLegacy);
    test("v4-save-continue-authoritative-feedback","WAREHOUSE_ROADS_API.md full free-v4 I/O and actual logistics prediction; conflicts unknown,no v4 challenge",saveContinueAndFeedbackUnknown);
    test("final-inclusive-ledger-format-bounds","Final WAREHOUSE_ROADS_API.md9830400/573440/78644 load limits,inclusive,nonnegative,coupled conservation",finalLedgerBounds);
    test("final-query-defaults-and-ledger-lifecycle","Final WAREHOUSE_ROADS_API.md nonwarehouse/OOB0,nonfarm/non-v4/OOBfalse,full-route independent,initial/conversion invalid",finalQueryContracts);
    std::cout<<"SUMMARY\tfailures="<<failures<<'\n';return failures?1:0;
}
