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
}
