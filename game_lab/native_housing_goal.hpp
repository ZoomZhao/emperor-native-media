// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <limits>
#include <variant>
#include <vector>

namespace game_lab {
// A finite native compatibility rule, not an original event/goal decoder.
// Eligibility must be established by the caller for this particular objective.
// A missing tier mapping is Unknown, never a nonqualifying/qualifying default.
enum class NativeHousingEligibility { Unknown, Qualifies, DoesNotQualify };
struct NativeHousingResidence {
    int64_t residents;
    NativeHousingEligibility eligibility;
};
enum class NativeHousingGoalVerdict { DefinitelyNotMet, DefinitelyMet, Undetermined };
struct NativeHousingGoalInterval {
    int64_t lower;
    int64_t upper;
    int64_t totalResidents;
    NativeHousingGoalVerdict verdict;
};
enum class NativeHousingGoalError {
    InvalidThreshold, IncompleteInventory, NegativeResidents,
    InvalidEligibility, PopulationOverflow, InvalidResidenceInventory
};
using NativeHousingGoalEvaluation = std::variant<NativeHousingGoalInterval, NativeHousingGoalError>;

// Complete empty inventories are valid. Only positive population thresholds
// are supported. Errors contain no usable partial interval. No event, clock,
// receipt, completion flag or source data is modified by this function.
inline NativeHousingGoalEvaluation evaluateNativeHousingPopulation(
    const std::vector<NativeHousingResidence>& residences,
    bool inventoryComplete, int64_t threshold) {
    if (threshold <= 0) return NativeHousingGoalError::InvalidThreshold;
    if (!inventoryComplete) return NativeHousingGoalError::IncompleteInventory;
    int64_t lower = 0, upper = 0, total = 0;
    for (const auto& residence : residences) {
        if (residence.residents < 0) return NativeHousingGoalError::NegativeResidents;
        switch (residence.eligibility) {
        case NativeHousingEligibility::Unknown:
        case NativeHousingEligibility::Qualifies:
        case NativeHousingEligibility::DoesNotQualify: break;
        default: return NativeHousingGoalError::InvalidEligibility;
        }
        // Check the whole population, including unqualified residents. Since
        // 0 <= lower <= upper <= total, this also bounds both later additions.
        if (residence.residents > std::numeric_limits<int64_t>::max() - total)
            return NativeHousingGoalError::PopulationOverflow;
        total += residence.residents;
        if (residence.eligibility != NativeHousingEligibility::DoesNotQualify)
            upper += residence.residents;
        if (residence.eligibility == NativeHousingEligibility::Qualifies)
            lower += residence.residents;
    }
    const auto verdict = upper < threshold ? NativeHousingGoalVerdict::DefinitelyNotMet
        : lower >= threshold ? NativeHousingGoalVerdict::DefinitelyMet
        : NativeHousingGoalVerdict::Undetermined;
    return NativeHousingGoalInterval{lower, upper, total, verdict};
}
} // namespace game_lab
