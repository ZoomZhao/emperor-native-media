#include "city_game_state.hpp"
#include "app/city_warehouse_routes.hpp"
#include "../scripts/self_fixture_evidence.hpp"
#include <iostream>
#include <limits>
#include <queue>
#include <set>
using namespace game_lab;
namespace {
int checks=0;
void require(bool value,const char* reason) {++checks;if(!value){std::cerr<<"FAIL: "<<reason<<'\n';std::exit(1);}}
void build(CitySimulation& c,unsigned x,unsigned y,BuildingType type) {std::string m;require(c.build(x,y,type,m),"authored dry grid build");}
void remove(CitySimulation& c,unsigned x,unsigned y) {std::string m;require(c.demolish(x,y,m),"authored empty road demolition");}
CitySimulation empty() {CitySimulation c(CityRuleset::RoadWarehouses);c.setSpeed(0);return c;}
// Independent farm-started flood fill, using only public cell types and coordinates.
// It neither labels production components nor calls any production route query.
std::pair<std::vector<size_t>,std::vector<size_t>> oracle(const CitySimulation& c,unsigned fx,unsigned fy) {
    std::set<size_t> visited,warehouses;
    std::queue<std::pair<int,int>> pending;
    constexpr int dx[]={-1,1,0,0},dy[]={0,0,-1,1};
    const auto inside=[](int x,int y){return x>=0&&y>=0&&x<128&&y<128;};
    const auto offer=[&](int x,int y) {
        if(inside(x,y)&&c.buildingAt(x,y)==BuildingType::Road&&visited.insert(size_t(y)*128+x).second)
            pending.push({x,y});
    };
    for(int n=0;n<4;++n)offer(int(fx)+dx[n],int(fy)+dy[n]);
    while(!pending.empty()) {
        auto [x,y]=pending.front();pending.pop();
        for(int n=0;n<4;++n) {
            const int nx=x+dx[n],ny=y+dy[n];
            if(inside(nx,ny)&&c.buildingAt(nx,ny)==BuildingType::Warehouse)warehouses.insert(size_t(ny)*128+nx);
            offer(nx,ny);
        }
    }
    return {{visited.begin(),visited.end()},{warehouses.begin(),warehouses.end()}};
}
CityFarmWarehouseRoute check(const CitySimulation& c,unsigned x,unsigned y) {
    const auto before=c.saveSnapshot();const auto events=c.events();
    const auto q=c.farmWarehouseRoute(x,y);const auto expected=oracle(c,x,y);
    require(q.valid&&q.farmTile==size_t(y)*128+x,"valid selected farm identity");
    require(q.roadTiles==expected.first,"exact sorted road range equals independent BFS");
    require(q.warehouseTiles==expected.second,"exact deduplicated warehouses equal independent BFS");
    int stored=0;for(size_t i:expected.second)stored+=c.warehouseStockAt(i%128,i/128);
    require(q.capacity==int(expected.second.size())*600&&q.stored==stored&&q.freeCapacity==q.capacity-stored,"capacity and vacancy from actual reachable stocks");
    require(q.hasWorkers==(c.stats().availableWorkers>0),"worker fact from current city");
    require(c.farmHasWarehouseRoute(x,y)==!expected.second.empty(),"existing route boolean agrees");
    require(c.saveSnapshot()==before&&c.events()==events,"range query preserves all city bytes and events");
    return q;
}
void invalid(const CitySimulation& c,unsigned x,unsigned y) {
    const auto before=c.saveSnapshot();const auto events=c.events();const auto q=c.farmWarehouseRoute(x,y);
    require(!q.valid&&q.roadTiles.empty()&&q.warehouseTiles.empty()&&q.capacity==0&&q.stored==0&&q.freeCapacity==0,"nonfarm/old-rule/out-of-bounds query returns empty result");
    require(c.saveSnapshot()==before&&c.events()==events,"invalid query also preserves state");
}
void put(std::vector<uint8_t>& bytes,size_t offset,uint32_t value) {for(unsigned n=0;n<4;++n)bytes[offset+n]=uint8_t(value>>(8*n));}
void stocks(CitySimulation& c,const std::vector<std::pair<size_t,int>>& values) {
    auto bytes=c.saveSnapshot();require(bytes.size()==131152,"canonical own v4 layout");int total=0;
    for(size_t i=0;i<16384;++i)put(bytes,65580+4*i,0);
    for(auto [i,value]:values){put(bytes,65580+4*i,value);total+=value;}
    put(bytes,20,total);for(size_t o=131116;o<131152;o+=4)put(bytes,o,0);
    self_fixture::bytes("own-route-stock-fixture",bytes);require(c.loadSnapshot(bytes),"own canonical stock fixture accepted");
}
bool contains(const std::string& text,const char* part){return text.find(part)!=std::string::npos;}
}
int main(int argc,char** argv) {
    require(argc==2,"own output directory required");std::filesystem::path dir(argv[1]);std::filesystem::create_directories(dir);
    auto c=empty();build(c,8,2,BuildingType::Farm);
    require(check(c,8,2).warehouseTiles.empty(),"no road rejects direct proximity");
    build(c,7,3,BuildingType::Road);require(check(c,8,2).roadTiles.empty(),"diagonal farm contact excluded");
    build(c,6,2,BuildingType::Road);build(c,7,2,BuildingType::Road);
    auto q=check(c,8,2);require(q.roadTiles.size()==3&&q.warehouseTiles==std::vector<size_t>{261}&&q.freeCapacity==200,"real orthogonal route includes reachable dead-end road and initial warehouse");
    require(contains(describeCityFarmWarehouseRoute(q),"余量200担")&&contains(describeCityFarmWarehouseRoute(q),"当前无工"),"numeric and zero-worker explanations");
    remove(c,6,2);q=check(c,8,2);require(q.warehouseTiles.empty()&&q.roadTiles.size()==2,"cut recomputes range without old warehouse");
    require(contains(describeCityFarmWarehouseRoute(q),"道路未通仓"),"disconnected explanation");
    build(c,6,2,BuildingType::Road);require(check(c,8,2).warehouseTiles.size()==1,"reconnection restores current warehouse");
    build(c,12,2,BuildingType::Warehouse);for(unsigned x=9;x<=11;++x)build(c,x,2,BuildingType::Road);
    require(check(c,8,2).warehouseTiles.size()==2,"farm multiple entrances union separate road components");
    stocks(c,{{261,600},{268,600}});q=check(c,8,2);
    require(q.freeCapacity==0&&contains(describeCityFarmWarehouseRoute(q),"官仓已满"),"full connected warehouses explained separately from road failure");
    stocks(c,{{261,600},{268,599}});require(check(c,8,2).freeCapacity==1,"exact one vacancy rather than optimistic pooled estimate");
    std::string message;require(c.transferWarehouseStock(5,2,12,2,1,message),"real stock transfer allowed");require(check(c,8,2).freeCapacity==1,"stock transfer preserves reachable aggregate vacancy");
    // A warehouse touching two components must be counted once and must not bridge them.
    auto bridge=empty();build(bridge,20,20,BuildingType::Farm);build(bridge,20,18,BuildingType::Warehouse);
    for(auto [x,y]:std::vector<std::pair<unsigned,unsigned>>{{20,19},{19,20},{19,19},{19,18},{20,17},{21,17}})build(bridge,x,y,BuildingType::Road);
    q=check(bridge,20,20);require(q.warehouseTiles.size()==1&&q.roadTiles.size()==4,"warehouse dedup over multiple entrances without warehouse bridging");
    build(bridge,21,18,BuildingType::Farm);q=check(bridge,21,18);require(q.roadTiles.size()==2&&q.warehouseTiles.size()==1,"second farm sees only its own component; warehouse does not bridge to other roads");
    auto touching=empty();build(touching,4,2,BuildingType::Farm);require(check(touching,4,2).warehouseTiles.empty(),"direct farm-warehouse adjacency alone has no route");
    auto edge=empty();build(edge,127,0,BuildingType::Farm);build(edge,0,1,BuildingType::Warehouse);build(edge,0,0,BuildingType::Road);
    require(check(edge,127,0).roadTiles.empty(),"last-column edge never wraps");build(edge,126,0,BuildingType::Warehouse);build(edge,126,1,BuildingType::Road);build(edge,127,1,BuildingType::Road);
    require(check(edge,127,0).warehouseTiles==std::vector<size_t>{126},"legal edge detour reaches exact warehouse");
    CityMap own;own.width=own.height=128;own.tiles.assign(16384,0);own.tiles[20*128+21]=200;
    auto water=empty();require(water.loadFromMap(own),"self-created terrain map");build(water,20,20,BuildingType::Farm);build(water,22,20,BuildingType::Warehouse);
    require(!water.build(21,20,BuildingType::Road,message)&&check(water,20,20).warehouseTiles.empty(),"water gap has no fictional path");
    for(auto [x,y]:std::vector<std::pair<unsigned,unsigned>>{{20,21},{21,21},{22,21}})build(water,x,y,BuildingType::Road);
    require(check(water,20,20).warehouseTiles.size()==1,"actual detour around water connects");
    // Seeded authored grids have varied components, dead ends, farms and warehouses.
    uint32_t seed=0x81db427u;self_fixture::numbers("own-route-grid-seed",{seed,32,120});
    size_t randomFarms=0;
    for(unsigned round=0;round<32;++round) {
        auto random=empty();std::vector<std::pair<unsigned,unsigned>> farms;
        for(unsigned n=0;n<120;++n) {
            seed=1664525u*seed+1013904223u;const unsigned x=40+(seed>>16)%12;
            seed=1664525u*seed+1013904223u;const unsigned y=40+(seed>>16)%12;
            if(random.buildingAt(x,y)!=BuildingType::None)continue;
            auto type=n%19==0?BuildingType::Warehouse:n%13==0?BuildingType::Farm:BuildingType::Road;
            build(random,x,y,type);if(type==BuildingType::Farm)farms.push_back({x,y});
        }
        for(auto [x,y]:farms){check(random,x,y);++randomFarms;}
    }
    require(randomFarms>100,"many independent seeded farm comparisons actually executed");
    invalid(c,0,0);invalid(c,5,2);invalid(c,128,2);invalid(c,8,128);invalid(c,std::numeric_limits<unsigned>::max(),2);
    for(auto rule:{CityRuleset::LegacyServices,CityRuleset::StaffedServices}) {CitySimulation old(rule);build(old,8,2,BuildingType::Farm);invalid(old,8,2);}
    for(unsigned x=20;x<38;x+=3)build(c,x,10,BuildingType::House);
    for(unsigned n=0;n<4;++n){c.advanceMonth();check(c,8,2);}
    require(check(c,8,2).hasWorkers,"worker explanation updates after actual housing and months");
    require(!contains(describeCityFarmWarehouseRoute(check(c,8,2)),"当前无工"),"stale worker reason disappears");
    CityScenarioSession no;CityScenarioAwardBook book;require(saveCityGameState(c,no,book,dir/"routes.emp",message),"native format own file save");
    CitySimulation restored;require(loadCityGameState(restored,no,book,dir/"routes.emp",message)&&restored.saveSnapshot()==c.saveSnapshot(),"new instance restores exact bytes");
    auto original=check(c,8,2);auto again=check(restored,8,2);require(original.roadTiles==again.roadTiles&&original.warehouseTiles==again.warehouseTiles&&original.freeCapacity==again.freeCapacity,"routes recompute from saved buildings and stock");
    remove(restored,6,2);require(check(restored,8,2).warehouseTiles.size()==1,"restored city removal updates only current query");
    require(restored.loadSnapshot(c.saveSnapshot()),"replace changed city from own snapshot");require(check(restored,8,2).warehouseTiles.size()==2,"load cannot retain old cut mask");
    restored.reset();invalid(restored,8,2);
    require(contains(describeCityFarmWarehouseRoute({}),"勘查农庄"),"empty selection shows legend");
    std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"independent_bfs\":true,\"seeded_grids\":32,\"seeded_farms\":"<<randomFarms<<",\"read_only\":true,\"devices\":false}\n";
}
