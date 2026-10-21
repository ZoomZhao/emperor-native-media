#include "original_city_clocks.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using game_lab::OriginalCityClocks;
constexpr size_t kSlots = 10;
size_t checks = 0;
void need(bool ok, const char* message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
}
uint32_t period(size_t i) { return game_lab::kOriginalCityClockPeriod[i]; }
// Straightforward re-derivation from one stored value, written independently of the
// port: increment, and clear only when the incremented value reached the period.
uint32_t derivedNext(uint32_t value, uint32_t p) {
    const uint32_t next = value + 1u;
    return next >= p ? 0u : next;
}

void constantTests() {
    const std::array<uint32_t, kSlots> periods{4, 8, 16, 24, 32, 64, 128, 256, 384, 768};
    need(game_lab::kOriginalCityClockPeriod == periods,
         "period table equals the ten cmp literals 0x529998..0x529A64");
    for (size_t i = 0; i < kSlots; ++i)
        need(game_lab::kOriginalCityClockAddress[i] == 0x010C71F8u - uint32_t(4 * i),
             "counter globals descend by 4 from 0x10C71F8 to 0x10C71D4");
    need(sizeof(OriginalCityClocks) == 40, "ten u32 globals with no padding");
    OriginalCityClocks fresh;
    for (uint32_t v : fresh.value) need(v == 0, "aggregate value-initialises to zero");
}

// Each slot must wrap on exactly the advance whose incremented value equals the period.
void wrapTests() {
    for (size_t i = 0; i < kSlots; ++i) {
        const uint32_t p = period(i);
        OriginalCityClocks c;
        c.value[i] = p - 1;
        advanceOriginalCityClocks(c);
        need(c.value[i] == 0, "slot clears on the single advance that reaches its period");
        c.value[i] = p - 2;
        advanceOriginalCityClocks(c);
        need(c.value[i] == p - 1, "slot holds period-1 after starting at period-2");
        advanceOriginalCityClocks(c);
        need(c.value[i] == 0, "period-1 then advance is 0, so the period is exact");
        c.value[i] = p;
        advanceOriginalCityClocks(c);
        need(c.value[i] == 0, "a stored value equal to the period also clears");
        c.value[i] = p + 5;
        advanceOriginalCityClocks(c);
        need(c.value[i] == 0, "a stored value above the period also clears");
        c.value[i] = 0xFFFFFFFFu;
        advanceOriginalCityClocks(c);
        need(c.value[i] == 0, "u32 overflow to 0 is below every period and stays 0");
        OriginalCityClocks d;
        std::vector<bool> seen(p, false);
        for (uint32_t step = 0; step < p; ++step) {
            advanceOriginalCityClocks(d);
            need(d.value[i] == (step + 1) % p, "slot walks 0..period-1 in order from zero");
            need(!seen.at(d.value[i]), "each residue appears exactly once per period");
            seen.at(d.value[i]) = true;
        }
        need(d.value[i] == 0, "exactly period advances return the slot to 0");
        advanceOriginalCityClocks(d);
        need(d.value[i] == 1, "the reset is not absorbing: 0 advances to 1 again");
    }
}

// The exe stores slot 0 (0x10C71F8) first and slot 9 (0x10C71D4) last; the ten globals
// are disjoint, so no observer can see that order.
void orderTests() {
    const std::array<std::array<size_t, kSlots>, 4> orders{{
        {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}},
        {{9, 8, 7, 6, 5, 4, 3, 2, 1, 0}},
        {{5, 0, 9, 3, 7, 1, 8, 2, 6, 4}},
        {{4, 6, 2, 8, 0, 5, 1, 9, 3, 7}},
    }};
    for (const auto& order : orders) {
        OriginalCityClocks seed;
        for (size_t i = 0; i < kSlots; ++i) seed.value[i] = (uint32_t(i) * 37u + 5u) % period(i);
        OriginalCityClocks port = seed, manual = seed;
        for (unsigned step = 0; step < 7; ++step) {
            advanceOriginalCityClocks(port);
            for (size_t i : order) manual.value[i] = derivedNext(manual.value[i], period(i));
            need(port.value == manual.value,
                 "per-slot increment and reset are independent of the ten-slot order");
        }
    }
    OriginalCityClocks base;
    for (size_t i = 0; i < kSlots; ++i) base.value[i] = (uint32_t(i) * 11u + 3u) % period(i);
    for (unsigned step = 0; step < 50; ++step) {
        OriginalCityClocks joint = base;
        advanceOriginalCityClocks(joint);
        for (size_t i = 0; i < kSlots; ++i) {
            OriginalCityClocks single;
            single.value[i] = base.value[i];
            advanceOriginalCityClocks(single);
            need(single.value[i] == joint.value[i],
                 "a slot trajectory depends only on its own stored value");
        }
        base = joint;
    }
}

void truthTableTests() {
    OriginalCityClocks state;
    std::array<uint32_t, kSlots> reference{};
    for (size_t i = 0; i < kSlots; ++i) {
        const uint32_t p = period(i);
        // Slot 9 starts out of range on purpose; it still clears to 0 on the first step.
        state.value[i] = i == 9 ? p + 5u : (uint32_t(i) * 97u) % p;
        reference[i] = state.value[i];
    }
    for (unsigned step = 0; step < 1000; ++step) {
        for (size_t i = 0; i < kSlots; ++i) reference[i] = derivedNext(reference[i], period(i));
        advanceOriginalCityClocks(state);
        need(state.value == reference, "1000-step table matches per-slot re-derivation");
    }
    for (size_t i = 0; i < kSlots; ++i)
        need(state.value[i] < period(i), "stored value always stays below its period");
}
}  // namespace

int main() {
    try {
        constantTests();
        wrapTests();
        orderTests();
        truthTableTests();
        std::cout << "{\"passed\":true,\"authored_checks\":" << checks << "}\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
