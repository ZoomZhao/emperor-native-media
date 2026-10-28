// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>

namespace game_lab {
// Offsets include Common5's serialized own-version word. Runtime receiver
// offsets74/76/78 are different. These are signed accumulators, not percentages.
// Names describe the existing project disaster branches, not a measured
// probability or a complete original-game hazard model. Keep legacy members
// fireRisk/damageRisk and their persisted keys byte compatible.
struct OriginalBuildingRiskWords {
    int16_t collapse=0,ignition=0,burning=0;
    static constexpr size_t collapseOffset=68,ignitionOffset=70,burningOffset=72;
};
inline std::optional<OriginalBuildingRiskWords> originalBuildingRiskWords(const uint8_t* common,size_t size){
    if(!common||size<OriginalBuildingRiskWords::burningOffset+2)return std::nullopt;
    const auto word=[&](size_t at){const unsigned u=unsigned(common[at])|(unsigned(common[at+1])<<8);
        return int16_t(u<=32767?int(u):int(u)-65536);};
    return OriginalBuildingRiskWords{word(68),word(70),word(72)};
}
}
