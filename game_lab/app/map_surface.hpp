// SPDX-License-Identifier: MIT
#pragma once
#include "../map_reference.hpp"
#include "../original_building_scene.hpp"
#include "../resource_root.hpp"
#include <map>
#include <optional>

namespace game_lab {
struct OriginalMapSource;
enum class AppSurfaceIssueKind { GroupSideConflict, BlockGeometryUnsupported, ArchiveUnavailable, IndexUnresolved };
struct AppSurfaceIssue {
    uint32_t x=0,y=0,storedWord=0,recordSide=0,groupSide=0;
    AppSurfaceIssueKind kind=AppSurfaceIssueKind::BlockGeometryUnsupported;
};
struct AppSurfaceDiagnostics {
    std::vector<AppSurfaceIssue> cells;
    size_t totalUnknown=0,unlocated=0;
};
// Coordinates are zero-based active-grid positions,not an original-screen origin.
std::string describeAppSurfaceDiagnostics(const AppSurfaceDiagnostics&,size_t maxRows=2048);
struct AppSurfaceCandidateStats {
    size_t structuralCells=0,structuralGroups=0,decodedCells=0,decodedGroups=0,
        drawnCells=0,drawnGroups=0,paintedPixels=0,decodeFailureCells=0,rejectedGroupCells=0,cacheBytes=0;
};
struct AppSurfacePreviewResult {
    DecodedImage image;
    MapImageGridStats verified;
    AppSurfaceCandidateStats candidate;
};
// Explicit preview convention,not a recovered original painter/anchor rule.
// The strict renderer and its metrics remain unchanged.
AppSurfacePreviewResult renderMapSurfaceCandidateViewport(const OriginalMapTerrain&,
    const OriginalTerrainState&,const std::vector<MapImageGridTileset>&,const OriginalMapCamera&);
// App adapter for the existing image-grid renderer. SAV optionally reconstructs
// ground from its persisted inputs; no screen registration or gameplay is used.
class AppMapSurface {
public:
    explicit AppMapSurface(ResourceRoot root) : root_(std::move(root)) {}
    void clear();
    bool select(const std::string& relativeMap, const OriginalMapTerrain& expected);
    bool select(const OriginalMapSource&,const OriginalMapTerrain& expected);
    bool selectSavedBuildings(const OriginalMapTerrain&,const OriginalBuildingProfile&,const OriginalSavedGround* ground=nullptr,uint64_t tickMs=0,const OriginalFigureProfile* figures=nullptr,OriginalFigureRenderSource figureSource=OriginalFigureRenderSource::Saved);
    OriginalBuildingSceneView renderSavedBuildings(const OriginalMapCamera&,int bit=-1) const;
    bool available() const { return state_.has_value(); }
    const std::string& error() const { return error_; }
    MapImageGridResult render(const OriginalMapCamera&) const;
    AppSurfacePreviewResult renderPreview(const OriginalMapCamera&) const;
    AppSurfaceDiagnostics diagnostics(const MapImageGridStats&) const;
private:
    bool adoptState(const OriginalMapTerrain&,std::optional<OriginalTerrainState>);
    struct Archive { SG3Archive index; std::string companion; };
    ResourceRoot root_;
    OriginalMapTerrain terrain_;
    std::optional<OriginalTerrainState> state_;
    std::optional<OriginalBuildingScene> savedBuildings_;
    std::map<uint32_t, Archive> archives_;
    std::string error_;
};
}
