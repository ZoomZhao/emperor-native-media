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
void laborAndHarvestCases() {
    // 1. Zero workers: farm built, population == 0.
    // Food starts at 400. Zero workers must strictly produce 0 food in both regular and autumn months.
    {
        game_lab::CitySimulation city;
        std::string err;
        check(city.build(10, 10, game_lab::BuildingType::Farm, err), "build farm with 0 pop");
        check(city.stats().totalFarms == 1, "farm count is 1");
        check(city.stats().neededWorkers == 8, "1 farm needs 8 workers");
        check(city.stats().availableWorkers == 0, "0 population has 0 workers");
        check(city.stats().employmentPercent == 0, "employment percent is 0%");
        check(city.laborRatio() == 0.0, "labor ratio is 0.0");

        // Advance through regular months (month 1 -> 2 -> ... -> 7)
        for (int m = 1; m <= 7; ++m) {
            city.advanceMonth();
            check(city.stats().foodSupply == 400, "zero workers produce strictly 0 food in regular month");
            check(!city.events().empty() && city.events().back() == "农庄严重缺工，当月颗粒无收。",
                  "severe labor shortage event emitted in regular month");
        }

        // Advance through autumn months (month 8, 9, 10)
        // With zero workers, autumn 30担 harvest must also strictly be 0 (no 30% floor).
        for (int m = 8; m <= 10; ++m) {
            city.advanceMonth();
            check(city.stats().foodSupply == 400, "zero workers produce strictly 0 food in autumn month");
            check(!city.events().empty() && city.events().back() == "农庄严重缺工，当月颗粒无收。",
                  "severe labor shortage event emitted in autumn");
        }

        // Inspection on farm tile confirms severe shortage and 0 yield
        const std::string inspectStr = city.inspectTile(10, 10);
        check(inspectStr.find("农庄") != std::string::npos, "inspect farm mentions 农庄");
        check(inspectStr.find("需工 8 人") != std::string::npos, "inspect farm mentions 需工 8 人");
        check(inspectStr.find("全城岗位满足率: 0%") != std::string::npos, "inspect farm 0% employment");
        check(inspectStr.find("全城新粮: 0 担") != std::string::npos, "inspect farm 0 yield");
        check(inspectStr.find("严重缺工停产") != std::string::npos, "inspect farm reason 严重缺工停产");
    }

    // 2. Partial workers:
    // With 1 farm (needed = 8) and population = 8 (workers = 4): ratio = 4/8 = 0.5 (50%).
    {
        game_lab::CitySimulation city;
        std::string err;
        check(city.build(10, 10, game_lab::BuildingType::Road, err), "build road");
        check(city.build(10, 11, game_lab::BuildingType::House, err), "build house"); // capacity 8
        check(city.build(12, 12, game_lab::BuildingType::Farm, err), "build farm");  // needed 8
        auto snap = city.saveSnapshot();
        // Modify snapshot population to 8, month to 5 (regular month), food to 100
        set32(snap, 12, 8); // population = 8 -> availableWorkers = 4
        set32(snap, 20, 100); // foodSupply = 100
        set32(snap, 28, 5); // month = 5
        check(city.loadSnapshot(snap), "load snapshot with pop 8 at month 5");
        check(city.stats().population == 8, "population is 8");
        check(city.stats().availableWorkers == 4, "availableWorkers is 4");
        check(city.stats().neededWorkers == 8, "neededWorkers is 8");
        check(city.stats().employmentPercent == 50, "employment percent is 50%");
        check(city.laborRatio() == 0.5, "labor ratio is 0.5");

        // Inspect before advance: month 5 regular month
        const std::string ins5 = city.inspectTile(12, 12);
        check(ins5.find("全城新粮: 2 担") != std::string::npos, "partial worker expected yield is 2 (5 * 0.5)");
        check(ins5.find("缺工折减") != std::string::npos, "partial worker reason is 缺工折减");

        // Advance to month 6:
        // regularYield = int(1 * 5 * 0.5) = 2.
        // foodNeeded = (8 + 9) / 10 = 1.
        // foodSupply becomes 100 + 2 - 1 = 101.
        city.advanceMonth();
        check(city.stats().foodSupply == 101, "partial harvest scaled by 0.5 in regular month (100 + 2 - 1 = 101)");
        check(!city.events().empty() && city.events().back().find("新粮入仓 2 担") != std::string::npos,
              "regular partial harvest event reports 2 担");

        // Now test autumn month with partial workers:
        // Set month to 7 so advanceMonth() enters month 8 (autumn): regular 5 + autumn 30 = 35 base.
        // Scaled by 0.5: regular = int(5*0.5) = 2, autumn = int(30*0.5) = 15. Total = 17.
        snap = city.saveSnapshot();
        set32(snap, 12, 8); // pop 8
        set32(snap, 20, 100); // food 100
        set32(snap, 28, 7); // month 7 (will advance to month 8)
        check(city.loadSnapshot(snap), "load snapshot pop 8 at month 7");
        city.advanceMonth(); // advances to month 8
        check(city.stats().month == 8, "month is 8");
        // harvest = int(5 * 0.5) + int(30 * 0.5) = 2 + 15 = 17.
        // foodNeeded = (8 + 9) / 10 = 1.
        // food = 100 + 17 - 1 = 116.
        check(city.stats().foodSupply == 116, "autumn partial harvest scaled by 0.5 (100 + 17 - 1 = 116)");
        check(!city.events().empty() && city.events().back().find("新粮入仓 17 担") != std::string::npos,
              "autumn partial harvest event reports 17 担");

        // Inspect in autumn with partial workers
        const std::string ins8 = city.inspectTile(12, 12);
        check(ins8.find("全城新粮: 17 担") != std::string::npos, "autumn partial expected yield is 17");
        check(ins8.find("缺工折减") != std::string::npos, "autumn partial reason is 缺工折减");
    }

    // 3. Full workers:
    // 1 farm (needed 8). Population = 16 (availableWorkers = 8) -> 100% employment.
    {
        game_lab::CitySimulation city;
        std::string err;
        check(city.build(10, 10, game_lab::BuildingType::Road, err), "build road");
        check(city.build(10, 11, game_lab::BuildingType::House, err), "build house 1");
        check(city.build(10, 9, game_lab::BuildingType::House, err), "build house 2");
        check(city.build(12, 12, game_lab::BuildingType::Farm, err), "build farm");
        auto snap = city.saveSnapshot();
        set32(snap, 12, 16); // pop = 16 -> workers = 8
        set32(snap, 20, 200); // food = 200
        set32(snap, 28, 3); // month = 3
        check(city.loadSnapshot(snap), "load full employment snapshot");
        check(city.stats().employmentPercent == 100, "employment is 100%");
        check(city.laborRatio() == 1.0, "labor ratio is 1.0");

        // Inspect in regular month: 5 担, 满员满产
        const std::string ins3 = city.inspectTile(12, 12);
        check(ins3.find("全城新粮: 5 担") != std::string::npos, "full employment regular yield is 5");
        check(ins3.find("满员满产") != std::string::npos, "full employment reason is 满员满产");

        // Advance month 3 -> 4:
        // regularYield = 5. foodNeeded = (16+9)/10 = 2.
        // foodSupply = 200 + 5 - 2 = 203.
        city.advanceMonth();
        check(city.stats().foodSupply == 203, "full employment regular food (200 + 5 - 2 = 203)");
        check(!city.events().empty() && city.events().back().find("新粮入仓 5 担") != std::string::npos,
              "full employment regular event reports 5 担");

        // Set to month 7, advance to month 8 (autumn):
        snap = city.saveSnapshot();
        set32(snap, 12, 16);
        set32(snap, 20, 200);
        set32(snap, 28, 7);
        check(city.loadSnapshot(snap), "load full employment month 7");
        city.advanceMonth(); // month 8
        check(city.stats().month == 8, "advanced to month 8");
        // Harvest = 5 + 30 = 35. foodNeeded = 2. food = 200 + 35 - 2 = 233.
        check(city.stats().foodSupply == 233, "full employment autumn food (200 + 35 - 2 = 233)");
        check(!city.events().empty() && city.events().back().find("新粮入仓 35 担") != std::string::npos,
              "full employment autumn event reports 35 担");

        // Inspect in autumn: 35 担, 秋收丰收季
        const std::string ins8 = city.inspectTile(12, 12);
        check(ins8.find("全城新粮: 35 担") != std::string::npos, "full employment autumn yield is 35");
        check(ins8.find("秋收丰收季") != std::string::npos, "autumn full employment mentions 秋收丰收季");
    }

    // 4. Labor competition across multiple building types:
    // Farm (8), Well (2), Temple (4), Market (4), Watchtower (2) -> total = 20 needed workers.
    {
        game_lab::CitySimulation city;
        std::string err;
        check(city.build(10, 10, game_lab::BuildingType::Farm, err), "build farm (8)");
        check(city.build(10, 12, game_lab::BuildingType::Well, err), "build well (2)");
        check(city.build(10, 14, game_lab::BuildingType::Temple, err), "build temple (4)");
        check(city.build(10, 16, game_lab::BuildingType::Market, err), "build market (4)");
        check(city.build(10, 18, game_lab::BuildingType::Watchtower, err), "build watchtower (2)");
        check(city.stats().neededWorkers == 20, "20 needed workers across all 5 buildings");

        // Failed build does not alter labor stats
        const auto beforeFail = city.saveSnapshot();
        check(!city.build(10, 10, game_lab::BuildingType::Road, err), "cannot build on occupied tile");
        check(city.stats().neededWorkers == 20, "neededWorkers unaltered after failed build");
        check(city.saveSnapshot() == beforeFail, "state strictly unchanged after failed build");
    }
}

void inspectionCases() {
    game_lab::CityMap map;
    map.width = map.height = 128;
    map.tiles.assign(128 * 128, 0);
    map.tiles[5 * 128 + 5] = 210; // water tile

    game_lab::CitySimulation city;
    std::string err;
    check(city.loadFromMap(map), "load map with water tile");

    // Static houseCapacityForLevel checks
    check(game_lab::CitySimulation::houseCapacityForLevel(4) == 48, "house level 4 capacity 48");
    check(game_lab::CitySimulation::houseCapacityForLevel(3) == 32, "house level 3 capacity 32");
    check(game_lab::CitySimulation::houseCapacityForLevel(2) == 20, "house level 2 capacity 20");
    check(game_lab::CitySimulation::houseCapacityForLevel(1, true) == 8, "house level 1 near road capacity 8");
    check(game_lab::CitySimulation::houseCapacityForLevel(1, false) == 4, "house level 1 isolated capacity 4");

    // Out of bounds
    check(city.inspectTile(128, 0) == "坐标超出疆界", "out of bounds X");
    check(city.inspectTile(0, 128) == "坐标超出疆界", "out of bounds Y");
    check(city.inspectTile(999, 999) == "坐标超出疆界", "extreme out of bounds");

    // Water tile
    const std::string waterIns = city.inspectTile(5, 5);
    check(waterIns.find("河湖水域") != std::string::npos, "water tile mentions 河湖水域");
    check(waterIns.find("天然水泽阻隔") != std::string::npos, "water tile mentions 天然水泽阻隔");

    // Empty land
    const std::string emptyIns = city.inspectTile(0, 0);
    check(emptyIns.find("陆地平原") != std::string::npos, "empty tile mentions 陆地平原");
    check(emptyIns.find("天然旷野") != std::string::npos, "empty tile mentions 天然旷野");

    // Road
    check(city.build(1, 1, game_lab::BuildingType::Road, err), "build road");
    const std::string roadIns = city.inspectTile(1, 1);
    check(roadIns.find("道路") != std::string::npos, "road mentions 道路");
    check(roadIns.find("需工 0 人") != std::string::npos, "road needs 0 workers");

    // House isolated (level 1, capacity 4, 闭塞小屋)
    check(city.build(20, 20, game_lab::BuildingType::House, err), "build isolated house");
    const std::string hIso = city.inspectTile(20, 20);
    check(hIso.find("闭塞小屋") != std::string::npos, "isolated house is 闭塞小屋");
    check(hIso.find("容量: 4 人") != std::string::npos, "isolated house capacity is 4");
    check(city.houseCapacityAt(20, 20) == 4, "houseCapacityAt isolated is 4");

    // House near road (level 1, capacity 8, 郊野茅舍)
    check(city.build(1, 2, game_lab::BuildingType::House, err), "build house near road");
    const std::string hRoad = city.inspectTile(1, 2);
    check(hRoad.find("郊野茅舍") != std::string::npos, "house near road is 郊野茅舍");
    check(hRoad.find("容量: 8 人") != std::string::npos, "house near road capacity is 8");
    check(city.houseCapacityAt(1, 2) == 8, "houseCapacityAt near road is 8");

    // Well
    check(city.build(3, 3, game_lab::BuildingType::Well, err), "build well");
    const std::string wellIns = city.inspectTile(3, 3);
    check(wellIns.find("水井") != std::string::npos, "well mentions 水井");
    check(wellIns.find("需工 2 人") != std::string::npos, "well needs 2 workers");

    // Temple
    check(city.build(3, 5, game_lab::BuildingType::Temple, err), "build temple");
    const std::string templeIns = city.inspectTile(3, 5);
    check(templeIns.find("宗庙") != std::string::npos, "temple mentions 宗庙");
    check(templeIns.find("需工 4 人") != std::string::npos, "temple needs 4 workers");

    // Market
    check(city.build(3, 7, game_lab::BuildingType::Market, err), "build market");
    const std::string marketIns = city.inspectTile(3, 7);
    check(marketIns.find("市井粮肆") != std::string::npos, "market mentions 市井粮肆");
    check(marketIns.find("需工 4 人") != std::string::npos, "market needs 4 workers");

    // Watchtower
    check(city.build(3, 9, game_lab::BuildingType::Watchtower, err), "build watchtower");
    const std::string towerIns = city.inspectTile(3, 9);
    check(towerIns.find("更楼") != std::string::npos, "watchtower mentions 更楼");
    check(towerIns.find("需工 2 人") != std::string::npos, "watchtower needs 2 workers");
}
}
int main(int argc,char** argv) {
    try {
        knownCases();snapshotCases();
        laborAndHarvestCases();
        inspectionCases();
        if(argc>1)fileCases(std::filesystem::path(argv[1]));
        extremeBudget();
        std::printf("{\"passed\":%s,\"checks\":%zu,\"self_authored_city\":true,\"UI\":false,\"audio_devices\":false,\"failures\":[",failures.empty()?"true":"false",checks);
        for(size_t i=0;i<failures.size();++i)std::printf("%s\"%s\"",i?",":"",failures[i].c_str());
        std::puts("]}");return failures.empty()?0:1;
    }catch(const std::exception& error){std::fprintf(stderr,"city test error: %s\n",error.what());return 2;}
}
