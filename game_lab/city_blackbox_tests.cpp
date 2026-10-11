#include "city_state_io.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <type_traits>

// Independent tests of the self-authored model only. All rules below are cited in
// docs/CITY_BLACKBOX_TESTS.md; no original-game rules or assets are used.
namespace {
using game_lab::BuildingType;
using game_lab::CitySimulation;
using game_lab::CityStats;
enum class Profile { Baseline, StrictLabor, Invariants };
Profile profile = Profile::Baseline;
std::filesystem::path output;
std::vector<uint8_t> beforeEvidence, afterEvidence;
std::string context;
int failed = 0;
struct Skipped : std::runtime_error { using std::runtime_error::runtime_error; };

void require(bool yes, const std::string& reason) {
    if (!yes) throw std::runtime_error(reason + (context.empty() ? "" : " | " + context));
}
void captureBefore(const CitySimulation& city) { beforeEvidence = city.saveSnapshot(); }
void captureAfter(const CitySimulation& city) { afterEvidence = city.saveSnapshot(); }
void writeBytes(const std::filesystem::path& path, const std::vector<uint8_t>& bytes) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    require(bool(file), "cannot write test evidence");
}
void set32(std::vector<uint8_t>& bytes, size_t offset, int value) {
    const auto word = static_cast<uint32_t>(value);
    for (unsigned i = 0; i != 4; ++i) bytes.at(offset + i) = static_cast<uint8_t>(word >> (8 * i));
}
// EMP1 public wire contract: treasury=8, population=12, food=20, month=28.
// This supplies independently authored valid inputs without mutating internals.
void seed(CitySimulation& city, int population, int food, int month, int treasury = 4000) {
    auto bytes = city.saveSnapshot();
    set32(bytes, 8, treasury); set32(bytes, 12, population);
    set32(bytes, 20, food); set32(bytes, 28, month);
    require(city.loadSnapshot(bytes), "self-authored EMP1 fixture must be accepted");
    require(city.saveSnapshot() == bytes, "fixture must remain canonical");
}
void build(CitySimulation& city, unsigned x, unsigned y, BuildingType type) {
    std::string message;
    require(city.build(x, y, type, message), "setup build failed: " + message);
}
void demolish(CitySimulation& city, unsigned x, unsigned y) {
    std::string message;
    require(city.demolish(x, y, message), "setup demolition failed: " + message);
}
game_lab::CityMap syntheticMap() {
    game_lab::CityMap map;
    map.width = map.height = 128;
    map.tiles.assign(128 * 128, 10);
    for (unsigned y = 0; y < 128; ++y)
        for (unsigned x = 64; x < 68; ++x) map.tiles[y * 128 + x] = 220;
    return map;
}
CitySimulation fullyServicedHouse() {
    CitySimulation city;
    build(city, 10, 10, BuildingType::House);
    build(city, 11, 11, BuildingType::Road);
    build(city, 13, 13, BuildingType::Well);
    build(city, 14, 14, BuildingType::Market);
    build(city, 15, 15, BuildingType::Temple);
    return city;
}
void invariants(const CitySimulation& city) {
    const auto& s = city.stats();
    require(s.foodSupply >= 0, "food cannot be negative");
    require(s.population >= 0 && s.population <= s.housingCapacity, "population must fit capacity");
    require(s.housingCapacity >= 0, "housing capacity cannot be negative");
    require(s.month >= 1 && s.month <= 12 && s.speed >= 0 && s.speed <= 3, "clock bounds");
    require(s.favor >= 0 && s.favor <= 100, "favor bounds");
    require(city.monthAccumulator() >= 0 && city.monthAccumulator() < CitySimulation::kTicksPerMonth, "tick phase bounds");
    std::array<int, 8> counts{};
    for (unsigned y = 0; y < 128; ++y) for (unsigned x = 0; x < 128; ++x) {
        const auto type = city.buildingAt(x, y);
        const auto value = static_cast<unsigned>(type);
        require(value < counts.size(), "invalid building enum");
        ++counts[value];
        require(type == BuildingType::None || !city.isWaterTile(x, y), "building on water");
    }
    const std::array<int, 7> reported{{s.totalRoads, s.totalHouses, s.totalFarms, s.totalWells,
                                    s.totalTemples, s.totalMarkets, s.totalWatchtowers}};
    for (size_t i = 0; i < reported.size(); ++i) require(reported[i] == counts[i + 1], "building totals differ from public grid");
    require(s.availableWorkers == s.population / 2, "workers must equal floor(population/2)");
    if (profile != Profile::Invariants) {
        const int demand = counts[3] * 8 + counts[4] * 2 + counts[5] * 4 + counts[6] * 4 + counts[7] * 2;
        require(s.neededWorkers == demand, "labor demand ledger mismatch");
        const int fulfilled = demand ? std::min(100, s.availableWorkers * 100 / demand) : 100;
        require(s.employmentPercent == fulfilled, "reported job fulfillment mismatch");
    }
    CitySimulation restored;
    require(restored.loadSnapshot(city.saveSnapshot()), "every reachable state must load in a fresh instance");
    require(restored.saveSnapshot() == city.saveSnapshot(), "reachable-state canonical round trip");
}
int upkeep(const CityStats& s) {
    return s.totalFarms + s.totalWells + 3 * s.totalTemples + 2 * s.totalMarkets + s.totalWatchtowers;
}
int harvest(const CityStats& s, int nextMonth) {
    const bool autumn = nextMonth >= 8 && nextMonth <= 10;
    if (profile == Profile::StrictLabor) {
        const int used = std::min(s.availableWorkers, s.neededWorkers);
        // Integer rational oracle established before the frozen core's integer
        // implementation, independent of farmYieldForMonth and UI formatting.
        const int regular = s.neededWorkers ? s.totalFarms * 5 * used / s.neededWorkers : s.totalFarms * 5;
        const int seasonal = !autumn ? 0 : (s.neededWorkers ? s.totalFarms * 30 * used / s.neededWorkers : s.totalFarms * 30);
        return regular + seasonal;
    }
    const int percent = std::max(30, s.employmentPercent);
    return s.totalFarms * 5 + (autumn ? s.totalFarms * 30 * percent / 100 : 0);
}
void ledgerMonth(CitySimulation& city) {
    const auto s = city.stats();
    captureBefore(city);
    const int month = s.month == 12 ? 1 : s.month + 1;
    const int grown = harvest(s, month);
    const int consumed = (s.population + 9) / 10;
    const bool shortage = s.foodSupply + grown < consumed;
    const int food = std::max(0, s.foodSupply + grown - consumed);
    int people = shortage ? std::max(0, s.population - 4) : s.population;
    if (food > 10) people += std::min(6, s.housingCapacity - people);
    city.advanceMonth(); captureAfter(city);
    require(city.stats().foodSupply == food, "monthly food ledger expected=" + std::to_string(food) + " actual=" + std::to_string(city.stats().foodSupply));
    require(city.stats().population == people, "starvation/migration ordering mismatch");
    require(city.stats().treasury == s.treasury + people - upkeep(s), "tax and maintenance ledger mismatch");
    invariants(city);
}
void test(const char* id, const char* source, const std::function<void()>& run) {
    beforeEvidence.clear(); afterEvidence.clear(); context.clear();
    try {
        run(); std::cout << "PASS\t" << id << '\t' << source << "\tcompleted\n";
    } catch (const Skipped& error) {
        std::cout << "SKIP\t" << id << '\t' << source << '\t' << error.what() << '\n';
    } catch (const std::exception& error) {
        ++failed;
        if (!beforeEvidence.empty()) writeBytes(output / (std::string(id) + ".before.emp"), beforeEvidence);
        if (!afterEvidence.empty()) writeBytes(output / (std::string(id) + ".after.emp"), afterEvidence);
        std::cout << "FAIL\t" << id << '\t' << source << '\t' << error.what() << '\n';
    }
}
void note(const char* id, const std::string& reason) {
    std::cout << "UNIMPLEMENTED\t" << id << "\tno established contract\t" << reason << '\n';
}
uint32_t randomWord(uint32_t& value) {
    value = value * 1664525u + 1013904223u;
    return value;
}
void properties() {
    std::ofstream trace(output / "property-inputs.tsv");
    require(bool(trace), "cannot open property input trace");
    trace << "seed\tstep\tkind\tx\ty\ttype\n";
    for (uint32_t initial : {1u, 0x5eedu, 0xc0ffeeu, 0xdeadbeefu}) {
        CitySimulation city; require(city.loadFromMap(syntheticMap()), "property map");
        uint32_t rng = initial;
        for (unsigned step = 0; step < 400; ++step) {
            // Use high bits: low LCG bits would cycle through only two actions
            // when exactly four words are drawn per step.
            const unsigned kind = (randomWord(rng) >> 16) % 8;
            const unsigned x = (randomWord(rng) >> 16) % 134, y = (randomWord(rng) >> 16) % 134;
            const auto type = static_cast<BuildingType>((randomWord(rng) >> 16) % 10);
            trace << initial << '\t' << step << '\t' << kind << '\t' << x << '\t' << y << '\t' << static_cast<unsigned>(type) << '\n';
            context = "seed=" + std::to_string(initial) + " step=" + std::to_string(step) +
                " kind=" + std::to_string(kind) + " x=" + std::to_string(x) + " y=" + std::to_string(y) + " type=" + std::to_string(static_cast<unsigned>(type));
            captureBefore(city); const auto bytes = city.saveSnapshot(); const auto events = city.events();
            const int money = city.stats().treasury; std::string message;
            if (kind < 3) {
                const bool allowed = city.canBuild(x, y, type);
                const bool accepted = city.build(x, y, type, message);
                captureAfter(city);
                require(accepted == allowed, "canBuild disagrees with build");
                if (!accepted) require(city.saveSnapshot() == bytes && city.events() == events, "rejected build mutated state");
                else require(city.stats().treasury == money - game_lab::BuildingCost::cost(type), "build charge mismatch");
            } else if (kind == 3) {
                const bool accepted = city.demolish(x, y, message); captureAfter(city);
                if (!accepted) require(city.saveSnapshot() == bytes && city.events() == events, "rejected demolition mutated state");
                else if (profile != Profile::Invariants) require(city.stats().treasury == money - 2, "demolition charge mismatch");
            } else if (kind == 4) city.setSpeed(static_cast<int>(x % 9) - 2);
            else if (kind == 5) city.tick();
            else if (kind == 6) city.advanceMonth();
            else { CitySimulation fresh; require(fresh.loadSnapshot(bytes), "property fresh instance load"); city = fresh; }
            captureAfter(city); invariants(city);
        }
    }
    trace.close(); require(bool(trace), "property input trace write failed");
}
template <class T, class = void> struct HasInspect : std::false_type {};
template <class T> struct HasInspect<T, std::void_t<decltype(std::declval<const T&>().inspectTile(0u, 0u))>> : std::true_type {};
CitySimulation threeFarmSettlement() {
    CitySimulation city;
    build(city, 10, 10, BuildingType::House); build(city, 12, 10, BuildingType::House);
    build(city, 11, 11, BuildingType::Road); build(city, 11, 12, BuildingType::Well);
    build(city, 13, 12, BuildingType::Market);
    for (unsigned x = 20; x <= 22; ++x) build(city, x, 20, BuildingType::Farm);
    require(city.stats().neededWorkers == 30 && city.stats().housingCapacity == 64, "three-farm fixture demand/capacity");
    return city;
}
template <class T> void forecastContract() {
    if constexpr (!HasInspect<T>::value) {
        throw Skipped("inspectTile absent from committed baseline; no diagnostic contract to test");
    } else {
        if (profile != Profile::StrictLabor) throw Skipped("forecast oracle requires reviewed strict-labor implementation");
        std::ofstream trace(output / "city-forecast-cases.tsv");
        require(bool(trace), "forecast evidence output");
        trace << "farms\tpopulation\tworkers\tdemand\tprior_month\tnext_month\tforecast\tactual_harvest\n";
        for (int farms : {2, 3}) for (int month = 1; month <= 12; ++month) {
            T city;
            if (farms == 3) city = threeFarmSettlement();
            else {
                build(city, 10, 10, BuildingType::House); build(city, 10, 11, BuildingType::Road);
                build(city, 20, 20, BuildingType::Farm); build(city, 21, 20, BuildingType::Farm);
            }
            seed(city, farms == 3 ? 42 : 6, 100, month); captureBefore(city);
            const int nextMonth = month == 12 ? 1 : month + 1;
            const int expected = harvest(city.stats(), nextMonth);
            const int consumed = (city.stats().population + 9) / 10;
            context = "farms=" + std::to_string(farms) + " workers=" + std::to_string(city.stats().availableWorkers) +
                " demand=" + std::to_string(city.stats().neededWorkers) + " priorMonth=" + std::to_string(month);
            const std::string label = "预计" + std::to_string(nextMonth) + "月全城新粮: ";
            for (int f = 0; f < farms; ++f) {
                const auto text = city.inspectTile(static_cast<unsigned>(20 + f), 20);
                const auto pos = text.find(label);
                require(pos != std::string::npos, "inspection must state next month and city-wide scope: " + text);
                const int displayed = std::stoi(text.substr(pos + label.size()));
                require(displayed == expected, "each farm must display city forecast expected=" + std::to_string(expected) +
                    " displayed=" + std::to_string(displayed));
            }
            require(city.farmYieldForMonth(nextMonth) == expected, "public yield API disagrees with independent integer oracle");
            writeBytes(output / ("forecast-" + std::to_string(farms) + "farms-month" + std::to_string(month) + ".emp"), city.saveSnapshot());
            const int food = city.stats().foodSupply;
            const auto prior = city.stats(); city.advanceMonth(); captureAfter(city);
            const int actual = city.stats().foodSupply - food + consumed;
            trace << farms << '\t' << prior.population << '\t' << prior.availableWorkers << '\t' << prior.neededWorkers << '\t'
                  << month << '\t' << nextMonth << '\t' << expected << '\t' << actual << '\n';
            require(actual == expected, "actual city harvest differs from forecast/oracle expected=" + std::to_string(expected) + " actual=" + std::to_string(actual));
            invariants(city);
        }
        trace.close(); require(bool(trace), "forecast trace write failed");
    }
}
void gaps() {
    CitySimulation city; build(city, 10, 10, BuildingType::House); build(city, 11, 11, BuildingType::Road);
    build(city, 13, 13, BuildingType::Well); build(city, 14, 14, BuildingType::Market); build(city, 15, 15, BuildingType::Temple);
    if (city.stats().population == 0 && city.stats().housingCapacity == 48)
        note("staffed-services", "0 workers still provide well/market/temple housing capacity=48; per-building staff allocation has no public contract");
    CitySimulation farm; require(farm.loadFromMap(syntheticMap()), "gap map");
    build(farm, 10, 10, BuildingType::House); build(farm, 10, 11, BuildingType::Road);
    build(farm, 100, 100, BuildingType::Farm); seed(farm, 8, 100, 2);
    const int start = farm.stats().foodSupply; farm.advanceMonth();
    const int produced = farm.stats().foodSupply - start + 1;
    if (produced > 0) note("road-connected-production", "farm across full-map river without roads produces=" + std::to_string(produced) + "; graph connectivity/logistics not established");
    CitySimulation emptyFarm; build(emptyFarm, 1, 1, BuildingType::Farm);
    const int oldFood = emptyFarm.stats().foodSupply; emptyFarm.advanceMonth();
    if (emptyFarm.stats().foodSupply > oldFood) note("zero-worker-production", "0 workers produce=" + std::to_string(emptyFarm.stats().foodSupply - oldFood) + "; baseline only reports labor, no strict production dependency");
    note("treasury-floor", "EMP1 accepts signed debt; maintenance has no insolvency policy. Negative treasury is a gap, not a current invariant violation.");
    note("market-inventory-water-input", "food is a global scalar; market stock, water quantity and production input inventories have no current API/contract");
}
} // namespace

int main(int argc, char** argv) {
    if (argc != 3) { std::cerr << "usage: city-blackbox-tests OUTPUT baseline|strict-labor|invariants\n"; return 2; }
    output = argv[1]; std::filesystem::create_directories(output);
    const std::string name = argv[2];
    if (name == "strict-labor") profile = Profile::StrictLabor;
    else if (name == "invariants") profile = Profile::Invariants;
    else if (name != "baseline") return 2;
    test("map-build-transaction", "CITY_MODEL.md implementation; canBuild/build/loadFromMap", [] {
        CitySimulation city; require(city.loadFromMap(syntheticMap()), "valid synthetic map");
        writeBytes(output / "synthetic-map.emp", city.saveSnapshot());
        const auto before = city.saveSnapshot(); const auto events = city.events(); std::string message;
        require(!city.loadFromMap({}), "invalid map rejection");
        for (auto type : {BuildingType::None, static_cast<BuildingType>(255)})
            require(!city.canBuild(5, 5, type) && !city.build(5, 5, type, message), "invalid type rejection");
        require(!city.build(64, 5, BuildingType::House, message), "river build rejection");
        require(!city.build(128, 5, BuildingType::House, message), "out-of-map build rejection");
        require(city.saveSnapshot() == before && city.events() == events, "rejections preserve state/events");
        build(city, 5, 5, BuildingType::House); const int money = city.stats().treasury;
        captureBefore(city); require(!city.build(5, 5, BuildingType::Farm, message), "occupied tile rejection");
        captureAfter(city); require(city.stats().treasury == money && beforeEvidence == afterEvidence, "occupied tile charge/transaction");
        invariants(city);
    });
    test("pause-speed-manual-month", "CITY_MODEL.md timing; App onSpeedChanged", [] {
        CitySimulation city; build(city, 10, 10, BuildingType::House);
        for (int i = 0; i < 7; ++i) city.tick(); city.setSpeed(0);
        const auto bytes = city.saveSnapshot(); const auto events = city.events();
        for (int i = 0; i < 1000; ++i) city.tick();
        require(city.saveSnapshot() == bytes && city.events() == events, "pause must freeze full model and tick phase");
        city.advanceMonth(); require(city.stats().month == 2 && city.stats().speed == 0 && city.monthAccumulator() == 7, "manual month works during pause and preserves tick phase");
        CitySimulation one, two, three;
        require(one.loadSnapshot(bytes) && two.loadSnapshot(bytes) && three.loadSnapshot(bytes), "speed fixtures");
        one.setSpeed(1); two.setSpeed(2); three.setSpeed(3);
        for (int i = 0; i < 90; ++i) one.tick();
        for (int i = 0; i < 45; ++i) two.tick();
        for (int i = 0; i < 30; ++i) three.tick();
        two.setSpeed(1); three.setSpeed(1);
        require(one.saveSnapshot() == two.saveSnapshot() && one.saveSnapshot() == three.saveSnapshot(), "equal simulation ticks at 1/2/3x must yield identical state");
        require(one.events() == two.events() && one.events() == three.events(), "speed grouping must preserve events");
        city.setSpeed(99); require(city.stats().speed == 3, "speed clamp upper");
        city.setSpeed(-5); require(city.stats().speed == 0, "speed clamp lower");
        city.reset(); require(city.monthAccumulator() == 0, "reset clears phase");
    });
    test("fresh-instance-file-continuation", "CITY_MODEL.md EMP1; App city_state_io consumer", [] {
        CitySimulation continuous = fullyServicedHouse();
        for (int i = 0; i < 7; ++i) continuous.tick();
        CitySimulation resumed; const auto path = output / "checkpoint.emp"; std::string error;
        require(game_lab::saveCityState(continuous, path, error), "shared file save");
        require(game_lab::loadCityState(resumed, path, error), "fresh shared file load");
        for (unsigned i = 0; i < 180; ++i) {
            context = "continuation step=" + std::to_string(i);
            auto operation = [i](CitySimulation& c) {
                if (i == 11) build(c, 20, 20, BuildingType::Farm);
                if (i == 37) demolish(c, 13, 13);
                if (i == 55) build(c, 13, 13, BuildingType::Well);
                if (i % 17 == 0) c.setSpeed(static_cast<int>((i / 17) % 4));
                if (i % 23 == 0) c.advanceMonth(); else c.tick();
            };
            operation(continuous); operation(resumed); captureBefore(continuous); captureAfter(resumed);
            require(beforeEvidence == afterEvidence, "continuous and resumed serialized state diverged");
            require(continuous.renderGridPixels() == resumed.renderGridPixels(), "continuous and resumed rendered grid diverged");
            require(continuous.stats().rankTitle == resumed.stats().rankTitle, "derived city rank diverged after restore");
            invariants(resumed);
            if (i % 13 == 0) {
                require(game_lab::saveCityState(continuous, path, error), "repeated save");
                CitySimulation next; require(game_lab::loadCityState(next, path, error), "repeated new-instance load"); resumed = next;
            }
        }
        const auto before = resumed.saveSnapshot(); const auto events = resumed.events();
        auto broken = before; broken.push_back(0);
        writeBytes(output / "trailing.emp", broken);
        require(!game_lab::loadCityState(resumed, output / "trailing.emp", error), "trailing file rejected");
        require(resumed.saveSnapshot() == before && resumed.events() == events, "rejected file load preserves state/events");
    });
    test("seeded-operation-properties", "CITY_MODEL.md next-step invariants; existing public API", properties);
    if (profile != Profile::Invariants) {
        test("housing-demolition-recovery", "city_simulation.hpp updateHousing/hasNeighbor/recalculateTotals", [] {
            CitySimulation city; build(city, 10, 10, BuildingType::House); require(city.stats().housingCapacity == 4, "isolated capacity");
            build(city, 11, 11, BuildingType::Road); require(city.stats().housingCapacity == 8, "diagonal radius-1 road capacity");
            build(city, 13, 13, BuildingType::Well); require(city.stats().housingCapacity == 20, "radius-3 well capacity");
            build(city, 14, 14, BuildingType::Market); require(city.stats().housingCapacity == 32, "radius-4 market capacity");
            build(city, 15, 15, BuildingType::Temple); require(city.stats().housingCapacity == 48, "radius-5 temple capacity");
            seed(city, 48, 100, 2); captureBefore(city); demolish(city, 13, 13); captureAfter(city);
            require(city.stats().housingCapacity == 8 && city.stats().population == 8 && city.stats().availableWorkers == 4, "well removal immediately clips capacity/population and recomputes labor");
            build(city, 13, 13, BuildingType::Well);
            require(city.stats().housingCapacity == 48 && city.stats().population == 8, "rebuilding capacity must not instantly spawn population");
            city.advanceMonth(); require(city.stats().population == 14, "immigration resumes next month");
            demolish(city, 11, 11); require(city.stats().housingCapacity == 4 && city.stats().population == 4, "road removal downgrades chain");
            build(city, 11, 11, BuildingType::Road); require(city.stats().housingCapacity == 48, "road recovery restores chain");
            invariants(city);
        });
        test("service-radius-boundaries", "city_simulation.hpp hasNeighbor square bounds", [] {
            CitySimulation city; build(city, 0, 0, BuildingType::House); build(city, 1, 1, BuildingType::Road);
            build(city, 4, 0, BuildingType::Well); require(city.stats().housingCapacity == 8, "well outside radius 3");
            build(city, 3, 3, BuildingType::Well); require(city.stats().housingCapacity == 20, "well at clipped corner radius");
            build(city, 5, 0, BuildingType::Market); require(city.stats().housingCapacity == 20, "market outside radius 4");
            build(city, 4, 4, BuildingType::Market); require(city.stats().housingCapacity == 32, "market at radius 4");
            build(city, 6, 0, BuildingType::Temple); require(city.stats().housingCapacity == 32, "temple outside radius 5");
            build(city, 5, 5, BuildingType::Temple); require(city.stats().housingCapacity == 48, "temple at radius 5");
        });
        test("food-labor-monthly-ledgers", "city_simulation.hpp advanceMonth; selected profile documented in CITY_BLACKBOX_TESTS.md", [] {
            for (int farms : {1, 2, 5}) for (int population : {0, 1, 2, 3, 6, 8, 12, 16, 20, 31, 32}) for (int month : {1, 7, 10}) {
                CitySimulation city = fullyServicedHouse();
                for (int f = 0; f < farms; ++f) build(city, static_cast<unsigned>(30 + f), 30, BuildingType::Farm);
                seed(city, population, 100, month);
                context = "farms=" + std::to_string(farms) + " population=" + std::to_string(population) + " priorMonth=" + std::to_string(month);
                ledgerMonth(city);
            }
        });
        test("shortage-and-migration-threshold", "city_simulation.hpp advanceMonth foodNeeded/starvation/incoming", [] {
            CitySimulation city = fullyServicedHouse(); seed(city, 48, 0, 2);
            captureBefore(city); ledgerMonth(city); captureAfter(city);
            require(city.stats().population == 44 && city.stats().foodSupply == 0 && city.stats().favor == 61, "starvation removes four people and five favor, temple adds one");
            require(city.stats().housingCapacity == 48, "current housing rule is structural during shortage");
            seed(city, 0, 10, 2); city.advanceMonth(); require(city.stats().population == 0, "food=10 prevents immigration");
            seed(city, 0, 11, 2); city.advanceMonth(); require(city.stats().population == 6, "food=11 permits six immigrants");
        });
        test("production-demolition-shortage-recovery", "city_simulation.hpp farm totals -> harvest, food shortage, recalculateTotals", [] {
            CitySimulation city;
            build(city, 10, 10, BuildingType::House); build(city, 10, 11, BuildingType::Road);
            build(city, 11, 10, BuildingType::Well); build(city, 20, 20, BuildingType::Farm); build(city, 21, 20, BuildingType::Farm);
            seed(city, 20, 0, 7); ledgerMonth(city);
            require(city.stats().foodSupply > 0, "staffed farms replenish food");
            demolish(city, 20, 20); require(city.stats().neededWorkers == 10, "one farm removal frees eight jobs"); ledgerMonth(city);
            demolish(city, 21, 20); require(city.stats().totalFarms == 0 && city.stats().neededWorkers == 2, "last farm removal breaks production dependency");
            const int food = city.stats().foodSupply; ledgerMonth(city);
            require(city.stats().foodSupply == food - 2, "no farms means consumption without replacement");
            seed(city, 20, 0, 2); ledgerMonth(city);
            require(city.stats().population == 16 && city.stats().foodSupply == 0, "broken food supply causes real famine");
            build(city, 20, 20, BuildingType::Farm);
            for (int month = 0; month < 6; ++month) ledgerMonth(city);
            require(city.stats().population == 20 && city.stats().foodSupply > 10, "restored production allows population recovery");
        });
        test("fiscal-constraints-and-debt", "CITY_MODEL.md costs; city_simulation.hpp treasury signed/maintenance", [] {
            CitySimulation city;
            for (unsigned x = 0; x < 125; ++x) build(city, x, 5, BuildingType::Farm);
            require(city.stats().treasury == 0, "125 farms exhaust 5000 treasury");
            captureBefore(city); const auto bytes = city.saveSnapshot(); const auto events = city.events(); std::string message;
            require(!city.build(126, 5, BuildingType::Road, message) && !city.demolish(0, 5, message), "unaffordable build/demolition rejected");
            captureAfter(city); require(bytes == afterEvidence && city.events() == events, "insufficient funds preserve state/events");
            city.advanceMonth(); require(city.stats().treasury == -125, "current signed-debt upkeep contract");
            invariants(city);
        });
        test("ten-year-sustainable-map", "self-authored scenario; advanceMonth ledger and documented defaults", [] {
            CitySimulation city; require(city.loadFromMap(syntheticMap()), "sustainable synthetic map");
            for (unsigned x = 9; x <= 17; ++x) build(city, x, 11, BuildingType::Road);
            for (unsigned x : {10u, 12u, 14u, 16u}) build(city, x, 10, BuildingType::House);
            build(city, 13, 12, BuildingType::Well); build(city, 15, 14, BuildingType::Farm); build(city, 17, 14, BuildingType::Farm);
            for (int m = 0; m < 14; ++m) ledgerMonth(city);
            require(city.stats().population == 80 && city.stats().housingCapacity == 80, "four well-serviced houses naturally fill");
            const auto initial = city.stats(); writeBytes(output / "sustainable-mature.emp", city.saveSnapshot());
            for (int m = 0; m < 120; ++m) { context = "mature month=" + std::to_string(m); ledgerMonth(city); }
            require(city.stats().population == 80, "mature settlement maintains population");
            require(city.stats().foodSupply - initial.foodSupply == 2040, "ten-year food surplus=2*(12*5+3*30)*10-8*120");
            require(city.stats().treasury - initial.treasury == 9240, "ten-year tax surplus=(80-3)*120");
            writeBytes(output / "sustainable-end.emp", city.saveSnapshot());
        });
        test("saturation-and-extreme-calendar", "CITY_MODEL.md bounded totals and extreme year", [] {
            CitySimulation city; build(city, 10, 10, BuildingType::House); build(city, 11, 10, BuildingType::Road); build(city, 20, 20, BuildingType::Farm);
            seed(city, 8, std::numeric_limits<int>::max(), 12, std::numeric_limits<int>::max());
            auto bytes = city.saveSnapshot(); set32(bytes, 24, std::numeric_limits<int>::max());
            require(city.loadSnapshot(bytes), "extreme canonical fixture accepted"); captureBefore(city); city.advanceMonth(); captureAfter(city);
            require(city.stats().year == std::numeric_limits<int>::max() && city.stats().month == 1, "year saturated, month wraps");
            require(city.stats().foodSupply == std::numeric_limits<int>::max() - 1, "harvest saturated before consumption");
            require(city.stats().treasury == std::numeric_limits<int>::max(), "treasury addition saturated");
            invariants(city);
        });
    }
    test("farm-inspection-forecast", "frozen inspectTile next-month city yield vs independent oracle and actual advanceMonth", [] { forecastContract<CitySimulation>(); });
    if (profile == Profile::StrictLabor) {
        test("exact-pooled-yield-boundaries", "frozen FarmProductionRules/farmYieldForMonth; exact rational arithmetic and user-requested 73/1 regressions", [] {
            CitySimulation city = threeFarmSettlement(); seed(city, 42, 100, 7);
            require(city.stats().availableWorkers == 21 && city.stats().neededWorkers == 30, "21/30 workforce fixture");
            require(harvest(city.stats(), 8) == 73, "independent oracle: floor(15*21/30)+floor(90*21/30)=10+63");
            require(city.farmYieldForMonth(8) == 73 && city.farmYieldForMonth(9) == 73 && city.farmYieldForMonth(10) == 73, "three farms at 21/30 produce exact autumn 73");
            require(city.farmYieldForMonth(7) == 10 && city.farmYieldForMonth(11) == 10, "outside autumn only regular yield");
            require(city.farmYieldForMonth(0) == 0 && city.farmYieldForMonth(13) == 0, "invalid yield months rejected as zero");
            ledgerMonth(city); require(city.stats().foodSupply == 168, "actual autumn harvest 73 minus five consumed");
            writeBytes(output / "golden-three-farms-autumn-after.emp", city.saveSnapshot());
            CitySimulation two; build(two, 10, 10, BuildingType::House); build(two, 10, 11, BuildingType::Road);
            build(two, 20, 20, BuildingType::Farm); build(two, 21, 20, BuildingType::Farm);
            seed(two, 6, 100, 2); require(two.farmYieldForMonth(3) == 1, "six people/two farms city regular yield is one");
            ledgerMonth(two); require(two.stats().foodSupply == 100, "actual regular yield one minus one consumed");
            writeBytes(output / "golden-two-farms-after.emp", two.saveSnapshot());
        });
    }
    try { gaps(); }
    catch (const std::exception& error) { ++failed; std::cout << "FAIL\tgap-fixture-setup\tself-authored fixture setup\t" << error.what() << '\n'; }
    std::cout << "SUMMARY\tfailures=" << failed << "\tprofile=" << name << '\n';
    return failed ? 1 : 0;
}
