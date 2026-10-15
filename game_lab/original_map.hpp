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
// Public modern mapper variable names only; runtime consumption is unknown.
struct OriginalMapCellLayers {
    static constexpr size_t cells=228u*228u;
    static constexpr size_t edgesOffset=209471,objectsOffset=469391,
        mapperRandomOffset=729311,mapperFertileOffset=937255;
    std::vector<uint32_t> objects;
    std::vector<uint8_t> edges,mapperRandom,mapperFertile;
    bool valid() const { return objects.size()==cells&&edges.size()==cells&&
        mapperRandom.size()==cells&&mapperFertile.size()==cells; }
};
std::optional<OriginalMapCellLayers> parseOriginalMapCellLayers(const std::vector<uint8_t>& logicalPayload);
// Caller-selected diagnostic reshape; neither location nor image semantics inferred.
struct OriginalMapRawSpec { size_t offset=0; unsigned width=0; };
struct OriginalMapRawLayer {
    OriginalMapRawSpec spec;
    std::vector<uint32_t> values;
    bool valid() const { return (spec.width==1||spec.width==2||spec.width==4)&&values.size()==228u*228u; }
};
std::optional<OriginalMapRawLayer> parseOriginalMapRawLayer(const std::vector<uint8_t>& logicalPayload,OriginalMapRawSpec);
struct OriginalMapResult {
    OriginalMapStatus status=OriginalMapStatus::ReadFailure;
    OriginalMapTerrain terrain;
    std::optional<OriginalMapCellLayers> cellLayers;
    std::optional<OriginalMapRawLayer> diagnosticRawLayer;
    size_t inputBytes=0,decodedBytes=0,frames=0;
};
struct OriginalMapAnalysis {
    bool cropSupported=false;
    uint32_t cropOrigin=0,cropSide=0,unknownBits=0;
    size_t activeCells=0,outsideCropCells=0,unknownFlagCells=0;
    std::array<size_t,32> unknownBitOccurrences{};
    bool cropValid() const { return cropSupported && activeCells>0 && outsideCropCells==0; }
};
// Shared bounded compressed-data and city-payload stages. Non-city PAK parts
// may be complete containers without being city maps; no partial bytes publish.
struct OriginalMapContainerResult {
    bool complete=false;
    OriginalMapStatus failure=OriginalMapStatus::InvalidContainer;
    std::vector<uint8_t> payload;
    size_t inputBytes=0,frames=0;
};
OriginalMapContainerResult decodeOriginalMapContainer(const std::vector<uint8_t>&);
OriginalMapResult parseOriginalMapPayload(const std::vector<uint8_t>&,
    std::optional<OriginalMapRawSpec> diagnosticSpec=std::nullopt);
// Authored implementation from public format facts and byte validation.
// No city state, game rules, GPL parser/renderer or original code.
OriginalMapResult loadOriginalMapTerrain(const std::string& absolutePath,
    std::optional<OriginalMapRawSpec> diagnosticSpec=std::nullopt);
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
