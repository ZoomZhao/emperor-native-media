#include <app/city_goal_feedback_adapter.hpp>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace game_lab;
namespace {
int checks = 0;
void require(bool condition, const char* name) {
    ++checks;
    if (!condition) {
        std::cerr << "FAIL " << name << '\n';
        std::exit(1);
    }
}
bool has(const CityGoalFeedback& out, CityFeedbackCode code) {
    return std::any_of(out.issues.begin(), out.issues.end(),
                       [=](const auto& item) { return item.code == code; });
}
CityFeedbackSnapshot snapshot(int population, int capacity, int houses) {
    CityFeedbackSnapshot s;
    s.population = population;
    s.housingCapacity = capacity;
    s.houses = houses;
    s.harvestRule = CityHarvestRule::LaborLimitedIntegerV1;
    return s;
}
void show(const char* label, const CityGoalFeedback& out) {
    std::cout << label << ": " << out.status << '\n' << out.progressText << '\n' << out.previewText << '\n';
    for (const auto& item : out.issues) std::cout << "  " << item.message << '\n';
}

void goalsAndHistory() {
    auto empty = snapshot(0, 0, 0);
    empty.speed = 0;
    const auto e = evaluateCityGoals(empty);
    require(e.valid && e.state == CityGoalState::InProgress, "empty city state");
    require(e.goals.size() == 4 && e.goals[0].remaining == 50, "empty city population progress");
    require(has(e, CityFeedbackCode::NoHousing) && has(e, CityFeedbackCode::NoFarms) &&
            has(e, CityFeedbackCode::Paused), "empty city actionable limits");
    require(e.nextMonth.known && e.nextMonth.incoming == 0 && e.nextMonth.foodAfter == 400,
            "empty city has no fabricated consumption or migration");
    require(!has(e, CityFeedbackCode::Famine) && !has(e, CityFeedbackCode::HousingCoverageUnknown),
            "empty city does not invent famine or a particular coverage issue");
    show("空城", e);

    auto partial = snapshot(24, 40, 2);
    partial.foodSupply = 80;
    partial.treasury = 400;
    partial.farms = 2;
    const auto p = evaluateCityGoals(partial);
    require(p.valid && p.goals[0].remaining == 26 && p.goals[1].remaining == 40 &&
            p.goals[2].remaining == 100 && p.goals[3].met, "partial target gaps");
    require(p.nextMonth.harvest == 7 && p.nextMonth.consumption == 3 && p.nextMonth.foodAfter == 84 &&
            p.nextMonth.incoming == 6 && p.nextMonth.balance == 28, "manual partial month oracle");
    require(p.availableWorkers == 12 && p.neededWorkers == 16 &&
            has(p, CityFeedbackCode::LaborShortage), "labor derives from actual modeled counts");
    show("部分达成", p);

    auto met = snapshot(50, 64, 2);
    met.foodSupply = 120;
    met.treasury = 500;
    const auto first = evaluateCityGoals(met);
    require(first.state == CityGoalState::Met && first.transition == CityGoalTransition::Reached &&
            first.nextMemory.everMet && first.nextMemory.metLastEvaluation, "first attainment");
    const auto sustained = evaluateCityGoals(met, {}, first.nextMemory);
    require(sustained.transition == CityGoalTransition::None, "attainment emitted only once");
    auto lost = met;
    lost.foodSupply = 119;
    lost.treasury = 499;
    const auto regression = evaluateCityGoals(lost, {}, first.nextMemory);
    require(regression.state == CityGoalState::Regressed && regression.transition == CityGoalTransition::Lost &&
            regression.goals[1].remaining == 1 && regression.goals[2].remaining == 1, "attained then regressed");
    const auto stillLost = evaluateCityGoals(lost, {}, regression.nextMemory);
    require(stillLost.state == CityGoalState::Regressed && stillLost.transition == CityGoalTransition::None,
            "regression remains visible without repeated transition");
    const auto recovered = evaluateCityGoals(met, {}, stillLost.nextMemory);
    require(recovered.state == CityGoalState::Met && recovered.transition == CityGoalTransition::Restored,
            "explicit recoverable attainment");
    show("达成后退步", regression);
    show("补足后恢复", recovered);
    CityGoalConfig replacement{24, 80, 400, 20};
    require(evaluateCityGoals(partial, replacement).state == CityGoalState::Met, "replaceable authored configuration");
}

void shortagesAndBoundaries() {
    auto shortage = snapshot(30, 40, 2);
    shortage.farms = 1;
    shortage.wells = 1;
    shortage.temples = 10;
    shortage.foodSupply = 0;
    shortage.treasury = -7;
    const auto out = evaluateCityGoals(shortage);
    require(out.valid && out.neededWorkers == 50 && out.availableWorkers == 15, "shortage workers");
    require(out.currentMonthlyBalance == -2 && out.nextMonth.harvest == 1 &&
            out.nextMonth.famineLoss == 4 && out.nextMonth.populationAfter == 26 &&
            out.nextMonth.balance == -6 && out.nextMonth.treasuryAfter == -13, "manual famine fiscal oracle");
    require(has(out, CityFeedbackCode::Famine) && has(out, CityFeedbackCode::MigrationFoodBlocked) &&
            has(out, CityFeedbackCode::MonthlyDeficit) && has(out, CityFeedbackCode::TreasuryDebt) &&
            has(out, CityFeedbackCode::ConstructionFundsLow), "resource and budget reasons");
    require(out.goals[3].remaining == 2, "negative monthly balance target gap");
    require(out.nextMonth.migrationFoodGap == 13, "famine recovery includes unmet consumption before migration threshold");
    shortage.foodSupply += 13;
    const auto foodRecovery = evaluateCityGoals(shortage);
    require(foodRecovery.nextMonth.foodAfter == 11 && foodRecovery.nextMonth.famineLoss == 0 &&
            foodRecovery.nextMonth.incoming == 6, "exact advertised famine recovery gap restores migration");
    show("资源不足与亏损", out);

    auto threshold = snapshot(4, 20, 1);
    threshold.foodSupply = 11; // ceil(4/10)=1; exactly 10 after consumption blocks migration.
    const auto blocked = evaluateCityGoals(threshold);
    require(blocked.nextMonth.foodAfter == 10 && blocked.nextMonth.incoming == 0 &&
            blocked.nextMonth.migrationFoodGap == 1 && has(blocked, CityFeedbackCode::MigrationFoodBlocked), "migration strict >10 boundary");
    threshold.foodSupply = 12;
    const auto allowed = evaluateCityGoals(threshold);
    require(allowed.nextMonth.foodAfter == 11 && allowed.nextMonth.incoming == 6 &&
            !has(allowed, CityFeedbackCode::MigrationFoodBlocked), "migration recovers at 11");
    threshold.foodSupply = 1;
    require(evaluateCityGoals(threshold).nextMonth.famineLoss == 0, "exact food consumption does not cause famine");
    threshold.foodSupply = 0;
    require(evaluateCityGoals(threshold).nextMonth.famineLoss == 4, "food one below need causes modeled loss");
    threshold.population = 2;
    require(evaluateCityGoals(threshold).nextMonth.famineLoss == 2, "famine cannot remove more than population");

    auto full = snapshot(4, 4, 1);
    const auto f = evaluateCityGoals(full);
    require(f.nextMonth.incoming == 0 && has(f, CityFeedbackCode::HousingFull) &&
            has(f, CityFeedbackCode::HousingCoverageUnknown), "full housing and unknown spatial cause");
    full.roads = 100;
    full.wells = 100;
    require(has(evaluateCityGoals(full), CityFeedbackCode::HousingCoverageUnknown),
            "service totals do not prove spatial coverage");

    auto rounding = snapshot(2, 4, 1);
    rounding.farms = 1;
    require(evaluateCityGoals(rounding).nextMonth.harvest == 0, "regular harvest truncates fraction");
    rounding.month = 7;
    require(evaluateCityGoals(rounding).nextMonth.harvest == 3, "separate regular/autumn truncation");
    auto seasonal = snapshot(32, 48, 1);
    seasonal.farms = 2;
    seasonal.month = 7;
    require(evaluateCityGoals(seasonal).nextMonth.harvest == 70, "entering August starts autumn harvest");
    seasonal.month = 10;
    require(evaluateCityGoals(seasonal).nextMonth.harvest == 10, "entering November stops autumn harvest");
    seasonal.month = 12;
    require(evaluateCityGoals(seasonal).nextMonth.month == 1 &&
            evaluateCityGoals(seasonal).nextMonth.harvest == 10, "December rollover");
    seasonal.foodSupply = 0;
    seasonal.month = 7;
    const auto autumnRecovery = evaluateCityGoals(seasonal);
    require(autumnRecovery.nextMonth.foodAfter == 66 && autumnRecovery.nextMonth.incoming == 6 &&
            !has(autumnRecovery, CityFeedbackCode::Famine) &&
            !has(autumnRecovery, CityFeedbackCode::MigrationFoodBlocked), "harvest can restore migration despite zero current food");

    auto legacy = snapshot(0, 4, 1);
    legacy.farms = 1;
    legacy.harvestRule = CityHarvestRule::BaselineBb00e6b;
    require(evaluateCityGoals(legacy).nextMonth.harvest == 5, "legacy regular production does not depend on labor");
    legacy.month = 7;
    require(evaluateCityGoals(legacy).nextMonth.harvest == 14, "legacy autumn 30 percent floor");
    legacy.harvestRule = CityHarvestRule::LaborLimitedIntegerV1;
    require(evaluateCityGoals(legacy).nextMonth.harvest == 0, "new rule no workers no yield");
    legacy.harvestRule = CityHarvestRule::Unknown;
    const auto unknown = evaluateCityGoals(legacy);
    require(unknown.valid && !unknown.nextMonth.known && has(unknown, CityFeedbackCode::UnknownHarvestRule) &&
            !has(unknown, CityFeedbackCode::Famine) && !has(unknown, CityFeedbackCode::MigrationFoodBlocked) &&
            !has(unknown, CityFeedbackCode::LaborShortage), "unknown rule does not invent causes");
    legacy.harvestRule = static_cast<CityHarvestRule>(99);
    require(!evaluateCityGoals(legacy).nextMonth.known, "unrecognized future rule fails to unknown");
    legacy.harvestRule = CityHarvestRule::LaborLimitedPrototype;
    require(!evaluateCityGoals(legacy).nextMonth.known, "retired double-rule ID has no silent fallback");
    show("机制未知", unknown);
}

void integerRoundingAndHelperMismatch() {
    auto six = snapshot(6, 8, 2);
    six.farms = 2;
    require(evaluateCityGoals(six).nextMonth.harvest == 1, "six people two farms pooled regular yield is one");
    auto autumn = snapshot(42, 44, 11);
    autumn.farms = 3;
    autumn.wells = 3;
    autumn.month = 7;
    const auto out = evaluateCityGoals(autumn);
    require(out.valid && out.availableWorkers == 21 && out.neededWorkers == 30 &&
            out.nextMonth.harvest == 73 && out.nextMonth.foodAfter == 468,
            "21/30 autumn exact integer yield is 10+63=73, not double 72");
    autumn.coreHarvest = 73;
    require(evaluateCityGoals(autumn).nextMonth.known, "matching authoritative core helper accepted");
    autumn.coreHarvest = 72;
    const auto mismatch = evaluateCityGoals(autumn);
    require(mismatch.valid && !mismatch.nextMonth.known && has(mismatch, CityFeedbackCode::UnknownHarvestRule) &&
            !has(mismatch, CityFeedbackCode::LaborShortage) && !has(mismatch, CityFeedbackCode::Famine) &&
            !has(mismatch, CityFeedbackCode::MigrationFoodBlocked), "conflicting helper fails to unknown without fallback or invented production causes");
    autumn.coreHarvest = -1;
    require(!evaluateCityGoals(autumn).nextMonth.known, "negative helper result fails to unknown");
    show("整数秋收边界", out);
}

void invalidAndExtremeInput() {
    const auto valid = snapshot(4, 4, 1);
    const CityGoalMemory prior{true, true};
    const auto invalid = [&](CityFeedbackSnapshot s, const char* name) {
        const auto out = evaluateCityGoals(s, {}, prior);
        require(!out.valid && out.state == CityGoalState::Unknown && out.goals.empty() &&
                !out.nextMonth.known && has(out, CityFeedbackCode::InvalidSnapshot) &&
                out.nextMemory.everMet && out.nextMemory.metLastEvaluation, name);
    };
    auto s = valid; s.population = -1; invalid(s, "negative population");
    s = valid; s.foodSupply = -1; invalid(s, "negative food");
    s = valid; s.houses = -1; invalid(s, "negative building count");
    s = valid; s.farms = 16385; invalid(s, "building count past map size");
    s = valid; s.roads = 16384; invalid(s, "sum of counts past map size");
    s = valid; s.housingCapacity = 3; invalid(s, "capacity below physical minimum");
    s = valid; s.housingCapacity = 49; invalid(s, "capacity above physical maximum");
    s = valid; s.population = 5; invalid(s, "population beyond capacity");
    s = valid; s.month = 0; invalid(s, "month zero");
    s = valid; s.month = 13; invalid(s, "month thirteen");
    s = valid; s.speed = -1; invalid(s, "negative speed");
    s = valid; s.speed = 4; invalid(s, "speed beyond supported range");
    auto config = CityGoalConfig{};
    config.population = 0;
    require(!evaluateCityGoals(valid, config).valid, "zero population goal");
    config.population = 786433;
    require(!evaluateCityGoals(valid, config).valid, "population goal exceeds map maximum");
    config = {}; config.foodReserve = -1;
    require(!evaluateCityGoals(valid, config).valid, "negative food goal");
    config = {}; config.treasuryReserve = -1;
    require(!evaluateCityGoals(valid, config).valid, "negative treasury goal");
    config = {}; config.monthlyBalance = -1;
    require(!evaluateCityGoals(valid, config).valid, "negative monthly goal");
    require(!evaluateCityGoals(valid, {}, {false, true}).valid, "invalid attainment history");

    s = snapshot(0, 0, 0);
    s.farms = 16384;
    s.foodSupply = std::numeric_limits<int>::max();
    s.treasury = std::numeric_limits<int>::min();
    const auto low = evaluateCityGoals(s);
    require(low.valid && low.nextMonth.treasuryAfter == std::numeric_limits<int>::min() &&
            low.goals[2].remaining == 2147484148LL, "int minimum treasury and wide target gap");
    s = snapshot(786432, 786432, 16384);
    s.foodSupply = std::numeric_limits<int>::max();
    s.treasury = std::numeric_limits<int>::max();
    const auto high = evaluateCityGoals(s);
    require(high.valid && high.nextMonth.consumption == 78644 &&
            high.nextMonth.treasuryAfter == std::numeric_limits<int>::max(), "maximum map population and saturated treasury");
    s = snapshot(32, 48, 1);
    s.farms = 2;
    s.month = 7;
    s.foodSupply = std::numeric_limits<int>::max();
    require(evaluateCityGoals(s).nextMonth.foodAfter == int64_t(std::numeric_limits<int>::max()) - 4,
            "food saturation occurs before consumption");
}

void modelAdapterAndMonthlyOracle() {
#ifdef CITY_GOAL_TEST_SERVICES_V1
    CitySimulation city(CityRuleset::StaffedServices);
#else
    CitySimulation city;
#endif
    std::string message;
    const auto build = [&](uint32_t x, uint32_t y, BuildingType type) {
        require(city.build(x, y, type, message), "self-authored oracle city building");
    };
    build(1, 1, BuildingType::House);
    build(2, 2, BuildingType::House);
    build(1, 2, BuildingType::Road);
    build(2, 1, BuildingType::Well);
    build(3, 1, BuildingType::Market);
    build(4, 1, BuildingType::Temple);
    build(10, 10, BuildingType::Farm);
    build(11, 10, BuildingType::Farm);
#if defined(CITY_GOAL_TEST_SERVICES_V1)
    constexpr auto rule = CityHarvestRule::StaffedServicesV1;
#elif defined(CITY_GOAL_TEST_LEGACY)
    constexpr auto rule = CityHarvestRule::BaselineBb00e6b;
#else
    constexpr auto rule = CityHarvestRule::LaborLimitedIntegerV1;
#endif
    require(city.stats().housingCapacity == (rule == CityHarvestRule::StaffedServicesV1 ? 16 : 96), "real authored housing service capacity");
    require(!evaluateCityGoals(makeCityFeedbackSnapshot(city.stats(), CityHarvestRule::LaborLimitedIntegerV1)).nextMonth.known,
            "stats-only adapter cannot silently assert new helper contract");
    require(!evaluateCityGoals(makeCityFeedbackSnapshot(city, CityHarvestRule::LaborLimitedPrototype)).nextMonth.known,
            "live adapter rejects retired double rule");
    require(!evaluateCityGoals(makeCityFeedbackSnapshot(city, static_cast<CityHarvestRule>(99))).nextMonth.known,
            "live adapter rejects unknown future rule");
#ifdef CITY_GOAL_TEST_LEGACY
    require(!evaluateCityGoals(makeCityFeedbackSnapshot(city, CityHarvestRule::LaborLimitedIntegerV1)).nextMonth.known,
            "new integer rule on a core without helper is unknown");
#else
    require(!evaluateCityGoals(makeCityFeedbackSnapshot(city, CityHarvestRule::BaselineBb00e6b)).nextMonth.known,
            "old baseline ID on a helper-bearing new core is unknown");
#endif
    CityGoalMemory memory;
    for (int i = 0; i < 36; ++i) {
        if (i == 5) city.setSpeed(0);
        if (i == 9) city.setSpeed(3);
        if (i == 15) require(city.demolish(4, 1, message), "oracle demolition changes upkeep and capacity");
        const auto before = city.saveSnapshot();
        const auto events = city.events();
        const auto stats = city.stats();
        const auto out = evaluateCityGoals(makeCityFeedbackSnapshot(city, rule), {}, memory);
        memory = out.nextMemory;
        require(out.valid && out.nextMonth.known, "adapter yields valid readable snapshot");
        require(before == city.saveSnapshot() && events == city.events(), "feedback has no model or event mutations");
        require(out.availableWorkers == stats.availableWorkers && out.neededWorkers == stats.neededWorkers,
                "recomputed labor agrees with model fields");
        city.advanceMonth(); // Explicit manual advance is valid even when paused.
        const auto after = city.stats();
        require(out.nextMonth.month == after.month && out.nextMonth.foodAfter == after.foodSupply &&
                out.nextMonth.populationAfter == after.population && out.nextMonth.treasuryAfter == after.treasury,
                "forecast matches actual authored monthly sequence");
    }
    std::cout << "36 个月自写模型对照通过（规则="
              << (rule == CityHarvestRule::BaselineBb00e6b ? "bb00e6b" : "精确整数缺工版") << "）\n";
}

#ifndef CITY_GOAL_TEST_LEGACY
void frozenCoreAutumnFixture() {
    CitySimulation city;
    std::string message;
    // Eleven isolated houses (44 capacity), three distant wells and three farms => 30 jobs.
    for (uint32_t i = 0; i < 11; ++i) require(city.build(i * 5, 0, BuildingType::House, message), "boundary fixture house");
    for (uint32_t i = 0; i < 3; ++i) {
        require(city.build(i, 20, BuildingType::Well, message), "boundary fixture well");
        require(city.build(i, 30, BuildingType::Farm, message), "boundary fixture farm");
    }
    // Seven manual advances from January reach August with 42 people. Next September is also autumn.
    for (int i = 0; i < 7; ++i) city.advanceMonth();
    require(city.stats().population == 42 && city.stats().availableWorkers == 21 && city.stats().neededWorkers == 30,
            "real core reaches 21/30 boundary without mutable state access");
    const auto before = city.saveSnapshot();
    const auto events = city.events();
    const auto s = makeCityFeedbackSnapshot(city, CityHarvestRule::LaborLimitedIntegerV1);
    const auto out = evaluateCityGoals(s);
    require(s.coreHarvest && *s.coreHarvest == 73 && out.nextMonth.known && out.nextMonth.harvest == 73,
            "authoritative real core helper and module both return 73");
    require(before == city.saveSnapshot() && events == city.events(), "helper adapter remains read-only");
    city.advanceMonth();
    require(out.nextMonth.foodAfter == city.stats().foodSupply && out.nextMonth.populationAfter == city.stats().population &&
            out.nextMonth.treasuryAfter == city.stats().treasury, "21/30 exact boundary matches real month settlement");
}
#endif
} // namespace

int main() {
    goalsAndHistory();
    shortagesAndBoundaries();
    integerRoundingAndHelperMismatch();
    invalidAndExtremeInput();
    modelAdapterAndMonthlyOracle();
#ifndef CITY_GOAL_TEST_LEGACY
    frozenCoreAutumnFixture();
#endif
    std::cout << "PASS city_goal_feedback " << checks << " checks\n";
}
