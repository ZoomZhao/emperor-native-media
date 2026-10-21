#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace game_lab {
// All executable facts: Emperor[CH].exe SHA-256
// 0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658, image base 0x400000.
// VA-to-file offset is VA-0x400000. See docs/ORIGINAL_TICK_WIRING_AUDIT.md
// "City modulo clocks" for the verified leaf map.
//
// VERIFIED from the executable:
//  * The leaf FUN_00529990 runs 0x00529990..0x00529A75 and returns at 0x00529A76.
//    0x005371A0 calls it second (call site 0x005371A5, after FUN_004189B0 at
//    0x005371A0), once per admitted city tick.
//  * Each of the ten slots has the same four-instruction shape against a distinct
//    absolute global: load u32, inc, store the incremented value, then compare the
//    loaded-plus-one value against the period and, only when it is >= the period,
//    store 0. The store happens before the comparison (0x0052999B for slot 0,
//    0x00529A69 for slot 9), so the reset is the final write of the slot.
//    Net effect: next = previous + 1; value = next >= period ? 0 : next. A stored
//    value above the period (never produced by this leaf) also collapses to 0.
//  * The ten slots are advanced in descending address order and touch disjoint
//    globals, so the order is unobservable; slots are independent of each other.
//
// Periods below are listed in exe advance order, each with its counter global and
// the address of that slot's reset ("store 0") instruction:
//   index 0: 0x10C71F8 period   4   load 0x00529990, cmp 0x00529998, store 0x0052999B, reset 0x005299A2
//   index 1: 0x10C71F4 period   8   load 0x005299A8, cmp 0x005299AE, store 0x005299B1, reset 0x005299B8
//   index 2: 0x10C71F0 period  16   load 0x005299BE, cmp 0x005299C4, store 0x005299C7, reset 0x005299CE
//   index 3: 0x10C71EC period  24   load 0x005299D4, cmp 0x005299DA, store 0x005299DD, reset 0x005299E4
//   index 4: 0x10C71E8 period  32   load 0x005299EA, cmp 0x005299F0, store 0x005299F3, reset 0x005299FA
//   index 5: 0x10C71E4 period  64   load 0x00529A00, cmp 0x00529A06, store 0x00529A09, reset 0x00529A10
//   index 6: 0x10C71E0 period 128   load 0x00529A16, cmp 0x00529A1C, store 0x00529A21, reset 0x00529A28
//   index 7: 0x10C71DC period 256   load 0x00529A2E, cmp 0x00529A34, store 0x00529A39, reset 0x00529A40
//   index 8: 0x10C71D8 period 384   load 0x00529A46, cmp 0x00529A4C, store 0x00529A51, reset 0x00529A58
//   index 9: 0x10C71D4 period 768   load 0x00529A5E, cmp 0x00529A64, store 0x00529A69, reset 0x00529A70
// The ten globals are the contiguous dword array 0x10C71D4..0x10C71F8 (order 9..0).
//
// These globals are NOT restored by the SAV19 branch 0x005311DC..0x00531873: none
// of the ten addresses appears in any read request there, and each occurs exactly
// three times in the whole listing, all inside FUN_00529990. A loaded save
// therefore cannot supply them; a native city tick must start them explicitly.
struct OriginalCityClocks {
    // Index order matches the exe advance order (0x10C71F8 first, 0x10C71D4 last).
    std::array<uint32_t, 10> value{};
};

// Counter globals in exe advance order (descending address, 0x10C71D4..0x10C71F8).
inline constexpr std::array<uint32_t, 10> kOriginalCityClockAddress{
    0x010C71F8u, 0x010C71F4u, 0x010C71F0u, 0x010C71ECu, 0x010C71E8u,
    0x010C71E4u, 0x010C71E0u, 0x010C71DCu, 0x010C71D8u, 0x010C71D4u};
// Periods in the same order; each is the literal cmp operand of its slot.
inline constexpr std::array<uint32_t, 10> kOriginalCityClockPeriod{
    4u, 8u, 16u, 24u, 32u, 64u, 128u, 256u, 384u, 768u};

// One call is one 0x00529990 call: ten independent "increment, then clear when the
// incremented value reached the period" clocks.
inline void advanceOriginalCityClocks(OriginalCityClocks& clocks) {
    for (size_t i = 0; i < clocks.value.size(); ++i) {
        const uint32_t next = clocks.value[i] + 1u;  // inc eax; store back
        clocks.value[i] = next;                      // store before the period test
        if (next >= kOriginalCityClockPeriod[i]) clocks.value[i] = 0u;  // reset store
    }
}
}  // namespace game_lab
