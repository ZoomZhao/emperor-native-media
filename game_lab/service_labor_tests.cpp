#include "app/city_goal_feedback_adapter.hpp"
#include "city_state_io.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace game_lab;
static size_t checks = 0;
static void require(bool condition, const char* reason) {
    ++checks;
    if (!condition) throw std::runtime_error(reason);
}
static void set32(std::vector<uint8_t>& bytes, size_t offset, int value) {
    for (unsigned i = 0; i < 4; ++i) bytes.at(offset + i) = uint32_t(value) >> (8 * i);
}
static void build(CitySimulation& city, uint32_t x, uint32_t y, BuildingType type) {
    std::string error;
    require(city.build(x, y, type, error), "synthetic building accepted");
}
static CitySimulation village(CityRuleset rule) {
    CitySimulation city(rule);
    build(city, 10, 10, BuildingType::Road);
    for (const auto& pos : {std::pair{10u, 11u}, {11u, 10u}, {9u, 10u}})
        build(city, pos.first, pos.second, BuildingType::House);
    build(city, 10, 9, BuildingType::Well);
    build(city, 20, 20, BuildingType::Farm);
    city.setSpeed(0);
    return city;
}
static void canonical(const CitySimulation& city) {
    const auto snapshot = city.saveSnapshot();
    CitySimulation restored;
    require(restored.loadSnapshot(snapshot), "current save is canonical and loadable");
    require(restored.saveSnapshot() == snapshot, "roundtrip preserves every byte and ruleset");
    const auto& s = city.stats();
    require(s.population <= s.housingCapacity && s.population >= 0 && s.foodSupply >= 0, "capacity and food invariants");
    require(s.availableWorkers == s.population / 2, "workforce follows actual post-displacement population");
    int sum = 0;
    for (uint32_t y = 0; y < 128; ++y) for (uint32_t x = 0; x < 128; ++x) sum += city.houseCapacityAt(x, y);
    require(sum == s.housingCapacity, "per-house capacity agrees with city capacity");
}
static void forecastMatches(CitySimulation& city) {
    const auto before = city.saveSnapshot();
    const auto events = city.events();
    const auto projected = city.nextMonthStats();
    const auto rule = city.ruleset() == CityRuleset::StaffedServices ? CityHarvestRule::StaffedServicesV1 : CityHarvestRule::LaborLimitedIntegerV1;
    const auto report = evaluateCityGoals(makeCityFeedbackSnapshot(city, rule));
    require(report.valid && report.nextMonth.known, "current feedback recognizes actual ruleset");
    require(city.saveSnapshot() == before && city.events() == events, "next-month prediction is read-only");
    city.advanceMonth();
    const auto& s = city.stats();
    require(s.population == projected.population && s.housingCapacity == projected.housingCapacity &&
            s.foodSupply == projected.foodSupply && s.treasury == projected.treasury, "core preview matches actual next month");
    require(report.nextMonth.populationAfter == s.population && report.nextMonth.foodAfter == s.foodSupply &&
            report.nextMonth.treasuryAfter == s.treasury, "visible feedback matches population/food/treasury after actual service changes");
    canonical(city);
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "isolated fixture directory required");
        const std::filesystem::path root(argv[1]);
        std::filesystem::create_directories(root);
        auto staffed = village(CityRuleset::StaffedServices);
        auto legacy = village(CityRuleset::LegacyServices);
        require(staffed.stats().housingCapacity == 24 && legacy.stats().housingCapacity == 60, "zero labor removes real well coverage only under new rules");
        require(staffed.serviceEffectiveRadius(BuildingType::Well) == 0 && !staffed.serviceCoverageAt(9, 10, BuildingType::Well), "zero workers stop service");
        require(staffed.inspectTile(10, 9).find("缺工停用") != std::string::npos, "inspection explains actual stopped service");
        canonical(staffed);
        forecastMatches(staffed); // population 6, workforce 3/10: radius 0
        require(staffed.stats().population == 6 && staffed.stats().housingCapacity == 24, "first migration cannot invent full service");
        forecastMatches(staffed); // population 12, workforce 6/10: radius 1
        require(staffed.stats().population == 12 && staffed.stats().housingCapacity == 48, "partial workers expand only the real one-tile range");
        require(staffed.serviceCoverageAt(9, 10, BuildingType::Well) && !staffed.serviceCoverageAt(10, 11, BuildingType::Well), "near house benefits; distance-two house does not");
        forecastMatches(staffed);
        require(staffed.stats().population == 18 && staffed.stats().housingCapacity == 60 && staffed.serviceEffectiveRadius(BuildingType::Well) == 2, "more workers restore distance-two service");
        for (int i = 0; i < 6; ++i) forecastMatches(staffed);
        require(staffed.stats().population == 54 && staffed.serviceEffectiveRadius(BuildingType::Well) == 3, "naturally staffed village reaches full service");
        for (uint32_t x = 21; x <= 28; ++x) { build(staffed, x, 20, BuildingType::Farm); canonical(staffed); }
        require(staffed.stats().population == 24 && staffed.stats().housingCapacity == 24 && staffed.serviceEffectiveRadius(BuildingType::Well) == 0, "extra jobs cause real service collapse and immediate canonical displacement");
        std::string error;
        for (uint32_t x = 21; x <= 28; ++x) require(staffed.demolish(x, 20, error), "remove competing jobs");
        require(staffed.stats().population == 24 && staffed.stats().housingCapacity == 60, "reducing jobs restores capacity without inventing migrants");
        for (int i = 0; i < 5; ++i) forecastMatches(staffed);
        require(staffed.stats().population == 54, "migration recovers after service is restored");

        for (uint32_t x = 40; x < 54; ++x) build(staffed, x, 40, BuildingType::Temple);
        auto starvation = staffed.saveSnapshot();
        set32(starvation, 12, 44); set32(starvation, 20, 0);
        require(staffed.loadSnapshot(starvation), "canonical authored famine fixture accepted");
        const auto output = evaluateCityGoals(makeCityFeedbackSnapshot(staffed, CityHarvestRule::StaffedServicesV1));
        require(output.nextMonth.known && output.nextMonth.famineLoss == 4 && output.nextMonth.housingLoss == 16 &&
                output.nextMonth.populationAfter == 24, "famine crosses service threshold: feedback includes housing displacement");
        forecastMatches(staffed);

        const auto oldBytes = legacy.saveSnapshot();
        require(oldBytes.at(4) == 1 && staffed.saveSnapshot().at(4) == 3, "old/new service rules have explicit distinct saved versions");
        require(staffed.loadSnapshot(oldBytes) && staffed.ruleset() == CityRuleset::LegacyServices && staffed.saveSnapshot() == oldBytes,
                "loading v1 preserves old service behavior without silent population migration");
        for (int i = 0; i < 36; ++i) { legacy.advanceMonth(); staffed.advanceMonth(); require(legacy.saveSnapshot() == staffed.saveSnapshot(), "v1 continuation remains exact"); }
        const auto stable = staffed.saveSnapshot(); const auto oldEvents = staffed.events();
        for (int badVersion : {0, 2, 4, 99}) {
            auto invalid = stable; set32(invalid, 4, badVersion);
            require(!staffed.loadSnapshot(invalid) && staffed.saveSnapshot() == stable && staffed.events() == oldEvents, "unsupported version rejected transactionally");
        }
        staffed.reset(CityRuleset::StaffedServices);
        require(staffed.ruleset() == CityRuleset::StaffedServices && staffed.stats().population == 0, "new city explicitly enables staffed services");
        auto running = village(CityRuleset::StaffedServices); running.setSpeed(3);
        for (unsigned i = 0; i < 120; ++i) {
            running.tick(); canonical(running);
            const auto file = root / ("checkpoint-" + std::to_string(i) + ".emp");
            require(saveCityState(running, file, error), "real file consumer saves v3");
            CitySimulation resumed; require(loadCityState(resumed, file, error), "real file consumer restores v3 rules");
            auto uninterrupted = running;
            for (int j = 0; j < 12; ++j) { uninterrupted.tick(); resumed.tick(); require(uninterrupted.saveSnapshot() == resumed.saveSnapshot(), "saved service ranges and future ticks remain identical"); }
        }
        for (BuildingType service : {BuildingType::Well, BuildingType::Market, BuildingType::Temple, BuildingType::Watchtower}) {
            CitySimulation zero(CityRuleset::StaffedServices); build(zero, 30, 30, service);
            require(!zero.serviceIsOperational(service), "each zero-worker service really stops");
            const int favor = zero.stats().favor; zero.advanceMonth();
            require(zero.stats().favor == favor, "unstaffed temple/tower cannot supply prosperity");
        }
        CitySimulation ranges(CityRuleset::StaffedServices);
        for (uint32_t x = 0; x < 20; ++x) build(ranges, x * 5, 0, BuildingType::House);
        build(ranges, 20, 20, BuildingType::Farm);
        build(ranges, 21, 20, BuildingType::Well);
        build(ranges, 22, 20, BuildingType::Market);
        build(ranges, 23, 20, BuildingType::Temple);
        build(ranges, 24, 20, BuildingType::Watchtower);
        for (uint32_t x = 40; x < 45; ++x) build(ranges, x, 40, BuildingType::Temple);
        auto half = ranges.saveSnapshot(); set32(half, 12, 40); set32(half, 28, 3);
        require(ranges.loadSnapshot(half) && ranges.stats().neededWorkers == 40, "half-staffed canonical fixture accepted");
        require(ranges.serviceEffectiveRadius(BuildingType::Well) == 1 && ranges.serviceEffectiveRadius(BuildingType::Market) == 2 &&
                ranges.serviceEffectiveRadius(BuildingType::Temple) == 2 && !ranges.serviceIsOperational(BuildingType::Watchtower),
                "independent half-staffing expectations for every service type");
        require(ranges.serviceCoverageAt(22, 21, BuildingType::Well) && !ranges.serviceCoverageAt(23, 22, BuildingType::Well) &&
                ranges.serviceCoverageAt(22, 22, BuildingType::Market) && !ranges.serviceCoverageAt(22, 23, BuildingType::Market) &&
                ranges.serviceCoverageAt(25, 22, BuildingType::Temple) && !ranges.serviceCoverageAt(25, 23, BuildingType::Temple),
                "exact shrunk radius includes diagonal boundary and excludes next tile");
        require(!evaluateCityGoals(makeCityFeedbackSnapshot(ranges, CityHarvestRule::LaborLimitedIntegerV1)).nextMonth.known,
                "legacy feedback rule cannot silently forecast a new service city");
        auto forged = makeCityFeedbackSnapshot(ranges, CityHarvestRule::StaffedServicesV1);
        forged.coreNextMonth.reset();
        require(!evaluateCityGoals(forged).nextMonth.known, "missing authoritative service forecast is unknown");
        const int halfFavor = ranges.stats().favor;
        forecastMatches(ranges);
        require(ranges.stats().favor == halfFavor - 1, "half-staffed tower supplies no bonus and fails the quarterly safeguard");
        auto full = half; set32(full, 12, 80);
        require(ranges.loadSnapshot(full) && ranges.serviceIsOperational(BuildingType::Watchtower), "fully staffed fixture restores tower");
        require(ranges.serviceEffectiveRadius(BuildingType::Well) == 3 && ranges.serviceEffectiveRadius(BuildingType::Market) == 4 &&
                ranges.serviceEffectiveRadius(BuildingType::Temple) == 5, "full labor restores every configured radius");
        const int fullFavor = ranges.stats().favor; forecastMatches(ranges);
        require(ranges.stats().favor == fullFavor + 2, "operating temple and tower both provide their actual effects");
        std::cout << "{\"passed\":true,\"checks\":" << checks << ",\"service_rules\":3,\"legacy_preserved\":true}\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
