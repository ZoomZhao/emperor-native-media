#include "city_game_state.hpp"
#include <fstream>
#include <iostream>
using namespace game_lab;
namespace {
int checks = 0;
void require(bool ok,const char* message) {
    ++checks; if (!ok) { std::cerr << "FAIL " << message << '\n'; std::exit(1); }
}
struct Run {
    CitySimulation city{CityRuleset::RoadWarehouses};
    CityScenarioSession session;
    CityScenarioAwardBook book;
};
std::vector<uint8_t> bytes(const Run& r) {
    std::string error;auto out=encodeCityGameState(r.city,r.session,r.book,error);
    require(!out.empty() && error.empty(),"complete training state encodes");return out;
}
void build(Run& run,uint32_t x,uint32_t y,BuildingType type) {
    std::string message;require(run.city.build(x,y,type,message),"own training route build");
}
void remove(Run& run,uint32_t x,uint32_t y) {
    std::string message;require(run.city.demolish(x,y,message),"own training route demolition");
}
void begin(Run& run,uint64_t id) {
    run.city.setSpeed(0);require(beginCityWarehouseTraining(run.session,run.city,id,run.book).ok,"explicit v4 training begin");
}
void route(Run& r) {
    for(unsigned n=0;n<8;++n) build(r,20+10*n,20,BuildingType::House);
    for(unsigned x=7;x<=10;++x) build(r,x,4,BuildingType::Farm);
    for(unsigned x=5;x<=10;++x) build(r,x,3,BuildingType::Road);
}
void write(const std::filesystem::path& file,const std::vector<uint8_t>& data) {
    std::ofstream out(file,std::ios::binary);out.write(reinterpret_cast<const char*>(data.data()),data.size());
    require(bool(out),"only own fixture bytes written");
}
void set32(std::vector<uint8_t>& b,size_t p,uint32_t n) {
    for(unsigned k=0;k<4;++k)b[p+k]=uint8_t(n>>(8*k));
}
void sign(std::vector<uint8_t>& b) {
    std::vector<uint8_t> signedBytes(b.begin(),b.begin()+20);
    signedBytes.insert(signedBytes.end(),b.begin()+kCityGameHeaderBytes,b.end());
    const auto h=city_game_io_detail::checksum(signedBytes);std::copy(h.begin(),h.end(),b.begin()+20);
}
void checkpoint(const Run& live,const std::filesystem::path& file) {
    const auto original=bytes(live);const auto events=live.city.events();std::string error;
    require(original.size()==kCityGameHeaderBytes+kCityWarehouseStateBytes+kCityScenarioCheckpointBytes+12+8*live.book.completedRuns.size(),"v2 exact complete size");
    require(original[4]==2,"explicit ECG1 version2");
    require(saveCityGameState(live.city,live.session,live.book,file,error),"real v4 training atomic file save");
    Run fresh;
    require(loadCityGameState(fresh.city,fresh.session,fresh.book,file,error),"new instance complete file load");
    require(bytes(fresh)==original,"city + metadata + receipt bytes identical");
    require(fresh.session.checkpoint().startMonth==live.session.checkpoint().startMonth &&
            fresh.session.checkpoint().runId==live.session.checkpoint().runId &&
            fresh.session.checkpoint().spec.deadlineMonths==live.session.checkpoint().spec.deadlineMonths,
            "original run, start and deadline preserved");
    require(live.city.events()==events,"save prevalidation never mutates source events");
    auto future=live;
    for(int n=0;n<2;++n) {
        future.city.advanceMonth();fresh.city.advanceMonth();
        auto a=observeCityWarehouseTraining(future.session,future.city,future.book);
        auto b=observeCityWarehouseTraining(fresh.session,fresh.city,fresh.book);
        require(a.ok && b.ok && bool(a.award)==bool(b.award),"fresh continuation outcomes and one-time receipt match");
        require(bytes(future)==bytes(fresh),"same future inputs produce whole identical city/session/book");
    }
}
}
int main(int argc,char** argv) {
    require(argc==2,"own fixture directory argument");const std::filesystem::path dir(argv[1]);
    std::filesystem::create_directories(dir);std::string error;Run live;begin(live,7001);route(live);
    require(live.city.stats().treasury==4690,"no training resource grants");
    int won=-1;std::vector<uint8_t> preWin,full,cut;
    for(int month=0;month<=27;++month) {
        const auto observed=observeCityWarehouseTraining(live.session,live.city,live.book);
        require(observed.ok,"observe captured training config");if(observed.becameWon) won=month;
        if(month==10) { require(live.city.lastLogisticsMonth().fullDiscard==5,"actual full loss5 at10");full=bytes(live); }
        if(month==11) { require(live.city.lastLogisticsMonth().noRouteDiscard==9 && live.city.stats().foodSupply==592,"actual cut loss9 and stored592");cut=bytes(live); }
        if(month==12)require(live.city.lastLogisticsMonth().delivered==8,"rejoin admits current8 with no backfill");
        if(month==18)preWin=bytes(live);
        checkpoint(live,dir/("training-"+std::to_string(month)+".emp"));
        if(month==10)remove(live,6,3);
        if(month==11) { build(live,6,3,BuildingType::Road);build(live,10,2,BuildingType::Warehouse); }
        if(month==19) { remove(live,5,3);remove(live,10,3); }
        if(month<27)live.city.advanceMonth();
    }
    require(won==19 && live.session.checkpoint().status==CityScenarioStatus::Won,"actual route wins19 and stays Won after regression");
    require(live.book.completedRuns==std::vector<uint64_t>{7001},"exactly one completion receipt");
    Run replay;replay.book=live.book;
    require(decodeCityGameState(preWin,replay.city,replay.session,replay.book,error),"prewin file merged with current profile history");
    for(int n=0;n<3;++n) { replay.city.advanceMonth();auto u=observeCityWarehouseTraining(replay.session,replay.city,replay.book);require(u.ok && !u.award,"replayed win does not reissue receipt"); }
    require(replay.book.completedRuns==live.book.completedRuns && replay.session.checkpoint().awardRecorded,"profile union and internal recorded marker");
    for(int kind=0;kind<3;++kind) {
        Run fail;begin(fail,8000+kind);
        if(kind) { route(fail); }
        if(kind==2) { build(fail,10,2,BuildingType::Warehouse);for(unsigned x=5;x<=10;++x)remove(fail,x,3); }
        for(int month=0;month<=25;++month) {
            require(observeCityWarehouseTraining(fail.session,fail.city,fail.book).ok,"failure path observation");
            if(month==23 || month==24 || month==25)checkpoint(fail,dir/("failure-"+std::to_string(kind)+"-"+std::to_string(month)+".emp"));
            if(month<25)fail.city.advanceMonth();
        }
        require(fail.session.checkpoint().status==CityScenarioStatus::Lost && fail.book.completedRuns.empty(),"each failure persists Lost and no award");
    }
    // Every malformed input is checked through memory and actual bounded file APIs.
    const auto before=bytes(live);const auto events=live.city.events();
    const auto reject=[&](const std::vector<uint8_t>& bad) {
        require(!decodeCityGameState(bad,live.city,live.session,live.book,error) && !error.empty(),"bad complete bytes rejected");
        require(bytes(live)==before && live.city.events()==events,"bad memory restore preserves all live objects");
        write(dir/"bad.emp",bad);
        require(!loadCityGameState(live.city,live.session,live.book,dir/"bad.emp",error),"bad bounded file rejected");
        require(bytes(live)==before && live.city.events()==events,"bad file restore preserves all live objects");
    };
    auto mixed=full;std::copy(cut.begin()+kCityGameHeaderBytes,cut.begin()+kCityGameHeaderBytes+kCityWarehouseStateBytes,mixed.begin()+kCityGameHeaderBytes);reject(mixed);
    mixed=full;mixed[kCityGameHeaderBytes+8]^=1;
    CitySimulation foreign,originalFull;
    require(originalFull.loadSnapshot({full.begin()+kCityGameHeaderBytes,full.begin()+kCityGameHeaderBytes+kCityWarehouseStateBytes}) &&
            foreign.loadSnapshot({mixed.begin()+kCityGameHeaderBytes,mixed.begin()+kCityGameHeaderBytes+kCityWarehouseStateBytes}) &&
            foreign.stats().year==originalFull.stats().year && foreign.stats().month==originalFull.stats().month,
            "mixed body is itself a valid same-date city as the source full checkpoint");
    reject(mixed); // Same-date valid foreign city body, digest not rebound.
    mixed=full;const size_t metadata=kCityGameHeaderBytes+kCityWarehouseStateBytes;
    mixed[metadata+16]^=1;reject(mixed); // Separate run metadata.
    mixed=before;mixed.back()^=1;reject(mixed); // Mixed receipt payload.
    for(auto pair : std::vector<std::pair<size_t,uint32_t>>{{4,0},{4,1},{4,3},{8,65580},{12,71},{16,11},
        {metadata+8,1},{metadata+8,5},{metadata+12,2},{metadata+24,0},{metadata+40,0},{metadata+52,0}}) {
        auto bad=full;set32(bad,pair.first,pair.second);sign(bad);reject(bad);
    }
    mixed=full;mixed[kCityGameHeaderBytes+4]=3;sign(mixed);reject(mixed);
    mixed=full;set32(mixed,kCityGameHeaderBytes+65580+4*(2*128+5),601);sign(mixed);reject(mixed);
    mixed=full;mixed.push_back(0);reject(mixed);mixed=full;mixed.pop_back();reject(mixed);
    reject(std::vector<uint8_t>(kCityGameMaxBytes+1,0));
    Run wrong;begin(wrong,9001);auto cp=wrong.session.checkpoint();cp.spec.revision=2;
    require(wrong.session.restore(cp,{wrong.city.stats().year,wrong.city.stats().month},wrong.book).ok,"generic metadata can carry replacement revision");
    require(encodeCityGameState(wrong.city,wrong.session,wrong.book,error).empty(),"unregistered training revision encode rejects");
    CitySimulation v3(CityRuleset::StaffedServices);
    require(encodeCityGameState(v3,live.session,live.book,error).empty(),"training metadata cannot pair with v3");
    Run custom;begin(custom,9002);cp=custom.session.checkpoint();cp.spec.goals={40,701,4200,1};cp.spec.deadlineMonths=37;
    require(custom.session.restore(cp,{custom.city.stats().year,custom.city.stats().month},custom.book).ok,"valid captured replacement config");
    checkpoint(custom,dir/"replacement.emp");
    auto customBytes=bytes(custom);Run resumed;
    require(decodeCityGameState(customBytes,resumed.city,resumed.session,resumed.book,error) &&
        resumed.session.checkpoint().spec.goals.foodReserve==701 && resumed.session.checkpoint().spec.deadlineMonths==37,
        "restore captured goals/deadline without catalog reset");
    CityScenarioSession free;CityScenarioAwardBook book;
    for(auto rule : {CityRuleset::LegacyServices,CityRuleset::StaffedServices,CityRuleset::RoadWarehouses}) {
        CitySimulation city(rule);const auto raw=city.saveSnapshot();CitySimulation loaded;CityScenarioSession session;
        require(encodeCityGameState(city,free,book,error)==raw && decodeCityGameState(raw,loaded,session,book,error) && !session.active() && loaded.saveSnapshot()==raw,"free old/v3/v4 EMP1 unchanged and challenge disabled");
    }
    require(!saveCityGameState(live.city,live.session,live.book,dir/"missing-parent/file.emp",error),"failed native save target does not mutate live state");
    require(bytes(live)==before,"write failure preserves full live state");
    std::cout << "{\"passed\":true,\"checks\":" << checks << ",\"training_win_month\":19,\"running_terminal_checkpoints\":37,\"legacy_contract\":\"v1/v3/v4 free unchanged; classic suite separate\"}\n";
}
