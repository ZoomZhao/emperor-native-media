// SPDX-License-Identifier: MIT
#pragma once
#include "../original_scene_timeline.hpp"
#include "../map_reference.hpp"
#include "../original_building_scene.hpp"
#include "../resource_root.hpp"
#include "city_notifications.hpp"
#include "../original_world_snapshot.hpp"
#include <map>
#include <memory>
#include <optional>
#include <functional>

namespace game_lab {
struct OriginalMapSource;
// The native transaction must reject failures in either rendered layer.
inline bool nativeSceneResourcesReady(const OriginalBuildingSceneView& view,size_t expectedObjects){
    return view.image.valid()&&view.placedRecords==expectedObjects&&!view.withheldRecords&&
        !view.bases.decodeFailures&&!view.overlayDecodeFailures&&!view.overlayInvalid&&!view.groundUnresolved;
}

inline bool nativeSceneReady(const OriginalBuildingSceneView& view,size_t expectedObjects){return !view.pixelsDeferred&&nativeSceneResourcesReady(view,expectedObjects);}

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
// One step of an explicitly premised combined timeline driven from the saved
// snapshot. The App owns every non-SAV input; this adapter supplies only the
// draw lists and the resource root. A refused frame never installs a partial
// prefix: the saved or last admitted scene stays renderable.
// Documented process lifetime state (SAV.md runtime-source table). A load
// replaces saved city state but never reinitializes these values. CRT seed is
// supplied once by the App; no unresolved event/scheduler field lives here.

struct AppCityTimelineStep {
    bool complete=false;
    unsigned updates=0;
    unsigned refusedUpdate=0;
    std::string refusal;
    size_t decodeFailures=0;
};
// Explicit preview convention,not a recovered original painter/anchor rule.
// The strict renderer and its metrics remain unchanged.
AppSurfacePreviewResult renderMapSurfaceCandidateViewport(const OriginalMapTerrain&,
    const OriginalTerrainState&,const std::vector<MapImageGridTileset>&,const OriginalMapCamera&);
// App adapter for the existing image-grid renderer. SAV optionally reconstructs
// ground from its persisted inputs; no screen registration or gameplay is used.
class AppMapSurface {
public:
    explicit AppMapSurface(ResourceRoot root,std::shared_ptr<SG3SpriteCache> spriteCache={});
    ~AppMapSurface();
    AppMapSurface(const AppMapSurface&)=delete;
    AppMapSurface& operator=(const AppMapSurface&)=delete;
    void clear();
    bool select(const std::string& relativeMap, const OriginalMapTerrain& expected);
    bool select(const OriginalMapSource&,const OriginalMapTerrain& expected);
    bool selectSavedBuildings(const OriginalMapTerrain&,const OriginalBuildingProfile&,const OriginalSavedGround* ground=nullptr,uint64_t tickMs=0,const OriginalFigureProfile* figures=nullptr,OriginalFigureRenderSource figureSource=OriginalFigureRenderSource::Saved,const OriginalFigureOrderedPass* ordered=nullptr);
    bool selectPostLoadK1(const OriginalSaveResult&,const std::vector<uint8_t>& decodedPayload);
    bool selectPostLoadTimeline(const OriginalSaveResult&,const std::vector<uint8_t>& decodedPayload,
        const std::vector<OriginalSceneTimelineFrame>&,uint32_t previousAdmission,OriginalAnimationClock);
    // SAV playback carries only the current scene and process state by default.
    // Complete frame history is opt-in for bounded replay checks; normal
    // continuous playback must not retain per-frame draw-cell arrays.
    bool prepareCityPlayback(const OriginalSaveResult&,const std::vector<uint8_t>& decodedPayload,
        const OriginalCombinedProcessInputs&,const AppCitySessionState* session=nullptr,bool recordReplay=false);
    AppCityTimelineStep advanceCityPlayback(const OriginalSceneTimelineFrame&);
    bool prepareBoundCityPlayback(const OriginalWorldBoundSource&,const OriginalWorldPolicy&,const AppCitySessionState&,bool replay=false);
    std::optional<OriginalWorldSnapshot> worldSnapshot(std::string& error)const;
    bool restoreWorldSnapshot(const OriginalWorldSnapshot&,std::string& error);
    uint64_t worldRevision()const;

    // Opaque complete in-memory checkpoint. It owns mutable state and resource owners;
    // never interpreted as an original SAV or an ENCity version.
    struct CityCheckpoint;
    std::shared_ptr<const CityCheckpoint> cityCheckpoint() const;
    bool restoreCityCheckpoint(const CityCheckpoint&,uint64_t expectedSourceRevision);
    std::optional<OriginalKernelHandle> cityFigureHandle(size_t slot) const;
    bool cityFigureHandleCurrent(const OriginalKernelHandle&) const;
    // Diagnostic fault seam: can reject/throw, never approve unknown gameplay.
    void setCityPublicationProbe(std::function<void(const char*)> p){publicationProbe_=std::move(p);}
    bool cityPlaybackPrepared() const { return playback_!=nullptr; }
    unsigned cityPlaybackUpdates() const;
    const AppCityNotifications* cityNotifications() const;
    std::optional<AppCityTributeFinances> cityTributeFinances() const;
    AppCityTributeResult respondCityNotification(AppCityTributeId,AppCityTributeDecision);
    std::optional<AppCitySessionState> cityPlaybackSession() const;
    const std::string& cityPlaybackError() const;
    void clearCityPlayback();
    std::vector<OriginalSceneTimelineFrame> cityPlaybackReplayFrames() const;
    // FNV-1a over every installed placement (binding, draw position,
    // components), overlay and terrain flag word: the whole scene, not a viewport.
    uint64_t installedSceneDigest() const;
    OriginalBuildingSceneView renderSavedBuildings(const OriginalMapCamera&,int bit=-1,SceneDrawCapture* capture=nullptr,bool deferRaster=false) const;
    bool available() const { return state_.has_value(); }
    const std::string& error() const { return error_; }
    MapImageGridResult render(const OriginalMapCamera&) const;
    AppSurfacePreviewResult renderPreview(const OriginalMapCamera&) const;
    AppSurfaceDiagnostics diagnostics(const MapImageGridStats&) const;
private:
    bool prepareCityPlaybackCandidate(const OriginalSaveResult&,const std::vector<uint8_t>&,const OriginalCombinedProcessInputs&,const AppCitySessionState*,bool);
    AppCityTimelineStep advanceCityPlaybackCandidate(const OriginalSceneTimelineFrame&);
    void swapPrepared(AppMapSurface&) noexcept;
    bool adoptState(const OriginalMapTerrain&,std::optional<OriginalTerrainState>);
    struct Archive { SG3Archive index; std::string companion; };
    struct Playback;
    // Post-load figure inputs shared by the one-shot timeline and the
    // replayable playback. Model/scratch premises are the documented
    // fresh-process values already declared by selectPostLoadTimeline.
    bool preparePostLoadInputs(const OriginalSaveResult&,const std::vector<uint8_t>&,
        std::map<uint32_t,SG3Archive>&,std::map<uint32_t,const SG3Archive*>&,
        OriginalFigureUpdateContext&,OriginalBuildingEmployment&,std::string&);
    ResourceRoot root_;
    std::shared_ptr<SG3SpriteCache> spriteCache_;
    OriginalMapTerrain terrain_;
    std::optional<OriginalTerrainState> state_;
    std::optional<OriginalBuildingScene> savedBuildings_;
    std::map<uint32_t, Archive> archives_;
    std::unique_ptr<Playback> playback_;
    mutable std::optional<OriginalMapCamera> lastPlaybackCamera_;
    std::function<void(const char*)> publicationProbe_;
    std::string error_;
};
}
