#pragma once
#include "original_map.hpp"
#include <cstdint>
namespace game_lab {
struct MapReferenceStats {
    size_t activeCells=0,validIDs=0,geometrySupported=0,outsideArchive=0,unknownGeometry=0,culled=0,
           visibleCells=0,drawnCells=0,decodeFailures=0,uniqueDecoded=0,cacheBytes=0;
};
struct MapReferenceResult {
    DecodedImage image;
    MapReferenceStats stats;
    bool experimental=true;
};
// Explicit caller archive/base/camera; no engine selector, original draw-order rule,
// captured coordinates, per-cell overrides, or pixels from an original screenshot.
MapReferenceResult renderMapReferenceViewport(const OriginalMapTerrain&,const OriginalMapRawLayer&,
    const SG3Archive&,const std::string& internal555,int64_t idBase,const OriginalMapCamera&);

enum class SG3DiagnosticComponent { Full, Top };
struct SG3DiagnosticLayer {
    uint32_t logicalID=0;
    SG3DiagnosticComponent component=SG3DiagnosticComponent::Full;
    double centerX=0,centerY=0;
};
struct SG3DiagnosticScene {
    uint32_t width=0,height=0,scale=1;
    std::vector<SG3DiagnosticLayer> layers;
};
// Caller-declared integer active-cell to physical-pixel affine registration.
// canvasOrigin is the physical-pixel top-left of the diagnostic canvas. The
// registration is an explicit hypothesis, never recovered from SAV fields.
struct SG3DiagnosticRegistration {
    std::array<std::array<int64_t,3>,2> matrix{};
    std::array<int64_t,2> canvasOrigin{};
};
// Validated crop/on-map cell only; no resource ID selection or elevation rule.
std::optional<OriginalMapPoint> projectSG3DiagnosticCell(const OriginalMapTerrain&,
    const SG3DiagnosticRegistration&,int64_t activeX,int64_t activeY);
struct SG3DiagnosticResult {
    DecodedImage image;
    // Final natural-alpha producer: layer index+1, or0 for transparent/unknown.
    std::vector<uint16_t> producer;
    std::vector<size_t> visiblePerLayer;
    size_t coveredPixels=0,decodedLayers=0;
};
// Explicit hypotheses only. No MAP/SAV selector, screenshot/background input,
// colour correction, inferred visibility mask or original draw-order contract.
// Bounded internal, nonmirrored, binary-alpha type30 single-cell components.
DecodedImage decodeSG3DiagnosticComponent(const SG3Archive&,size_t logicalIndex,
    const std::string& internal555,SG3DiagnosticComponent);
SG3DiagnosticResult renderSG3DiagnosticScene(const SG3Archive&,const std::string& internal555,
    const SG3DiagnosticScene&);
// Validate the complete output set before any writer runs. Resolve actual parent
// directories first (including symlinks and dot segments), then compare targets
// and existing-file identities. Returns normalized fresh paths; never writes.
std::vector<std::string> freshReferenceOutputs(const std::string& dataRoot,
    const std::vector<std::string>& requested);
}
