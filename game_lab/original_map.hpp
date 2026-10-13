#pragma once
#include "emperor_assets.hpp"
#include <string>
#include <vector>
#include <array>

namespace game_lab {
enum class OriginalMapStatus { LoadedTerrain, ReadFailure, InvalidContainer, UnsupportedContainer, UnsupportedRole, InvalidGrid, BudgetExceeded };
struct OriginalMapTerrain {
    static constexpr uint32_t storageSide=228;
    uint32_t activeSide=0;
    std::vector<uint32_t> flags;
    bool valid() const { return activeSide>0 && activeSide<=storageSide && flags.size()==size_t(storageSide)*storageSide; }
};
struct OriginalMapResult {
    OriginalMapStatus status=OriginalMapStatus::ReadFailure;
    OriginalMapTerrain terrain;
    size_t inputBytes=0,decodedBytes=0,frames=0;
};
struct OriginalMapAnalysis {
    bool cropSupported=false;
    uint32_t cropOrigin=0,cropSide=0,unknownBits=0;
    size_t activeCells=0,outsideCropCells=0,unknownFlagCells=0;
    std::array<size_t,32> unknownBitOccurrences{};
    bool cropValid() const { return cropSupported && activeCells>0 && outsideCropCells==0; }
};
// Authored implementation from public format facts and byte validation.
// No city state, objects, game rules, GPL parser/renderer or original code.
OriginalMapResult loadOriginalMapTerrain(const std::string& absolutePath);
// Diagnostic colors encode values/bits only, not game terrain semantics.
OriginalMapAnalysis analyzeOriginalMap(const OriginalMapTerrain&);
std::string originalMapFlagLabel(unsigned bit);
DecodedImage renderOriginalMapFlags(const OriginalMapTerrain&,int bit=-1,bool crop=false);
std::string describeOriginalMap(const OriginalMapResult&);
} // namespace game_lab
