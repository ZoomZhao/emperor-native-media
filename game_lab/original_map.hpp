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
// Flat diagnostic camera in validated active-crop coordinates. No screenshot
// positions, original palette, elevation rules or SG3 selector are embedded.
struct OriginalMapCamera {
    double centerX=0,centerY=0;
    uint32_t width=798,height=720,scale=1;
    bool outlines=true;
};
struct OriginalMapPoint { double x=0,y=0; };
std::optional<OriginalMapPoint> projectOriginalMapCell(const OriginalMapTerrain&,const OriginalMapCamera&,uint32_t activeX,uint32_t activeY);
DecodedImage renderOriginalMapSpatialFlags(const OriginalMapTerrain&,const OriginalMapCamera&,int bit=-1);
std::string describeOriginalMap(const OriginalMapResult&);
} // namespace game_lab
