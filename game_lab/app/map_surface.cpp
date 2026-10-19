// SPDX-License-Identifier: MIT
#include "map_surface.hpp"
#include "../original_pak.hpp"
#include <filesystem>
#include <set>
#include <sstream>
#include <algorithm>

namespace game_lab {
void AppMapSurface::clear() {
    state_.reset(); terrain_ = {}; error_.clear();
}
bool AppMapSurface::select(const std::string& relativeMap, const OriginalMapTerrain& expected) {
    clear();
    const auto path = root_.resolve(relativeMap);
    if (path.empty() || !expected.valid()) { error_ = "MAP 来源不可用"; return false; }
    const auto map = loadOriginalMapTerrain(path);
    if (map.status != OriginalMapStatus::LoadedTerrain || map.terrain.activeSide != expected.activeSide || map.terrain.flags != expected.flags) {
        error_ = "MAP 来源已变化或活动框不一致"; return false;
    }
    return adoptState(expected,loadOriginalTerrainState(path));
}
bool AppMapSurface::select(const OriginalMapSource& source,const OriginalMapTerrain& expected){
    if(source.kind==OriginalMapSourceKind::MapFile)return select(source.relativePath,expected);
    clear();
    if(source.kind!=OriginalMapSourceKind::PakCity){error_="该来源没有已确认的存储地表工作集";return false;}
    auto selected=loadOriginalMapSource(root_,source);
    if(selected.map.status!=OriginalMapStatus::LoadedTerrain||selected.map.terrain.activeSide!=expected.activeSide||selected.map.terrain.flags!=expected.flags||!selected.surfaceState){
        error_="包内城市地表缺失、来源已变化或活动框不一致";return false;
    }
    return adoptState(expected,std::move(selected.surfaceState));
}
bool AppMapSurface::adoptState(const OriginalMapTerrain& expected,std::optional<OriginalTerrainState> state){
    if (!expected.valid() || !state || !state->valid() || state->terrain != expected.flags) {
        error_ = "MAP 存储地表与当前地形来源不一致或数据不完整"; return false;
    }
    std::set<uint32_t> required;
    for (const auto word : state->image) if (word) required.insert(word >> 14);
    for (const auto slot : required) {
        if (archives_.count(slot)) continue;
        const auto leaf = mapImageGridArchiveName(slot);
        if (!leaf) continue; // Renderer counts unavailable namespaces explicitly.
        const std::filesystem::path relative = std::filesystem::path("DATA") / leaf;
        const auto archivePath = root_.resolve(relative.generic_string());
        if (archivePath.empty()) continue;
        auto index = loadSG3(archivePath);
        if (index.version != 213 || !index.numImages || index.numImages != index.sprites.size()) continue;
        auto companion = relative; companion.replace_extension(".555");
        archives_.emplace(slot, Archive{std::move(index), root_.resolve(companion.generic_string())});
    }
    terrain_ = expected; state_ = std::move(state); return true;
}
MapImageGridResult AppMapSurface::render(const OriginalMapCamera& camera) const {
    if (!state_) return {};
    std::vector<MapImageGridTileset> tilesets;
    for (const auto& entry : archives_) tilesets.push_back({entry.first, &entry.second.index, entry.second.companion, 201});
    auto pixels = camera; pixels.outlines = false;
    return renderMapImageGridViewport(terrain_, *state_, tilesets, pixels);
}
AppSurfaceDiagnostics AppMapSurface::diagnostics(const MapImageGridStats& stats) const {
    AppSurfaceDiagnostics out;
    out.totalUnknown=stats.geometryUnsupported+stats.archiveUnavailable+stats.unresolvedIndices+stats.decodeFailures;
    out.unlocated=out.totalUnknown;
    if(!state_||!out.totalUnknown)return out;
    const auto activity=analyzeOriginalMap(terrain_);
    if(!activity.cropValid())return out;
    std::map<uint32_t,uint32_t> geometry;
    for(const auto& mismatch:stats.compositeMismatches){
        if(mismatch.recordId<201)continue;
        const std::string reason=mismatch.reason;
        if(reason=="subTile group side != record cols"||reason=="record does not declare an s*s block")
            geometry.emplace((mismatch.tileset<<14)|(mismatch.recordId-201),mismatch.cols);
    }
    size_t locatedGeometry=0,locatedArchive=0,locatedIndex=0;
    for(uint32_t y=0;y<activity.cropSide;++y)for(uint32_t x=0;x<activity.cropSide;++x){
        const size_t cell=size_t(y+activity.cropOrigin)*228+x+activity.cropOrigin;
        if(terrain_.flags[cell]&0x80000u)continue;
        const auto word=state_->image[cell];if(!word)continue;
        const auto slot=word>>14;const auto archive=archives_.find(slot);
        AppSurfaceIssue issue{x,y,word,0,uint32_t(state_->subTile[cell]&15)+1,AppSurfaceIssueKind::BlockGeometryUnsupported};
        if(archive==archives_.end()){issue.kind=AppSurfaceIssueKind::ArchiveUnavailable;++locatedArchive;}
        else if(size_t((word&0x3fffu)+201)>archive->second.index.numImages){issue.kind=AppSurfaceIssueKind::IndexUnresolved;++locatedIndex;}
        else{
            const auto found=geometry.find(word);if(found==geometry.end())continue;
            issue.recordSide=found->second;
            if(issue.recordSide){
                if(issue.groupSide==issue.recordSide)continue; // same-record accepted cells stay absent
                issue.kind=AppSurfaceIssueKind::GroupSideConflict;
            }
            ++locatedGeometry;
        }
        out.cells.push_back(issue);
    }
    // A newer/unrecognised renderer reason must not acquire guessed coordinates.
    // Decode failures and other geometry remain explicitly unlocated.
    if(locatedGeometry>stats.geometryUnsupported||locatedArchive>stats.archiveUnavailable||locatedIndex>stats.unresolvedIndices){
        out.cells.clear();return out;
    }
    out.unlocated=out.totalUnknown-out.cells.size();return out;
}
std::string describeAppSurfaceDiagnostics(const AppSurfaceDiagnostics& diagnostics,size_t maxRows){
    std::ostringstream text;
    text<<"未覆盖 "<<diagnostics.totalUnknown<<" 格；已定位 "<<diagnostics.cells.size()<<" 格；未定位 "<<diagnostics.unlocated<<" 格。";
    if(!diagnostics.totalUnknown){text<<"\n本源非零存储格无遗漏；这不是完整原版画面对照。";return text.str();}
    text<<"\n下列为0基活动格坐标；将 X/Y 输入相机可定位。分类不证明原版对象语义或屏幕锚点。";
    const size_t shown=std::min(maxRows,diagnostics.cells.size());
    for(size_t i=0;i<shown;++i){const auto& issue=diagnostics.cells[i];
        const char* label=issue.kind==AppSurfaceIssueKind::GroupSideConflict?"组边长冲突":
            issue.kind==AppSurfaceIssueKind::BlockGeometryUnsupported?"不符合当前块几何":
            issue.kind==AppSurfaceIssueKind::ArchiveUnavailable?"缺少图包":"索引未解析";
        text<<"\n活动("<<issue.x<<','<<issue.y<<") · "<<label<<" · 图包 "<<(issue.storedWord>>14)<<" / 图像 "<<((issue.storedWord&0x3fffu)+201);
        if(issue.kind==AppSurfaceIssueKind::GroupSideConflict)text<<" · 存储组边长 "<<issue.groupSide<<" / 图像块边长 "<<issue.recordSide;
    }
    if(shown<diagnostics.cells.size())text<<"\n显示前 "<<shown<<'/'<<diagnostics.cells.size()<<" 格（列表预算上限）；其余仍未覆盖。";
    if(diagnostics.unlocated)text<<"\n另有 "<<diagnostics.unlocated<<" 格未定位（含解码失败或尚不支持的几何原因）；不能把它们当作已覆盖。";
    return text.str();
}
}
