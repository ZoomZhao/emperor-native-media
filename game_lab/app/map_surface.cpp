// SPDX-License-Identifier: MIT
#include "map_surface.hpp"
#include <filesystem>
#include <set>

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
    auto state = loadOriginalTerrainState(path);
    if (!state || !state->valid() || state->terrain != expected.flags) {
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
}
