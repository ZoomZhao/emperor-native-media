// SPDX-License-Identifier: MIT
#pragma once
#include "native_housing_goal.hpp"
#include "original_building.hpp"
#include <set>

namespace game_lab {
// Read-only bridge from the existing SAV building reader. The caller supplies
// a separately qualified positive threshold (e.g. the visible tutorial150).
// No cHousingGoal field, UI label, model index or type ordering is interpreted
// here. All saved tiers remain Unknown until an objective-specific mapping is
// independently qualified. This bridge is not installed in the App tick chain.
inline NativeHousingGoalEvaluation evaluateOriginalSavedHousingPopulation(
    const std::vector<uint8_t>& data, const OriginalBuildingProfile& profile,
    int64_t threshold) {
    if (threshold <= 0) return NativeHousingGoalError::InvalidThreshold;
    if (profile.status != OriginalBuildingStatus::Complete || profile.parsedRecords != 4000)
        return NativeHousingGoalError::IncompleteInventory;
    if (!profile.tableEnd || profile.tableEnd > data.size())
        return NativeHousingGoalError::InvalidResidenceInventory;
    const auto houses = originalBuildingHouseCapacityInputs(data, profile);
    if (!houses) return NativeHousingGoalError::InvalidResidenceInventory;
    std::array<size_t, 18> seen{};
    std::set<size_t> slots;
    for (const auto& footprint : profile.footprints) {
        if (footprint.type < 2 || footprint.type > 17) continue;
        if (footprint.slot >= profile.slotTypes.size() ||
            profile.slotTypes[footprint.slot] != footprint.type ||
            !slots.insert(footprint.slot).second)
            return NativeHousingGoalError::InvalidResidenceInventory;
        ++seen[footprint.type];
    }
    // A structurally complete table can still withhold invalid/unplaced
    // footprints. Such missing residences must not silently shrink the upper
    // bound. Unrelated nonresidential geometry need not block this evaluation.
    for (uint16_t type = 2; type <= 17; ++type) {
        const auto entry = profile.liveTypeCounts.find(type);
        const size_t expected = entry == profile.liveTypeCounts.end() ? 0 : entry->second;
        if (seen[type] != expected) return NativeHousingGoalError::IncompleteInventory;
    }
    if (houses->size() != slots.size()) return NativeHousingGoalError::InvalidResidenceInventory;
    std::vector<NativeHousingResidence> residences;
    residences.reserve(houses->size());
    for (const auto& house : *houses)
        residences.push_back({house.second.residents, NativeHousingEligibility::Unknown});
    return evaluateNativeHousingPopulation(residences, true, threshold);
}
} // namespace game_lab
