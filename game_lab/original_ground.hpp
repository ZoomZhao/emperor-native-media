#pragma once
#include "emperor_assets.hpp"
#include "original_terrain.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace game_lab {
// Per-cell request; every field comes from MAP bytes the native loader already parses.
struct GroundCodeRequest {
    uint32_t terrainFlags = 0; uint8_t randomByte = 0;
    // The 8 surrounding terrain flags in the executable's read order
    // N,NE,E,SE,S,SW,W,NW (FUN_004B9CC0 @0x004B9CC0 row-shifted views 0x390 =
    // 228 cells * 4 apart). hasNeighbours is false for a caller that only has the
    // centre cell; the shore branch then stays known=false instead of guessing.
    bool hasNeighbours = false;
    std::array<uint32_t,8> neighbourFlags{};
    // neighbourPresent[i] is true when neighbour i's flat index lies inside the
    // storage grid. The executable's in-map test (FUN_004BB390 @0x004BB390,
    // called by FUN_004BB160 @0x004BB160) skips an out-of-map ring cell instead
    // of reading it, so a caller must exclude an absent neighbour from the
    // ring-complete test rather than treat its zero flag as a real 0.
    std::array<bool,8> neighbourPresent{};
    // The per-cell runtime arrays FUN_0052E7C0 @0x0052E7C0 also restores from the
    // same `.map` payload (see original_terrain.hpp). hasLayers tells a caller that
    // featureFlags/fertility carry real MAP bytes; a request built without them
    // leaves the branches that read those arrays known=false instead of reading a
    // default zero as data. Defaults keep hand-built requests (terrain + random
    // only) valid.
    bool hasLayers = false;
    uint32_t featureFlags = 0;  // 0x00F37DA0 (setter 10, FUN_0053FA20)
    uint8_t fertility = 0;      // 0x00E9F8E0 (setter 17, FUN_004BD8C0)
    // 0x00E92DD0 road-distance byte (setter 5, FUN_004BC6A0 @0x004BC6A0). Valid
    // only when hasLayers; like feature/fertility it is one of the per-cell
    // arrays FUN_0052E7C0 @0x0052E7C0 restores from the same `.map` payload
    // (original_terrain.hpp, offset 885,271). 0xFF selects the road-art arm.
    uint8_t roadDistance = 0;
};
// Original executable per-cell ground dispatch. `code` is meaningful only when
// `known` is true. Branches the executable delegates to subsystems not yet
// modelled set `known=false` and name the branch, so callers measure coverage
// instead of guessing. `chainNoWrite` is a determinate outcome: the chain runs
// but no setter writes 0x00FE9880 for this cell. The oracle refutes calling that
// "no ground": 0x00FE9880 is loaded from the `.map` payload (logical offset
// 1535, original_terrain.hpp), not computed, so a cell the chain does not write
// keeps the art it was loaded with. Measured over all 167 Cities/*.map:
// chainNoWrite vs a stored zero == 0 cells, vs a stored nonzero == 114,937.
struct GroundSelection {
    uint16_t code = 0;      // 0x600|ordinal for tileset 3, 0x400|n for tileset 2
    bool known = false;
    bool block = false;     // the executable draws w x h block art for this cell
    const char* branch = "";// stable identifier, e.g. "plain-land"
    bool chainNoWrite = false; // determinate: no setter writes 0x00FE9880
    // The setter stores FUN_00408170(code) + variant (0x00FE9880), so `code` stays
    // the pure base code and the per-cell offset travels here in the record/id
    // space. It is *not* folded into the code's low 9 bits, which name a header slot.
    uint8_t variant = 0;
};
GroundSelection selectGroundTileCode(const GroundCodeRequest&);
// Whole-map reconstruction in FUN_005403C0's row order. A stamp can replace an
// earlier prediction; covered cells retain the stamping branch and origin.
// The stored image/draw/subTile grids are never inputs to block selection.
struct GroundMapCell {
    GroundSelection selection;
    size_t origin = 0;
    uint8_t stampSide = 1;
};
std::vector<GroundMapCell> selectGroundMap(const OriginalTerrainState&,uint32_t activeSide);
// FUN_004B7E80 @0x004B7E80: read-only farm rectangle predicate, in crop-local
// coordinates. Exposed so margin/terrain/neighbour refusals can be tested directly.
bool acceptsFarmGroundBlock(const OriginalTerrainState&,uint32_t activeSide,
    int x,int y,int width,int height);
// Build a request from the MAP-decoded per-cell layers the loader restores (all
// arrays in OriginalTerrainState, FUN_0052E7C0 read order). Cell indexing matches
// OriginalMapTerrain::flags (y*storageSide+x). A neighbour whose flat index falls
// outside the storage grid has no MAP byte and stays 0, matching the executable's
// zeroed edge storage. The request carries featureFlags/fertility and sets
// hasLayers, so the setter-10 feature bits and the setter-17 fertility rank are
// resolvable.
GroundCodeRequest groundCodeRequest(const OriginalTerrainState&, size_t cell);
// code 0x600|k -> header slot k -> 0-based index into SG3Archive::sprites.
// false when the slot is empty (the executable's lookup then yields -1).
bool resolveGroundTileCode(const SG3Archive&, uint16_t code, size_t& archiveIndex);
// Same, then adds the setter's variant in the record/id space:
//   archiveIndex = imageIndex[slot] - 1 + variant
// because the engine's image handle is FUN_00408170(code) + variant and its low
// 14 bits are the 0-based tileset image index. false when the sum is out of range
// for archive.sprites (never clamped).
bool resolveGroundTileCode(const SG3Archive&, uint16_t code, uint8_t variant, size_t& archiveIndex);
}
