// SPDX-License-Identifier: MIT
#pragma once
#include "../map_reference.hpp"
#include "../resource_root.hpp"
#include <map>
#include <optional>

namespace game_lab {
// App adapter for the existing stored-grid renderer. No terrain-to-image
// reconstruction, original-screen registration, or authored gameplay is used.
class AppMapSurface {
public:
    explicit AppMapSurface(ResourceRoot root) : root_(std::move(root)) {}
    void clear();
    bool select(const std::string& relativeMap, const OriginalMapTerrain& expected);
    bool available() const { return state_.has_value(); }
    const std::string& error() const { return error_; }
    MapImageGridResult render(const OriginalMapCamera&) const;
private:
    struct Archive { SG3Archive index; std::string companion; };
    ResourceRoot root_;
    OriginalMapTerrain terrain_;
    std::optional<OriginalTerrainState> state_;
    std::map<uint32_t, Archive> archives_;
    std::string error_;
};
}
