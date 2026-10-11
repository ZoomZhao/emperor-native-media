#include "city_state_io.hpp"
#include "../scripts/self_fixture_evidence.hpp"
#include <cstdio>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace {
std::vector<std::string> failures;
size_t checks = 0;
void check(bool value, const char* name) { ++checks; if (!value) failures.emplace_back(name); }
void set32(std::vector<uint8_t>& bytes, size_t offset, int64_t value) {
    const uint32_t word = static_cast<uint32_t>(value);
    for (unsigned i=0;i<4;++i) bytes.at(offset+i)=static_cast<uint8_t>(word>>(8*i));
}
game_lab::CitySimulation populated() {
    game_lab::CitySimulation city; std::string error;
    check(city.build(10,10,game_lab::BuildingType::Road,error),"road accepted");
    check(city.build(10,11,game_lab::BuildingType::House,error),"house accepted");
    check(city.build(11,11,game_lab::BuildingType::Farm,error),"farm accepted");
    city.advanceMonth(); city.setSpeed(3); city.tick();
    return city;
}
void rejects(const game_lab::CitySimulation& original, const std::vector<uint8_t>& bytes, const char* name) {
    auto target=original; const auto before=target.saveSnapshot(); const auto events=target.events();
    self_fixture::bytes(name,bytes);
    check(!target.loadSnapshot(bytes),name);
    check(target.saveSnapshot()==before && target.events()==events,"rejected snapshot preserves state and events");
}
void knownCases() {
    game_lab::CitySimulation city; std::string error;
    for (auto type : {game_lab::BuildingType::None,static_cast<game_lab::BuildingType>(255)}) {
        const auto before=city.saveSnapshot();const auto events=city.events();
        check(!city.canBuild(2,2,type) && !city.build(2,2,type,error),"None and invalid build rejected");
        check(city.saveSnapshot()==before && city.events()==events && !error.empty(),"invalid build immutable with reason");
    }
    for(int i=0;i<9;++i)city.tick(); city.reset(); city.tick();
    check(city.stats().month==1 && city.monthAccumulator()==1,"reset clears unfinished tick phase");
    city.reset();city.setSpeed(3);for(int i=0;i<6;++i)city.tick();
    check(city.stats().month==2 && city.monthAccumulator()==8,"3x retains remainder after month");
    city.tick();check(city.stats().month==3 && city.monthAccumulator()==1,"3x remainder crosses next month correctly");
    game_lab::CityMap map;map.width=map.height=128;map.tiles.assign(128*128,0);map.tiles[13*128+13]=210;
    city.loadFromMap(map);const auto clean=city.saveSnapshot();
    check(!city.build(13,13,game_lab::BuildingType::House,error) && city.saveSnapshot()==clean,"self-authored water rule enforced");
    check(city.build(2,2,game_lab::BuildingType::Road,error),"build on valid map");
    const auto beforeBadMap=city.saveSnapshot();const auto events=city.events();city.loadFromMap({});
    check(city.saveSnapshot()==beforeBadMap && city.events()==events,"invalid map cannot clear existing city");
    int x=-77,y=-88;const double huge=std::numeric_limits<double>::max();
    check(game_lab::mapPointToTile(huge/2,huge/2,huge,huge,128,x,y)&&x==64&&y==64,"finite coordinate extremes stay bounded");
    for(double bad:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()}){
        x=-77;y=-88;check(!game_lab::mapPointToTile(bad,1,560,560,128,x,y)&&x==-77&&y==-88,"nonfinite point preserves outputs");
        x=-77;y=-88;check(!game_lab::mapPointToTile(1,1,bad,560,128,x,y)&&x==-77&&y==-88,"nonfinite view preserves outputs");
    }
    x=-77;y=-88;check(!game_lab::mapPointToTile(560,1,560,560,128,x,y)&&x==-77&&y==-88,"outside edge preserves outputs");
}
void snapshotCases() {
    const auto original=populated();const auto snapshot=original.saveSnapshot();
    self_fixture::bytes("valid-city-snapshot",snapshot);
    check(snapshot.size()==game_lab::kCityStateBytes,"EMP1 fixed complete byte layout");
    game_lab::CitySimulation restored;
    check(restored.loadSnapshot(snapshot)&&restored.saveSnapshot()==snapshot,"full canonical city round trip including tick phase");
    for(size_t length:{size_t(0),size_t(7),size_t(43),size_t(44),snapshot.size()-1})
        rejects(original,std::vector<uint8_t>(snapshot.begin(),snapshot.begin()+length),"truncated snapshot rejected");
    auto bad=snapshot;bad.push_back(0);rejects(original,bad,"trailing snapshot rejected");
    bad=snapshot;bad[0]='X';rejects(original,bad,"bad magic rejected");
    bad=snapshot;set32(bad,4,0xffffffffu);rejects(original,bad,"unsupported high-bit version rejected");
    struct Invalid {size_t offset;int64_t value;};
    for(const auto item:std::vector<Invalid>{{12,-1},{12,2147483647},{16,-1},{16,1},{20,-1},{28,0},{28,13},{32,-1},{32,4},{36,-1},{36,101},{40,-1},{40,10}}){
        bad=snapshot;set32(bad,item.offset,item.value);rejects(original,bad,"invalid scalar field rejected");
    }
    const size_t buildings=44+game_lab::CitySimulation::kTotalTiles*2;
    bad=snapshot;bad[buildings]=255;rejects(original,bad,"unknown building cannot be silently discarded");
    bad=snapshot;bad[buildings+game_lab::CitySimulation::kTotalTiles]=255;rejects(original,bad,"invalid empty tile level rejected");
    bad=snapshot;bad[buildings+game_lab::CitySimulation::kTotalTiles+11*128+10]=4;
    rejects(original,bad,"housing level inconsistent with neighbors rejected");
}
void extremeBudget() {
    auto city=populated();auto bytes=city.saveSnapshot();
    set32(bytes,8,std::numeric_limits<int32_t>::max());set32(bytes,20,std::numeric_limits<int32_t>::max());
    set32(bytes,24,std::numeric_limits<int32_t>::max());set32(bytes,28,12);
    self_fixture::bytes("max-int-city-budget",bytes);
    check(city.loadSnapshot(bytes),"complete extreme signed budget state accepted");city.advanceMonth();
    check(city.stats().year==std::numeric_limits<int32_t>::max(),"year addition does not overflow");
    check(city.stats().treasury==std::numeric_limits<int32_t>::max(),"tax addition bounded without signed overflow");
    check(city.stats().foodSupply>=0,"harvest addition cannot wrap food negative");
}
void fileCases(const std::filesystem::path& root) {
    std::filesystem::create_directories(root);const auto file=root/"city.emp";auto city=populated();std::string error;
    const auto snapshot=city.saveSnapshot();
    check(game_lab::saveCityState(city,file,error)&&error.empty(),"atomic save uses complete current state");
    game_lab::CitySimulation restored;check(game_lab::loadCityState(restored,file,error)&&restored.saveSnapshot()==snapshot,"shared App file consumer round trip");
    const auto before=restored.saveSnapshot();
    for(const auto& path:std::vector<std::filesystem::path>{std::filesystem::path{},root/"missing.emp",root})
        check(!game_lab::loadCityState(restored,path,error)&&!error.empty()&&restored.saveSnapshot()==before,"file errors preserve city with reason");
    auto bad=snapshot;set32(bad,28,13);self_fixture::bytes("bad-city-file",bad);
    {std::ofstream out(root/"bad.emp",std::ios::binary);out.write(reinterpret_cast<const char*>(bad.data()),bad.size());}
    check(!game_lab::loadCityState(restored,root/"bad.emp",error)&&restored.saveSnapshot()==before,"malformed file preserves current city");
    {std::ofstream out(root/"short.emp",std::ios::binary);out.put('E');}
    check(!game_lab::loadCityState(restored,root/"short.emp",error)&&restored.saveSnapshot()==before,"short file rejected before parsing");
    std::filesystem::create_symlink(file,root/"alias.emp");
    check(!game_lab::loadCityState(restored,root/"alias.emp",error)&&restored.saveSnapshot()==before,"save symlink rejected");
    check(!game_lab::saveCityState(city,root,error)&&!error.empty(),"save failure reports reason");
    check(game_lab::loadCityState(restored,file,error)&&restored.saveSnapshot()==snapshot,"failed save leaves existing file valid");
}
}
int main(int argc,char** argv) {
    try {
        knownCases();snapshotCases();
        if(argc>1)fileCases(std::filesystem::path(argv[1]));
        extremeBudget();
        std::printf("{\"passed\":%s,\"checks\":%zu,\"self_authored_city\":true,\"UI\":false,\"audio_devices\":false,\"failures\":[",failures.empty()?"true":"false",checks);
        for(size_t i=0;i<failures.size();++i)std::printf("%s\"%s\"",i?",":"",failures[i].c_str());
        std::puts("]}");return failures.empty()?0:1;
    }catch(const std::exception& error){std::fprintf(stderr,"city test error: %s\n",error.what());return 2;}
}
