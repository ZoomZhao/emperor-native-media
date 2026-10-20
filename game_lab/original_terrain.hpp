#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace game_lab {
// Per-cell working set that the original city-map loader publishes from the raw
// `.map` logical payload. Extracted (authorised clean-room, AGENTS.md 2026-10-17)
// from the user's legally owned Emperor[CH].exe, SHA-256
// 0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658
// (4,820,992 bytes, PE32 i386, imagebase 0x400000). Only offsets and sizes are
// recorded; no listing, binary or asset is stored.
//
// Evidence: the read branch of FUN_0052E7C0 @0x0052E7C0, entered when
// FUN_0040CF90()==0 at 0x0052E80A (`je 0x52E97C`). Every grid is a
// `push size; push <global>; mov ecx,esi(stream); call FUN_00780533 @0x00780533`
// (stream -> buffer) pair at 0x0052E9E5..0x0052EB3F, in the order below. The
// sibling write branch (0x0052E835..0x0052E977) pushes the same globals/sizes
// through FUN_00780642 @0x00780642, which pins each pair to one field. A bare city
// load reaches this read path through FUN_00534780 @0x00534780
// (`push 0; push 0; push stream; call 0x52E7C0`) and FUN_00534660 @0x00534660.
struct OriginalTerrainState {
    static constexpr size_t storageSide=228;
    static constexpr size_t cells=storageSide*storageSide; // 51,984 = 0xCB10
    static constexpr size_t words=cells*4;                 // 207,936 = 0x32C40
    // Flat logical-payload byte offsets, in the loader's own read order. The first
    // grid starts immediately after the 1,535-byte city header.
    static constexpr size_t imageOffset=1535,        // 0x00FE9880 u32 words
        drawOffset=209471,                            // 0x00FDCD70 u8  cells
        terrainOffset=261455,                         // 0x00F6A9E0 u32 words
        featureOffset=469391,                         // 0x00F37DA0 u32 words
        subTileOffset=677327,                         // 0x00F9D620 u8  cells
        randomByteOffset=729311,                      // 0x00F1E780 u8  cells
        gF05160Offset=781295,                         // 0x00F05160 u8  cells
        g101D0ACOffset=833279,                        // 0x0101D0AC u32
        g101D0B0Offset=833283,                        // 0x0101D0B0 u32
        gEAC3F0Offset=833287,                         // 0x00EAC3F0 u8  cells
        roadDistanceOffset=885271,                    // 0x00E92DD0 u8  cells
        fertilityOffset=937255,                       // 0x00E9F8E0 u8  cells
        blob24Offset=989239,                          // 0x24-byte block
        gBEBF3AOffset=989275,                         // 0x00BEBF3A u8  cells
        gBF8A4AOffset=1041259,                        // 0x00BF8A4A u8  cells
        g10C6F18Offset=1093243,                       // 0x010C6F18 u32
        g10C6DB0Offset=1093247;                       // 0x010C6DB0 0x168 bytes
    static constexpr size_t blob24Size=0x24, g10C6DB0Size=0x168;
    // End of the flat run; the loader then deserialises the object pools
    // (FUN_0042D250 @0x0042D250 / FUN_0042D790 @0x0042D790, targets 0x008C7630 and
    // 0x0115D4F0) and, for map version >4, one more 0x00F2B290 grid.
    static constexpr size_t flatEnd=g10C6DB0Offset+g10C6DB0Size; // 1,093,607
    // The ground dispatch FUN_0053EC90 @0x0053EC90 consumes terrain, randomByte,
    // feature and fertility; the rest is parsed to keep the read order complete.
    std::vector<uint32_t> image;      // 0x00FE9880 surface image id (chain output)
    std::vector<uint8_t> draw;        // 0x00FDCD70
    std::vector<uint32_t> terrain;    // 0x00F6A9E0 terrain flags (== the MAP field)
    std::vector<uint32_t> feature;    // 0x00F37DA0 per-cell feature/orientation bits
    std::vector<uint8_t> subTile;     // 0x00F9D620
    std::vector<uint8_t> randomByte;  // 0x00F1E780 per-cell random byte
    std::vector<uint8_t> gF05160;     // 0x00F05160
    uint32_t g101D0AC=0,g101D0B0=0;   // 0x0101D0AC / 0x0101D0B0
    std::vector<uint8_t> gEAC3F0;     // 0x00EAC3F0
    std::vector<uint8_t> roadDistance; // 0x00E92DD0
    std::vector<uint8_t> fertility;   // 0x00E9F8E0 (FUN_004BD920 @0x004BD920 gate)
    std::array<uint8_t,blob24Size> blob24{}; // read into FUN_00408C70(0x00890210)
    // Signed height at this+2 of the 0x00BEBF38 elevation object:
    // FUN_00408CF0 @0x00408CF0, caller ECX=0x00BEBF38 at 0x00473225.
    // Restored by the existing loader read at 0x0052EAC4..0x0052EACB.
    std::vector<uint8_t> gBEBF3A;     // 0x00BEBF3A signed height (stored as bytes)
    std::vector<uint8_t> gBF8A4A;     // 0x00BF8A4A
    uint32_t g10C6F18=0;              // 0x010C6F18
    std::array<uint8_t,g10C6DB0Size> g10C6DB0{}; // 0x010C6DB0
    bool valid() const;
};
// Parse the loader's flat read order out of a decoded `.map` logical payload.
// Requires the city signature and every flat field in range; never reads beyond
// flatEnd and never publishes a partial state.
std::optional<OriginalTerrainState> parseOriginalTerrainState(const std::vector<uint8_t>& logicalPayload);
// Same, after an exclusive bounded file read plus the shared compressed-container
// decode (decodeOriginalMapContainer @ game_lab/original_map.hpp).
std::optional<OriginalTerrainState> loadOriginalTerrainState(const std::string& absolutePath);
} // namespace game_lab
