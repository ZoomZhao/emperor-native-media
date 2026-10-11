#include "city_game_state.hpp"
#include "app/city_goal_feedback_adapter.hpp"
#include "../scripts/self_fixture_evidence.hpp"
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
using namespace game_lab;
namespace {
int checks=0;
void require(bool ok,const char* why) { ++checks; if (!ok) {std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);} }
void put(std::vector<uint8_t>& bytes,size_t offset,uint32_t value) {
    for(unsigned n=0;n<4;++n) bytes[offset+n]=uint8_t(value>>(8*n));
}
void build(CitySimulation& c,uint32_t x,uint32_t y,BuildingType type) {
    std::string message;require(c.build(x,y,type,message),"self-authored grid build");
}
void remove(CitySimulation& c,uint32_t x,uint32_t y) {
    std::string message;require(c.demolish(x,y,message),"self-authored grid demolition");
}
void stocks(CitySimulation& c,const std::vector<std::pair<size_t,int>>& values) {
    auto bytes=c.saveSnapshot();require(bytes.size()==131152,"specified v4 layout");
    int total=0;
    for(size_t i=0;i<CitySimulation::kTotalTiles;++i) put(bytes,65580+4*i,0);
    for(auto [tile,value]:values) {put(bytes,65580+4*tile,uint32_t(value));total+=value;}
    put(bytes,20,uint32_t(total));
    for(size_t offset=131116;offset<131152;offset+=4) put(bytes,offset,0);
    self_fixture::bytes("authored-warehouse-stock",bytes);
    require(c.loadSnapshot(bytes),"canonical self-authored stock fixture");
}
constexpr size_t initial=2*128+5;
CitySimulation staffed() {
    CitySimulation c(CityRuleset::RoadWarehouses);c.setSpeed(0);
    for(unsigned n=0;n<6;++n) build(c,10+3*n,10,BuildingType::House);
    for(unsigned n=0;n<4;++n) c.advanceMonth();
    require(c.stats().population==24,"warehouse bootstrap reaches24 without changing housing");
    return c;
}
CitySimulation linked() {
    auto c=staffed();build(c,8,2,BuildingType::Farm);
    build(c,6,2,BuildingType::Road);build(c,7,2,BuildingType::Road);
    require(c.farmHasWarehouseRoute(8,2),"two-road warehouse connection");return c;
}
void ledger(const CitySimulation& c) {
    const auto& r=c.lastLogisticsMonth();require(r.valid,"monthly ledger present");
    require(int64_t(r.beginStored)+r.produced==int64_t(r.endStored)+r.consumed+r.noRouteDiscard+r.fullDiscard,"independent total grain conservation");
    require(r.produced==r.delivered+r.noRouteDiscard+r.fullDiscard,"all production admitted or explicitly lost");
    int total=0;bool bounded=true;
    for(unsigned y=0;y<128;++y) for(unsigned x=0;x<128;++x) {
        const int amount=c.warehouseStockAt(x,y);bounded = bounded && amount>=0 && amount<=600;total+=amount;
    }
    require(bounded,"all per-warehouse stocks within capacity");
    require(total==c.stats().foodSupply && total==r.endStored,"actual per-tile grain equals displayed balance");
}
bool routeOracle(const CitySimulation& c,unsigned fx,unsigned fy) {
    if(c.buildingAt(fx,fy)!=BuildingType::Farm) return false;
    std::queue<std::pair<int,int>> queue;std::set<std::pair<int,int>> visited;
    const int dx[]={-1,1,0,0},dy[]={0,0,-1,1};
    const auto road=[&](int x,int y) {return x>=0 && y>=0 && x<128 && y<128 && c.buildingAt(unsigned(x),unsigned(y))==BuildingType::Road;};
    for(int d=0;d<4;++d) if(road(int(fx)+dx[d],int(fy)+dy[d])) {
        const auto p=std::make_pair(int(fx)+dx[d],int(fy)+dy[d]);queue.push(p);visited.insert(p);
    }
    while(!queue.empty()) {
        const auto [x,y]=queue.front();queue.pop();
        for(int d=0;d<4;++d) {
            const int nx=x+dx[d],ny=y+dy[d];
            if(nx>=0 && ny>=0 && nx<128 && ny<128 && c.buildingAt(unsigned(nx),unsigned(ny))==BuildingType::Warehouse) return true;
            if(road(nx,ny) && visited.insert({nx,ny}).second) queue.push({nx,ny});
        }
    }
    return false;
}
void projection(const CitySimulation& c) {
    const auto before=c.saveSnapshot();const auto events=c.events();
    const auto feedback=evaluateCityGoals(makeCityFeedbackSnapshot(c,CityHarvestRule::RoadWarehousesV1));
    require(feedback.valid && feedback.nextMonth.known,"warehouse forecast known");
    auto future=c;future.advanceMonth();const auto& p=feedback.nextMonth;const auto& r=future.lastLogisticsMonth();
    require(p.harvest==r.delivered && p.produced==r.produced,"preview uses admitted grain and separate production");
    require(p.foodAfter==future.stats().foodSupply && p.populationAfter==future.stats().population &&
            p.treasuryAfter==future.stats().treasury,"preview equals real month food population treasury");
    require(c.saveSnapshot()==before && c.events()==events,"forecast never changes source inventory or events");
}
}
int main(int argc,char** argv) {
    require(argc==2,"isolated output directory required");const std::filesystem::path dir(argv[1]);std::filesystem::create_directories(dir);
    std::string message,error;
    CitySimulation initialCity(CityRuleset::RoadWarehouses);
    require(initialCity.warehouseStockAt(5,2)==400 && initialCity.stats().foodSupply==400,"initial grain physically at5,2");
    require(initialCity.stats().totalWarehouses==1 && initialCity.stats().grainStorageCapacity==600 &&
            initialCity.stats().neededWorkers==2 && initialCity.stats().treasury==5000,"initial configuration and warehouse jobs");
    const auto fresh=initialCity.saveSnapshot();const auto events=initialCity.events();
    require(!initialCity.demolish(5,2,message) && initialCity.saveSnapshot()==fresh && initialCity.events()==events,"stocked warehouse demolition refuses transactionally");
    require(!initialCity.build(128,0,BuildingType::Warehouse,message) && initialCity.saveSnapshot()==fresh,"invalid warehouse coordinate preserves city");
    require(!initialCity.build(5,2,BuildingType::Warehouse,message) && initialCity.saveSnapshot()==fresh,"occupied warehouse refuses without fee");
    build(initialCity,8,2,BuildingType::Farm);build(initialCity,6,2,BuildingType::Road);build(initialCity,7,2,BuildingType::Road);
    projection(initialCity);initialCity.advanceMonth();
    require(initialCity.lastLogisticsMonth().produced==0 && initialCity.lastLogisticsMonth().delivered==0,"zero labor cannot produce or transport grain");ledger(initialCity);

    auto c=linked();projection(c);const int before=c.stats().foodSupply;c.advanceMonth();
    require(c.lastLogisticsMonth().produced==5 && c.lastLogisticsMonth().delivered==5 && c.lastLogisticsMonth().consumed==3 && c.stats().foodSupply==before+2,"full labor route5 minus consumption3");ledger(c);
    remove(c,6,2);require(!c.farmHasWarehouseRoute(8,2),"cut road disconnects immediately");projection(c);c.advanceMonth();
    require(c.lastLogisticsMonth().produced==5 && c.lastLogisticsMonth().delivered==0 && c.lastLogisticsMonth().noRouteDiscard==5,"cut road has zero transport and explicit loss");ledger(c);
    build(c,6,2,BuildingType::Road);projection(c);c.advanceMonth();
    require(c.lastLogisticsMonth().produced==35 && c.lastLogisticsMonth().delivered==35 && c.lastLogisticsMonth().noRouteDiscard==0,"reconnection in autumn restores current35 grain without backfill");ledger(c);

    auto diagonal=staffed();build(diagonal,8,2,BuildingType::Farm);
    for(unsigned x=6;x<=8;++x) build(diagonal,x,3,BuildingType::Road);
    require(!diagonal.farmHasWarehouseRoute(8,2),"diagonal warehouse contact does not connect");
    build(diagonal,5,3,BuildingType::Road);require(diagonal.farmHasWarehouseRoute(8,2),"long orthogonal detour connects");
    auto touching=staffed();build(touching,4,2,BuildingType::Farm);require(!touching.farmHasWarehouseRoute(4,2),"direct farm-warehouse adjacency without road refuses");
    build(touching,4,3,BuildingType::Road);build(touching,5,3,BuildingType::Road);require(touching.farmHasWarehouseRoute(4,2),"actual road connecting adjacent buildings succeeds");

    auto full=linked();stocks(full,{{initial,600}});projection(full);full.advanceMonth();
    require(full.lastLogisticsMonth().delivered==0 && full.lastLogisticsMonth().fullDiscard==5 && full.lastLogisticsMonth().consumed==3 && full.stats().foodSupply==597,"production sees full capacity before later consumption");ledger(full);
    projection(full);full.advanceMonth();require(full.lastLogisticsMonth().delivered==3 && full.lastLogisticsMonth().fullDiscard==2 && full.stats().foodSupply==597,"only current pre-consumption vacancy admits grain");ledger(full);
    auto many=linked();build(many,10,2,BuildingType::Warehouse);build(many,9,2,BuildingType::Road);
    stocks(many,{{initial,600},{2*128+10,100}});projection(many);many.advanceMonth();
    require(many.warehouseStockAt(5,2)==597 && many.warehouseStockAt(10,2)==105,"reachable full first warehouse spills into second; consumption remains row-first");ledger(many);
    require(many.logisticsOverview().warehouses==2 && many.logisticsOverview().capacity==1200,"aggregate warehouse capacity derived from actual buildings");

    auto missing=staffed();stocks(missing,{{initial,0}});remove(missing,5,2);build(missing,8,2,BuildingType::Farm);
    projection(missing);missing.advanceMonth();require(missing.lastLogisticsMonth().noRouteDiscard==5 &&
        missing.lastLogisticsMonth().consumed==0 && missing.lastLogisticsMonth().unmetDemand==3 && missing.stats().population==20,"no warehouse loses harvest; no negative inventory; actual famine4");ledger(missing);

    CitySimulation pooled(CityRuleset::RoadWarehouses);pooled.setSpeed(0);build(pooled,10,10,BuildingType::House);build(pooled,13,10,BuildingType::House);pooled.advanceMonth();
    require(pooled.stats().population==6,"pooled fixture6 workers3");build(pooled,8,2,BuildingType::Farm);build(pooled,16,2,BuildingType::Farm);
    build(pooled,6,2,BuildingType::Road);build(pooled,7,2,BuildingType::Road);
    require(pooled.farmYieldForMonth(3)==1,"pooled rounding retains one grain at3/18 jobs");pooled.advanceMonth();
    require(pooled.lastLogisticsMonth().produced==1 && pooled.lastLogisticsMonth().delivered==1 && pooled.lastLogisticsMonth().noRouteDiscard==0,"single remainder goes first farm; zero-share disconnected farm invents no loss");ledger(pooled);

    auto edge=staffed();build(edge,127,0,BuildingType::Farm);build(edge,0,1,BuildingType::Warehouse);build(edge,0,0,BuildingType::Road);
    require(!edge.farmHasWarehouseRoute(127,0),"map edge does not wrap to next row");
    build(edge,126,0,BuildingType::Warehouse);build(edge,126,1,BuildingType::Road);build(edge,127,1,BuildingType::Road);
    require(edge.farmHasWarehouseRoute(127,0),"legal last-column orthogonal route connects");

    auto random=staffed();uint32_t seed=0x71a209u;std::vector<std::pair<unsigned,unsigned>> farms;
    for(unsigned n=0;n<180;++n) {
        seed=1664525u*seed+1013904223u;const unsigned x=32+(seed>>16)%16;
        seed=1664525u*seed+1013904223u;const unsigned y=32+(seed>>16)%16;
        if(random.buildingAt(x,y)!=BuildingType::None) continue;
        const auto type=n%17==0 ? BuildingType::Warehouse : n%11==0 ? BuildingType::Farm : BuildingType::Road;
        build(random,x,y,type);if(type==BuildingType::Farm) farms.push_back({x,y});
    }
    self_fixture::numbers("road-oracle-seed",{0x71a209u,180});
    for(auto [x,y]:farms) require(random.farmHasWarehouseRoute(x,y)==routeOracle(random,x,y),"independent farm-started flood-fill oracle agrees");

    CityMap map;map.width=map.height=128;map.tiles.assign(16384,200);
    const auto old=initialCity.saveSnapshot();const auto oldEvents=initialCity.events();
    require(!initialCity.loadFromMap(map) && initialCity.saveSnapshot()==old && initialCity.events()==oldEvents,"all-water warehouse map rejects without resetting");
    map.tiles[0]=0;require(initialCity.loadFromMap(map) && initialCity.warehouseStockAt(0,0)==400 && initialCity.warehouseStockAt(5,2)==0,"water at initial site relocates to first dry tile");

    for(auto rule:{CityRuleset::LegacyServices,CityRuleset::StaffedServices}) {
        CitySimulation oldCity(rule);build(oldCity,1,1,BuildingType::House);
        const auto raw=oldCity.saveSnapshot();const auto oldHistory=oldCity.events();require(raw.size()==65580,"legacy save length stays exact");
        CitySimulation restored;require(restored.loadSnapshot(raw) && restored.saveSnapshot()==raw && restored.ruleset()==rule,"legacy load retains original rules and all bytes");
        require(!oldCity.build(5,2,BuildingType::Warehouse,message) && oldCity.saveSnapshot()==raw && oldCity.events()==oldHistory,"legacy city cannot accidentally build new warehouse enum");
        auto corrupted=raw;corrupted[44+32768+5]=8;require(!restored.loadSnapshot(corrupted) && restored.saveSnapshot()==raw,"legacy layout rejects warehouse enum");
        require(saveCityState(oldCity,dir/(rule==CityRuleset::LegacyServices ? "v1.emp":"v3.emp"),error),"legacy native I/O saves exact format");
        if(rule==CityRuleset::LegacyServices) require(!oldCity.migrateToRoadWarehouses(5,2,message) && oldCity.saveSnapshot()==raw,"v1 remains unchanged and conversion explicitly refuses");
        else {
            const int treasury=oldCity.stats().treasury,grain=oldCity.stats().foodSupply;
            require(oldCity.migrateToRoadWarehouses(5,2,message) && oldCity.ruleset()==CityRuleset::RoadWarehouses &&
                oldCity.stats().foodSupply==grain && oldCity.warehouseStockAt(5,2)==grain && oldCity.stats().treasury==treasury-60,"explicit v3 conversion moves all grain and charges one construction fee");
        }
    }
    for(int kind=0;kind<3;++kind) {
        CitySimulation upgrade(CityRuleset::StaffedServices);auto raw=upgrade.saveSnapshot();
        if(kind==0) put(raw,20,601);
        if(kind==1) put(raw,8,59);
        require(upgrade.loadSnapshot(raw),"own conversion boundary fixture");
        const auto history=upgrade.events();
        require(!upgrade.migrateToRoadWarehouses(kind==2 ? 128u:5u,2,message) && upgrade.saveSnapshot()==raw && upgrade.events()==history,"capacity funds or coordinate conversion failure preserves entire state");
    }

    auto round=linked();for(unsigned n=0;n<24;++n) {
        projection(round);round.advanceMonth();ledger(round);
        const auto bytes=round.saveSnapshot();CitySimulation restored;
        require(restored.loadSnapshot(bytes) && restored.saveSnapshot()==bytes,"v4 complete per-tile stock and monthly ledger roundtrip");
        auto next=round;next.advanceMonth();restored.advanceMonth();require(restored.saveSnapshot()==next.saveSnapshot(),"same future from24 independent restored instances");
        if(n==8) remove(round,6,2);if(n==12) build(round,6,2,BuildingType::Road);
    }
    const auto stable=round.saveSnapshot();const auto stableEvents=round.events();
    for(int kind=0;kind<10;++kind) {
        auto bad=stable;
        if(kind==0) bad.pop_back();
        if(kind==1) bad.push_back(0);
        if(kind==2) put(bad,65580+4*initial,601);
        if(kind==3) put(bad,65580,1);
        if(kind==4) put(bad,20,uint32_t(round.stats().foodSupply+1));
        if(kind==5) put(bad,131116,2);
        if(kind==6) put(bad,131128,999999);
        if(kind==7) put(bad,131116,0);
        if(kind==8) put(bad,4,3);
        if(kind==9) put(bad,131144,0xffffffffu);
        self_fixture::bytes("damaged-v4",bad);
        require(!round.loadSnapshot(bad) && round.saveSnapshot()==stable && round.events()==stableEvents,"damaged v4 transaction rejects all partial changes");
    }
    require(saveCityState(round,dir/"warehouse.emp",error),"v4 native state save");CitySimulation io;
    require(loadCityState(io,dir/"warehouse.emp",error) && io.saveSnapshot()==stable,"v4 native state load preserves inventory and ledger");
    CityScenarioSession none;CityScenarioAwardBook book;require(saveCityGameState(round,none,book,dir/"warehouse-game.emp",error),"App whole-save API handles v4 free city");
    CityScenarioSession loadedSession;CityScenarioAwardBook loadedBook;
    require(loadCityGameState(io,loadedSession,loadedBook,dir/"warehouse-game.emp",error) && !loadedSession.active() && io.saveSnapshot()==stable,"v4 load stays free mode without invented challenge");
    std::filesystem::create_symlink(dir/"warehouse.emp",dir/"warehouse-link.emp");const auto ioBefore=io.saveSnapshot();
    require(!loadCityState(io,dir/"warehouse-link.emp",error) && io.saveSnapshot()==ioBefore,"v4 file symlink refuses without replacing current city");
    require(!saveCityGameState(round,none,book,dir/"missing/warehouse.emp",error) && round.saveSnapshot()==stable,"v4 save failure preserves city and inventory");
    auto snapshot=makeCityFeedbackSnapshot(round,CityHarvestRule::RoadWarehousesV1);snapshot.coreDelivered=999999;
    require(!evaluateCityGoals(snapshot).nextMonth.known,"conflicting delivered helper refuses forecast");
    snapshot=makeCityFeedbackSnapshot(round,CityHarvestRule::RoadWarehousesV1);snapshot.coreDelivered.reset();
    require(!evaluateCityGoals(snapshot).nextMonth.known,"missing delivered helper never falls back to production");
    require(makeCityFeedbackSnapshot(round,CityHarvestRule::StaffedServicesV1).harvestRule==CityHarvestRule::Unknown,"warehouse core refuses old forecast profile");
    std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"ruleset\":4,\"restored_months\":24,\"inventory_conservation\":true,\"roads\":\"four-neighbor\",\"devices\":false}"<<'\n';
}
