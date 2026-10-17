#pragma once
#include "emperor_assets.hpp"
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
};
// Original executable per-cell ground dispatch. `code` is meaningful only when
// `known` is true. Branches the executable delegates to subsystems not yet
// modelled set `known=false` and name the branch, so callers measure coverage
// instead of guessing. `noGround` is a determinate outcome: the chain runs but
// no setter writes 0x00FE9880 for this cell, so there is provably no ground code.
struct GroundSelection {
    uint16_t code = 0;      // 0x600|ordinal for tileset 3, 0x400|n for tileset 2
    bool known = false;
    bool block = false;     // the executable draws w x h block art for this cell
    const char* branch = "";// stable identifier, e.g. "plain-land"
    bool noGround = false;  // determinate: the cell is left without a ground code
    // The setter stores FUN_00408170(code) + variant (0x00FE9880), so `code` stays
    // the pure base code and the per-cell offset travels here in the record/id
    // space. It is *not* folded into the code's low 9 bits, which name a header slot.
    uint8_t variant = 0;
};
GroundSelection selectGroundTileCode(const GroundCodeRequest&);
// Build a request from the MAP-derived grids the native loader already parses.
// Cell indexing matches OriginalMapTerrain::flags (y*storageSide+x). A neighbour
// whose flat index falls outside the storage grid has no MAP byte and stays 0,
// matching the executable's zeroed edge storage.
GroundCodeRequest groundCodeRequest(const std::vector<uint32_t>& terrainFlags,
    const std::vector<uint8_t>& randomGrid, size_t cell);
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
