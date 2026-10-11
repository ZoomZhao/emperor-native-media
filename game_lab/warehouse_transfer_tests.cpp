#include "city_game_state.hpp"
#include "app/city_warehouse_transfer.hpp"
#include "../scripts/self_fixture_evidence.hpp"
#include <iostream>
#include <limits>
using namespace game_lab;
namespace {
int checks=0;
void require(bool ok,const char* why){++checks;if(!ok){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
void build(CitySimulation& c,unsigned x,unsigned y,BuildingType type){std::string m;require(c.build(x,y,type,m),"own dry-map build");}
CitySimulation city(){CitySimulation c(CityRuleset::RoadWarehouses);c.setSpeed(0);build(c,10,2,BuildingType::Warehouse);build(c,15,2,BuildingType::Warehouse);return c;}
void put(std::vector<uint8_t>& b,size_t o,uint32_t n){for(unsigned k=0;k<4;++k)b[o+k]=uint8_t(n>>(k*8));}
void seed(CitySimulation& c,int a,int b,int d){auto bytes=c.saveSnapshot();for(size_t i=0;i<128*128;++i)put(bytes,65580+4*i,0);put(bytes,65580+4*(2*128+5),a);put(bytes,65580+4*(2*128+10),b);put(bytes,65580+4*(2*128+15),d);put(bytes,20,a+b+d);for(size_t o=131116;o<131152;o+=4)put(bytes,o,0);self_fixture::bytes("own-transfer-stock-fixture",bytes);require(c.loadSnapshot(bytes),"canonical authored stock fixture");}
void invariant(const CitySimulation& c,int expected){int sum=0;bool bounded=true;for(unsigned y=0;y<128;++y)for(unsigned x=0;x<128;++x){const int n=c.warehouseStockAt(x,y);sum+=n;bounded=bounded&&n>=0&&n<=600;}require(bounded&&sum==expected&&c.stats().foodSupply==expected,"independent stock sum/capacity/global conservation");}
void reject(CitySimulation& c,unsigned sx,unsigned sy,unsigned dx,unsigned dy,int64_t n){const auto b=c.saveSnapshot();const auto e=c.events();std::string m;require(!c.transferWarehouseStock(sx,sy,dx,dy,n,m),"invalid transfer rejected");require(c.saveSnapshot()==b&&c.events()==e,"rejection preserves full snapshot and events");require(!m.empty(),"rejection explains reason");}
}
int main(int argc,char** argv){
    require(argc==2,"own output path required");const auto dir=std::filesystem::path(argv[1]);std::filesystem::create_directories(dir);
    auto c=city();const int cash=c.stats().treasury;const auto year=c.stats().year;const auto month=c.stats().month;
    std::string m;require(c.transferWarehouseStock(5,2,10,2,125,m),"partial transfer success");require(c.warehouseStockAt(5,2)==275&&c.warehouseStockAt(10,2)==125,"exact125 stock deltas");invariant(c,400);
    require(c.stats().treasury==cash&&c.stats().year==year&&c.stats().month==month,"transfer has no money/time/resource grants");
    require(c.transferWarehouseStock(10,2,15,2,125,m)&&c.warehouseStockAt(10,2)==0&&c.warehouseStockAt(15,2)==125,"three-warehouse transfer empties exact source");
    reject(c,5,2,5,2,1);reject(c,5,2,10,2,0);reject(c,5,2,10,2,-1);reject(c,5,2,10,2,276);reject(c,5,2,10,2,std::numeric_limits<int64_t>::max());
    reject(c,128,2,10,2,1);reject(c,5,128,10,2,1);reject(c,5,2,128,2,1);reject(c,5,2,10,128,1);reject(c,0,0,10,2,1);reject(c,5,2,0,0,1);reject(c,10,2,15,2,1);
    for(auto r:{CityRuleset::LegacyServices,CityRuleset::StaffedServices}){CitySimulation old(r);reject(old,5,2,10,2,1);}
    auto full=city();seed(full,400,600,1);reject(full,5,2,10,2,1);seed(full,400,599,1);reject(full,5,2,10,2,2);require(full.transferWarehouseStock(5,2,10,2,1,m)&&full.warehouseStockAt(10,2)==600,"exact one remaining capacity accepted");invariant(full,1000);
    const auto beforeDemolition=c.saveSnapshot();const auto beforeEvents=c.events();require(!c.demolish(5,2,m)&&c.saveSnapshot()==beforeDemolition&&c.events()==beforeEvents,"stocked warehouse still rejects demolition");
    require(c.transferWarehouseStock(5,2,15,2,275,m)&&c.warehouseStockAt(5,2)==0,"full remaining source transfer");invariant(c,400);
    require(c.demolish(5,2,m)&&c.buildingAt(5,2)==BuildingType::None&&c.stats().treasury==cash-2,"empty warehouse demolishes for normal fee");invariant(c,400);
    auto ui=city();CityWarehouseTransferSelection selection;auto stable=ui.saveSnapshot();const auto events=ui.events();
    require(!selection.begin(ui,0,0,m)&&!selection.active(),"invalid source does not start selection");require(selection.begin(ui,5,2,m)&&selection.active()&&!selection.ready(),"explicit source selection");
    require(!selection.choose(ui,5,2,m)&&!selection.ready(),"same-warehouse target rejected");require(!selection.choose(ui,0,0,m)&&!selection.ready(),"nonwarehouse target rejected");
    require(selection.choose(ui,10,2,m)&&selection.maximum(ui)==400,"destination previews actual max");
    for(const auto* text:{"","0","-1","+1","1.5","2e2","100x"," 1","601","999999999999999999","401"}){
        require(!selection.commit(ui,text,m)&&selection.ready(),"invalid/overstock UI input rejects but remains editable");require(ui.saveSnapshot()==stable&&ui.events()==events,"UI error leaves city/events untouched");
    }
    selection.cancel();require(!selection.active()&&ui.saveSnapshot()==stable&&ui.events()==events,"cancel keeps exact city and events");
    require(selection.begin(ui,5,2,m)&&selection.choose(ui,10,2,m)&&selection.commit(ui,"100",m)&&!selection.active(),"confirmed UI commit alone moves stock");invariant(ui,400);
    require(selection.begin(ui,5,2,m)&&selection.choose(ui,15,2,m),"selection before other operation");require(ui.transferWarehouseStock(5,2,10,2,300,m),"intervening stock change");stable=ui.saveSnapshot();
    require(!selection.commit(ui,"1",m)&&ui.saveSnapshot()==stable,"confirm revalidates current source, no stale reservation");selection.cancel();
    // A live monthly ledger remains valid because transfer preserves aggregate grain.
    auto future=city();build(future,8,2,BuildingType::Farm);build(future,6,2,BuildingType::Road);build(future,7,2,BuildingType::Road);for(unsigned x=20;x<70;x+=10)build(future,x,20,BuildingType::House);
    for(unsigned n=0;n<5;++n)future.advanceMonth();const auto ledger=future.lastLogisticsMonth();require(ledger.valid,"real month produces historical ledger");
    const int total=future.stats().foodSupply;require(future.transferWarehouseStock(5,2,10,2,50,m),"transfer after real ledger");invariant(future,total);
    require(future.lastLogisticsMonth().endStored==ledger.endStored&&future.lastLogisticsMonth().consumed==ledger.consumed,"transfer preserves historical ledger");
    CitySimulation restored;require(restored.loadSnapshot(future.saveSnapshot())&&restored.saveSnapshot()==future.saveSnapshot(),"transferred stocks canonical v4 snapshot roundtrip");
    CityScenarioSession no;CityScenarioAwardBook book;require(saveCityGameState(future,no,book,dir/"transfer.emp",m),"transferred city full file save");
    require(loadCityGameState(restored,no,book,dir/"transfer.emp",m)&&restored.saveSnapshot()==future.saveSnapshot(),"fresh file load exact warehouse stocks");
    for(unsigned n=0;n<24;++n){if(n==3){require(future.transferWarehouseStock(10,2,15,2,10,m),"continuous future transfer");require(restored.transferWarehouseStock(10,2,15,2,10,m),"restored future transfer");}future.advanceMonth();restored.advanceMonth();require(future.saveSnapshot()==restored.saveSnapshot(),"same input future24 full state equality");}
    std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"inventory_conservation\":true,\"transient_confirmation\":true,\"future_months\":24,\"devices\":false}\n";
}
