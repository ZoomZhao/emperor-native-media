#pragma once
#include <cstdint>
#include <vector>
namespace game_lab {
// Bit-exact reimplementation of the original executable's per-cell random byte grid
// 0x00F1E780 (228*228 = 51,984 bytes) in Emperor[CH].exe, SHA-256
// 0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658.
//
// VERIFIED from the executable (offsets are VAs, imagebase 0x400000):
//  * Generator FUN_004189B0 @0x004189B0: two 31-bit right-shifting LFSRs in
//    0x010C714C / 0x010C7148. One step is
//        bit30 <- bit4 XOR bit0; state >>= 1
//    (0x004189EB-0x004189F8 for 0x010C714C, 0x004189FA-0x00418A07 for 0x010C7148,
//     0x00418A09-0x00418A1B is the shift+or, 0x00418A21-0x00418A22 the 31-iteration
//     loop). A call performs exactly 31 steps (edi=0x1F, 0x004189E6/0x00418A22) and
//     then publishes, for the first LFSR, state&0x7FFF at 0x010C7138 (0x00418A28/
//     0x00418A36), state&0x7F at 0x010C713C (0x00418A2D/0x00418A3B) and state&7 at
//     0x010C7140 (0x00418A41/0x00418A4E), plus the same three fields for the second
//     LFSR at 0x010C712C/0x010C7130/0x010C7134 (0x00418A54-0x00418A6C).
//     The call also pushes the previous state&0x7F into a 100-entry ring
//     0x010C6F90[0x010C6F8C] (0x004189B1-0x004189CF); that ring has no reader in
//     .text, so it is recorded here but not modelled.
//  * Fill loop FUN_004B0AC0 @0x004B0AC0: 228 outer * 228 inner iterations
//     (ebx=edi=0xE4 at 0x004B0AC5/0x004B0ACA, counters at 0x004B0AE0/0x004B0AE4),
//     one FUN_004189B0 call per byte (0x004B0ACF) and one flat sequential store
//     grid[esi++]=byte[0x010C7138] (0x004B0AD4-0x004B0ADF). So the grid is 51,984
//     consecutive draws, low 8 bits of state&0x7FFF, in cell order 0..51983.
//  * Seeds: FUN_0052B590 @0x0052B590 (the WinMain routine, window class registration)
//     writes 0x010C714C=0x54657687, 0x010C7148=0x72641663, 0x010C7144=0x34518632 at
//     process start (0x0052B5F8-0x0052B60C). They are immediates: no file byte, clock
//     or rand()/time() is involved. 0x010C7144 is never read anywhere in .text.
//  * FUN_004B08A0 @0x004B08A0 zeroes the grid (0x004B0908) and calls FUN_004B0AC0
//     (0x004B0A5D); the grid is regenerated at scenario/game start, and on load it is
//     overwritten by the save stream (FUN_0052FDA0 @0x0052FDA0 reads 0xCB10 bytes to
//     0x00F1E780 through FUN_00780533; FUN_0052E7C0 @0x0052E7C0 writes it through
//     FUN_00780642). FUN_00529A80 @0x00529A80 additionally burns 100 draws when a
//     new game starts.
//
// NOT ESTABLISHED: the stream index at the moment of the fill. Real supplied saves
// contain the whole grid as one run of exactly 51,984 consecutive draws starting at
// stream index 2341, 61470 and 104778 in different files, so the grid is a function of
// (seed, stream index), not a pure function of the cell index; no cellRandomGrid() is
// offered. streamIndex positions the generator so a caller can reproduce a real fill.
class OriginalCellRandom {
public:
    // streamIndex is the number of draws already consumed from the process-start state;
    // 0 is the executable's initial state (FUN_0052B590 @0x0052B590).
    explicit OriginalCellRandom(uint64_t streamIndex=0);
    void reset();
    uint64_t streamIndex() const;
    void advance(uint64_t draws);
    // One FUN_004189B0 @0x004189B0 call: 31 LFSR steps on both registers, then the
    // grid byte = low 8 bits of 0x010C7138 (state&0x7FFF) as read at 0x004B0AD4.
    uint8_t next();
    // The second LFSR's published 0x010C712C field (state&0x7FFF) for the same call;
    // captured by FUN_004189B0 @0x00418A54 but not used by the grid.
    uint32_t secondField() const;
    uint32_t rawState() const;
    uint32_t rawSecondState() const;
    // FUN_004B0AC0 @0x004B0AC0 order and length: 228*228 sequential next() draws.
    std::vector<uint8_t> fillGrid();
private:
    static constexpr uint32_t kSeed=0x54657687;      // FUN_0052B590 @0x0052B5F8
    static constexpr uint32_t kSecondSeed=0x72641663;// FUN_0052B590 @0x0052B602
    uint32_t state_=kSeed;
    uint32_t second_=kSecondSeed;
    uint64_t draws_=0;
};
constexpr size_t kOriginalGridSide=228;              // 0xE4 in FUN_004B0AC0 @0x004B0AC5
constexpr size_t kOriginalGridCells=kOriginalGridSide*kOriginalGridSide;
}
