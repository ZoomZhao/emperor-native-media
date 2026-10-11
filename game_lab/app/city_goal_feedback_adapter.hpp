#pragma once
#include "city_goal_feedback.hpp"
#include "../city_simulation.hpp"
#include <type_traits>

namespace game_lab {

// Stats-only callers cannot confirm the integer helper contract; use the city overload for v2.
// This legacy overload supports only an explicitly selected bb00e6b stats snapshot.
inline CityFeedbackSnapshot makeCityFeedbackSnapshot(const CityStats& stats, CityHarvestRule rule) {
    CityFeedbackSnapshot s;
    s.treasury = stats.treasury;
    s.population = stats.population;
    s.housingCapacity = stats.housingCapacity;
    s.foodSupply = stats.foodSupply;
    s.month = stats.month;
    s.speed = stats.speed;
    s.roads = stats.totalRoads;
    s.houses = stats.totalHouses;
    s.farms = stats.totalFarms;
    s.wells = stats.totalWells;
    s.temples = stats.totalTemples;
    s.markets = stats.totalMarkets;
    s.watchtowers = stats.totalWatchtowers;
    s.harvestRule = rule == CityHarvestRule::BaselineBb00e6b ? rule : CityHarvestRule::Unknown;
    return s;
}

namespace city_feedback_adapter_detail {
template<class City, class = void> struct HasFarmYieldHelper : std::false_type {};
template<class City> struct HasFarmYieldHelper<City, std::void_t<decltype(
    std::declval<const City&>().farmYieldForMonth(1))>> : std::true_type {};

template<class City>
inline CityFeedbackSnapshot capture(const City& city, CityHarvestRule rule) {
    auto s = makeCityFeedbackSnapshot(city.stats(), CityHarvestRule::Unknown);
    if constexpr (HasFarmYieldHelper<City>::value) {
        if (rule == CityHarvestRule::LaborLimitedIntegerV1) {
            s.harvestRule = rule;
            const int nextMonth = s.month == 12 ? 1 : s.month + 1;
            s.coreHarvest = city.farmYieldForMonth(nextMonth);
        }
        // Older IDs on a helper-bearing core stay unknown; no silent legacy fallback.
    } else if (rule == CityHarvestRule::BaselineBb00e6b) {
        s.harvestRule = rule;
    }
    return s;
}
} // namespace city_feedback_adapter_detail

// Preferred App API: read public stats and the core's authoritative yield helper.
// A missing helper or unsupported rule returns unknown. No ticks, events, assets, or EMP1 changes.
inline CityFeedbackSnapshot makeCityFeedbackSnapshot(const CitySimulation& city, CityHarvestRule rule) {
    return city_feedback_adapter_detail::capture(city, rule);
}

} // namespace game_lab
