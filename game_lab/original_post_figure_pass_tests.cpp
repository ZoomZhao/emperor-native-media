#include "original_post_figure_pass.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using namespace game_lab;

size_t checks = 0;
void need(bool ok, const char* message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
}

// A registry view with `count` slots (slot 0 is never visited, as in 0x00564B86).
struct Pool {
    std::vector<OriginalPostFigureRegistryObject> objects;
    std::vector<const OriginalPostFigureRegistryObject*> slots;
    explicit Pool(size_t count) : objects(count), slots(count) {
        for (size_t i = 0; i < count; ++i) slots[i] = &objects[i];
    }
};

struct Calls {
    unsigned index = 0, update = 0, step = 0, handle = 0, skip = 0;
};

// Every required session field, explicitly supplied. The two counter initials are
// only read by the state factory; the live values come from the state itself.
OriginalPostFigurePassInputs session(uint8_t gate = 0, uint8_t pause = 0, uint8_t mode = 1,
                                     uint32_t counter = 0, uint32_t period = 30) {
    OriginalPostFigurePassInputs in;
    in.gate0x88EC38 = gate;
    in.pause0x10DE0D0 = pause;
    in.mode0x10DE045 = mode;
    in.counterInitial0x12A6E78 = counter;
    in.periodInitial0x859DA0 = period;
    return in;
}

void allProviders(OriginalPostFigurePassInputs& in, Calls& calls, const void* record,
                  bool stepResult, uint32_t advance) {
    in.indexRecord0x5653C0 = [&calls, record](const OriginalPostFigureRegistryObject&, uint32_t) {
        ++calls.index;
        return record;
    };
    in.updateRecord0x56C880 = [&calls](const void*, uint32_t) { ++calls.update; };
    in.stepRecord0x56D170 = [&calls, stepResult](const void*, uint32_t) {
        ++calls.step;
        return stepResult;
    };
    in.handleTypes76_77_0x564EA0 = [&calls](const OriginalPostFigureRegistryObject&, uint32_t) {
        ++calls.handle;
    };
    in.skipAdvance0x567610 = [&calls, advance](int32_t) {
        ++calls.skip;
        return advance;
    };
}

// ---------------------------------------------------------------------------
// 0x00562F70: 193-entry table, index range, refusal behaviour
// ---------------------------------------------------------------------------
void whitelistTests() {
    need(kOriginalPostFigureWhitelistSize == 193, "table is 193 bytes for types 76..268");
    need(kOriginalPostFigureWhitelistFirstType == 76, "first table index is type 76");
    need(kOriginalPostFigureWhitelistLastType == 268,
         "last table index 192 is type 268");
    need(kOriginalPostFigureWhitelistTableVa == 0x00562F9Cu,
         "table VA 0x00562F9C (the byte read at 0x00562F80 indexes 0x00562F9C)");
    need(kOriginalPostFigureWhitelistTableFileOffset == 0x00162F9Cu,
         "table file offset 0x162F9C = VA-0x400000");
    need(kOriginalPostFigureWhitelistTableFileOffset == kOriginalPostFigureWhitelistTableVa - 0x00400000u,
         "file offset is VA minus the 0x400000 image base");
    need(kOriginalPostFigureWhitelistJumpTableVa == 0x00562F94u,
         "two-entry dispatch table sits at 0x00562F94");

    size_t zeros = 0, ones = 0;
    for (uint8_t value : kOriginalPostFigureWhitelistIndex) {
        need(value == 0 || value == 1, "every table byte is 0 or 1 (two-entry dispatch)");
        if (value == 0) ++zeros;
        if (value == 1) ++ones;
    }
    need(zeros == kOriginalPostFigureWhitelistAcceptedCount, "29 accepting bytes at 0x162F9C");
    need(ones == kOriginalPostFigureWhitelistRejectedCount, "164 rejecting bytes");
    need(zeros + ones == 193, "29 + 164 covers the whole table");
    need(kOriginalPostFigureWhitelistAcceptedCount == 29, "documented accept count is 29");
    need(kOriginalPostFigureWhitelistRejectedCount == 164, "documented reject count is 164");

    // The 193 indices map one-to-one onto types 76..268 and the predicate is exactly
    // "table byte was the accepting dispatch index".
    for (size_t i = 0; i < kOriginalPostFigureWhitelistSize; ++i) {
        const uint32_t type = kOriginalPostFigureWhitelistFirstType + uint32_t(i);
        need(originalPostFigureWhitelisted(type) == (kOriginalPostFigureWhitelistIndex[i] == 0),
             "predicate result equals the stored dispatch index for every type 76..268");
    }

    // Independently stated accept list: 76..86, 92-93, 253..268.
    std::vector<uint32_t> acceptedFromPredicate, acceptedFromList;
    for (uint32_t type = 76; type <= 268; ++type)
        if (originalPostFigureWhitelisted(type)) acceptedFromPredicate.push_back(type);
    for (uint32_t type = 76; type <= 86; ++type) acceptedFromList.push_back(type);
    acceptedFromList.push_back(92);
    acceptedFromList.push_back(93);
    for (uint32_t type = 253; type <= 268; ++type) acceptedFromList.push_back(type);
    need(acceptedFromPredicate == acceptedFromList,
         "accepted set is exactly 76..86, 92, 93, 253..268");
    need(acceptedFromPredicate.size() == 29, "the accepted list has 29 types");

    // Boundaries of each run, plus the refusal side of the range check.
    need(originalPostFigureWhitelisted(76), "76 is accepted (first table byte)");
    need(originalPostFigureWhitelisted(86), "86 is accepted (last of the 11)");
    need(!originalPostFigureWhitelisted(87), "87 is rejected");
    need(!originalPostFigureWhitelisted(91), "91 is rejected");
    need(originalPostFigureWhitelisted(92), "92 is accepted");
    need(originalPostFigureWhitelisted(93), "93 is accepted");
    need(!originalPostFigureWhitelisted(94), "94 is rejected");
    need(!originalPostFigureWhitelisted(252), "252 is rejected");
    need(originalPostFigureWhitelisted(253), "253 is accepted");
    need(originalPostFigureWhitelisted(268), "268 is accepted (last table byte)");
    need(!originalPostFigureWhitelisted(269), "269 is above 192+76 and refused untabled");
    need(!originalPostFigureWhitelisted(75), "75 wraps below the table and is refused");
    need(!originalPostFigureWhitelisted(0), "type 0 is refused before the table");
    need(!originalPostFigureWhitelisted(1), "type 1 is refused before the table");
    need(!originalPostFigureWhitelisted(0x0000FFFFu), "65535 is refused before the table");
    need(!originalPostFigureWhitelisted(0x7FFFFFFFu), "INT32_MAX is refused before the table");
    need(!originalPostFigureWhitelisted(0x80000000u), "INT32_MIN is refused before the table");
    need(!originalPostFigureWhitelisted(0xFFFFFFFFu), "0xFFFFFFFF is refused before the table");
    need(!originalPostFigureWhitelisted(uint32_t(int32_t(-76))), "-76 sign-extended is refused");
    // A rejected type inside the reject runs must not be tabled either.
    need(!originalPostFigureWhitelisted(150), "150 is rejected inside 94..252");
}

// ---------------------------------------------------------------------------
// Counter block: increment at 0x00564B5B, store at 0x00564B5E, signed compare at 0x00564B63
// ---------------------------------------------------------------------------
void counterTests() {
    OriginalPostFigurePassInputs in = session();
    OriginalPostFigurePassState state;
    state.counter = 0;
    state.period = 30;

    // 30 calls hold the counter at 1..30; the 31st increments past the period.
    for (uint32_t tick = 1; tick <= 30; ++tick) {
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::Ok, "an invocation before the period returns ok");
        need(!r.due, "never due while increment <= period");
        need(state.counter == tick, "0x00564B5E stores the increment before the compare");
        need(state.period == 30, "period is untouched on the not-due path");
    }
    auto due = advanceOriginalPostFigurePass(state, in);
    need(due.due, "the 31st invocation exceeds period 30");
    need(due.status == OriginalPostFigurePassStatus::Ok, "a scan with no slots still returns ok");
    need(due.visited == 0, "an empty registry visits no slot");
    need(state.counter == 0, "0x00564C2D resets the counter after the scan");
    need(state.period == 30, "no progress keeps period 30 (0x00564C39)");

    // Trajectory from zero: scans on calls 31, 62, 93 when nothing progresses.
    OriginalPostFigurePassState walk;
    walk.counter = 0;
    walk.period = 30;
    std::vector<uint32_t> scans;
    for (uint32_t tick = 1; tick <= 100; ++tick)
        if (advanceOriginalPostFigurePass(walk, in).due) scans.push_back(tick);
    need(scans == std::vector<uint32_t>({31, 62, 93}),
         "period 30 scans every 31st invocation from a zero counter");

    // A supplied period of 50 is honoured for the first window, but the finish block
    // always rewrites 30 or 50: with no progress the window collapses back to 30.
    OriginalPostFigurePassState slow;
    slow.counter = 0;
    slow.period = 50;
    scans.clear();
    for (uint32_t tick = 1; tick <= 120; ++tick)
        if (advanceOriginalPostFigurePass(slow, in).due) scans.push_back(tick);
    need(scans == std::vector<uint32_t>({51, 82, 113}),
         "period 50 scans at 51, then the no-progress write of 30 makes it 31");

    // 32-bit wrap: 0xFFFFFFFF + 1 == 0, which is <= every ordinary period.
    OriginalPostFigurePassState wrap;
    wrap.counter = 0xFFFFFFFFu;
    wrap.period = 30;
    auto wrapped = advanceOriginalPostFigurePass(wrap, in);
    need(!wrapped.due, "counter wrap to 0 is not due");
    need(wrap.counter == 0, "the wrapped value is stored as is");
    need(wrap.period == 30, "a wrapped counter does not rewrite the period");

    // 0x7FFFFFFF + 1 == 0x80000000 is negative in the signed 0x00564B63 compare.
    OriginalPostFigurePassState signedHigh;
    signedHigh.counter = 0x7FFFFFFFu;
    signedHigh.period = 30;
    auto signedResult = advanceOriginalPostFigurePass(signedHigh, in);
    need(!signedResult.due, "0x80000000 compares as negative under the signed compare");
    need(signedHigh.counter == 0x80000000u, "the signed-negative counter is stored unchanged");

    // Period 0 is due on the first invocation; the compare uses the raw stored value.
    OriginalPostFigurePassState zeroPeriod;
    zeroPeriod.counter = 0;
    zeroPeriod.period = 0;
    need(advanceOriginalPostFigurePass(zeroPeriod, in).due, "period 0 is due on the first invocation");
    need(zeroPeriod.period == 30, "a due scan rewrites the period to 30");

    // A period with the top bit set is negative in the signed compare.
    OriginalPostFigurePassState negativePeriod;
    negativePeriod.counter = 0;
    negativePeriod.period = 0xFFFFFFFFu;
    need(advanceOriginalPostFigurePass(negativePeriod, in).due,
         "period 0xFFFFFFFF compares as -1, so increment 1 is due");
    need(negativePeriod.counter == 0, "the due scan resets the counter");
    OriginalPostFigurePassState minPeriod;
    minPeriod.counter = 0;
    minPeriod.period = 0x80000000u;
    need(advanceOriginalPostFigurePass(minPeriod, in).due,
         "period 0x80000000 compares as INT32_MIN, so increment 1 is due");

    // -1 <= -1 is not due: the signed comparison accepts equality.
    OriginalPostFigurePassState equality;
    equality.counter = 0xFFFFFFFEu;
    equality.period = 0xFFFFFFFFu;
    need(!advanceOriginalPostFigurePass(equality, in).due,
         "0xFFFFFFFF == -1 is equal to period -1, so not due");
    need(equality.counter == 0xFFFFFFFFu, "the equal case stores the increment");

    // The factory seeds the two globals from the required session values.
    OriginalPostFigurePassInputs seeded = session(0, 0, 1, 7, 50);
    const auto built = originalPostFigurePassSessionState(seeded);
    need(built.has_value(), "complete session inputs build a state");
    need(built->counter == 7 && built->period == 50, "state equals the declared initials");
    OriginalPostFigurePassInputs noCounter = session();
    noCounter.counterInitial0x12A6E78.reset();
    need(!originalPostFigurePassSessionState(noCounter).has_value(),
         "absent 0x12A6E78 initial value refuses to build a state");
    OriginalPostFigurePassInputs noPeriod = session();
    noPeriod.periodInitial0x859DA0.reset();
    need(!originalPostFigurePassSessionState(noPeriod).has_value(),
         "absent 0x859DA0 initial value refuses to build a state");
}

// ---------------------------------------------------------------------------
// Session inputs, gate, and the scan loop's refusal order
// ---------------------------------------------------------------------------
void requiredInputTests() {
    // Default-constructed inputs carry no session value at all; nothing may run.
    {
        OriginalPostFigurePassState state;
        state.counter = 4;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, OriginalPostFigurePassInputs{});
        need(r.status == OriginalPostFigurePassStatus::RefusedMissingSessionInput,
             "inputs with no session fields refuse");
        need(state.counter == 4 && state.period == 30, "a missing-input refusal does not mutate state");
        need(r.visited == 0, "a missing-input refusal visits nothing");
    }
    // Each single required field is checked, one at a time.
    for (int field = 0; field < 5; ++field) {
        OriginalPostFigurePassInputs in = session();
        if (field == 0) in.gate0x88EC38.reset();
        if (field == 1) in.pause0x10DE0D0.reset();
        if (field == 2) in.mode0x10DE045.reset();
        if (field == 3) in.counterInitial0x12A6E78.reset();
        if (field == 4) in.periodInitial0x859DA0.reset();
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::RefusedMissingSessionInput,
             "dropping any one required session field refuses");
        need(state.counter == 30 && state.period == 30, "the refusal leaves both globals alone");
    }
    // Gate 0x88EC38: the caller at 0x005371D3 skips the body when it is non-zero.
    {
        OriginalPostFigurePassInputs in = session(1);
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::SkippedGate, "gate 1 skips the pass");
        need(state.counter == 30 && state.period == 30, "a gated pass does not touch the counters");
        need(r.visited == 0 && !r.due, "a gated pass never reaches the counter block");
    }
    // Any non-zero gate value skips, not just 1.
    for (uint8_t gate : {uint8_t(2), uint8_t(0x7F), uint8_t(0xFF)}) {
        OriginalPostFigurePassInputs in = session(gate);
        OriginalPostFigurePassState state;
        state.counter = 5;
        state.period = 30;
        need(advanceOriginalPostFigurePass(state, in).status ==
                 OriginalPostFigurePassStatus::SkippedGate,
             "every non-zero gate value skips");
        need(state.counter == 5, "skipping never increments the countdown");
    }
}

void refusalTests() {
    // A null slot dword at 0x00564B91: the exe dereferences without a check.
    {
        Pool pool(2);
        pool.slots[1] = nullptr;
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::RefusedNullRegistrySlot,
             "a null registry slot refuses instead of dereferencing null");
        need(r.index == 1 && r.visited == 1, "the refusal names slot 1");
        need(state.counter == 31, "the counter increment at 0x00564B5E already happened");
        need(state.period == 30, "the period write at 0x00564C39 was not reached");
    }
    // 0x005653C0 absent: first unported callee on an accepted, live object.
    {
        Pool pool(2);
        pool.objects[1].type0x14 = 78;
        pool.objects[1].live0x04 = 1;
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::RefusedUnported5653C0,
             "absent 0x005653C0 refuses by name");
        need(state.counter == 31, "the refusal keeps the stored increment");
        need(state.period == 30, "the refusal keeps the old period");
        // A retry is still due, so the refusal is not hidden by the counter.
        const auto again = advanceOriginalPostFigurePass(state, in);
        need(again.status == OriginalPostFigurePassStatus::RefusedUnported5653C0 &&
                 state.counter == 32,
             "a repeated refusal keeps the pass due instead of resetting the window");
    }
    // 0x0056C880 absent: the record exists and the frame is not paused.
    {
        Pool pool(2);
        pool.objects[1].type0x14 = 78;
        pool.objects[1].live0x04 = 3;
        int record = 0;
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        in.indexRecord0x5653C0 = [&record](const OriginalPostFigureRegistryObject&, uint32_t) {
            return static_cast<const void*>(&record);
        };
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::RefusedUnported56C880,
             "absent 0x0056C880 refuses after a record is found");
    }
    // 0x0056D170 absent: 0x0056C880 was supplied and ran.
    {
        Pool pool(2);
        pool.objects[1].type0x14 = 78;
        pool.objects[1].live0x04 = 1;
        int record = 0;
        Calls calls;
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        in.indexRecord0x5653C0 = [&record](const OriginalPostFigureRegistryObject&, uint32_t) {
            return static_cast<const void*>(&record);
        };
        in.updateRecord0x56C880 = [&calls](const void*, uint32_t) { ++calls.update; };
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::RefusedUnported56D170,
             "absent 0x0056D170 refuses after 0x0056C880 ran");
        need(calls.update == 1, "0x0056C880 is not skipped before the 0x0056D170 refusal");
        need(!r.progress, "no progress was recorded before the refusal");
    }
    // Types 76/77 reach 0x00564EA0 even when the record is missing.
    {
        Pool pool(2);
        pool.objects[1].type0x14 = 76;
        pool.objects[1].live0x04 = 1;
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        in.indexRecord0x5653C0 = [](const OriginalPostFigureRegistryObject&, uint32_t) {
            return static_cast<const void*>(nullptr);
        };
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::RefusedUnported564EA0,
             "type 76 reaches 0x00564EA0 with no record");
        // With 0x00564EA0 supplied, the same object falls through to 0x00567610.
        Calls calls;
        in.handleTypes76_77_0x564EA0 = [&calls](const OriginalPostFigureRegistryObject&, uint32_t) {
            ++calls.handle;
        };
        OriginalPostFigurePassState second;
        second.counter = 30;
        second.period = 30;
        const auto r2 = advanceOriginalPostFigurePass(second, in);
        need(r2.status == OriginalPostFigurePassStatus::RefusedUnported567610,
             "supplying 0x00564EA0 exposes the next unported callee");
        need(calls.handle == 1, "0x00564EA0 ran once for type 76");
        need(calls.update == 0 && calls.step == 0, "a missing record skips 0x0056C880/0x0056D170");
    }
    // Type 77 takes the same branch; type 78 does not.
    for (int16_t type : {int16_t(76), int16_t(77)}) {
        Pool pool(2);
        pool.objects[1].type0x14 = type;
        pool.objects[1].live0x04 = 1;
        Calls calls;
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        in.indexRecord0x5653C0 = [](const OriginalPostFigureRegistryObject&, uint32_t) {
            return static_cast<const void*>(nullptr);
        };
        in.handleTypes76_77_0x564EA0 = [&calls](const OriginalPostFigureRegistryObject&, uint32_t) {
            ++calls.handle;
        };
        in.skipAdvance0x567610 = [&calls](int32_t) {
            ++calls.skip;
            return 1u;
        };
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::Ok, "types 76/77 run to the end");
        need(calls.handle == 1, "0x00564EA0 is called for type 76 and 77 only");
        need(calls.skip == 1, "0x00567610 is called after 0x00564EA0");
    }
    {
        Pool pool(2);
        pool.objects[1].type0x14 = 78;
        pool.objects[1].live0x04 = 1;
        Calls calls;
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        in.indexRecord0x5653C0 = [](const OriginalPostFigureRegistryObject&, uint32_t) {
            return static_cast<const void*>(nullptr);
        };
        in.skipAdvance0x567610 = [&calls](int32_t) {
            ++calls.skip;
            return 1u;
        };
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        need(advanceOriginalPostFigurePass(state, in).status == OriginalPostFigurePassStatus::Ok,
             "type 78 skips the 0x00564EA0 branch");
        need(calls.handle == 0, "0x00564EA0 is not called for type 78");
    }
    // 0x00567610 answering 0 would leave the scan index in place and repeat the slot.
    {
        Pool pool(2);
        pool.objects[1].type0x14 = 78;
        pool.objects[1].live0x04 = 1;
        Calls calls;
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        allProviders(in, calls, nullptr, false, 0u);
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::Refused567610ZeroAdvance,
             "a zero skip count is a named refusal, not a silent +1");
        need(r.index == 1, "the zero-skip refusal names the stuck index");
        need(calls.skip == 1 && calls.update == 0,
             "no record means 0x00567610 runs but 0x0056C880 does not");
        need(state.counter == 31 && state.period == 30, "the zero-skip refusal keeps both globals");
    }
}

void scanTests() {
    // Pause byte 0x010DE0D0 and mode 0x010DE045 == 86 skip only the entity work.
    for (int variant = 0; variant < 2; ++variant) {
        Pool pool(2);
        pool.objects[1].type0x14 = 78;
        pool.objects[1].live0x04 = 1;
        int record = 0;
        Calls calls;
        OriginalPostFigurePassInputs in =
            session(0, uint8_t(variant == 0 ? 1 : 0), uint8_t(variant == 1 ? 86 : 1));
        in.registry = pool.slots;
        in.indexRecord0x5653C0 = [&record](const OriginalPostFigureRegistryObject&, uint32_t) {
            return static_cast<const void*>(&record);
        };
        in.skipAdvance0x567610 = [&calls](int32_t) {
            ++calls.skip;
            return 1u;
        };
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::Ok, "paused/mode-86 slots still complete");
        need(calls.update == 0, "the entity update is skipped while paused or in mode 86");
        need(calls.skip == 1, "the type advance still runs while paused or in mode 86");
        need(!r.progress, "no progress is reported when the entity work is skipped");
        need(state.period == 30, "a paused scan keeps period 30");
    }

    // Rejected type: no type advance at all (0x00564BA2 -> 0x00564C17).
    {
        Pool pool(2);
        pool.objects[1].type0x14 = 87;
        pool.objects[1].live0x04 = 1;
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::Ok, "a whitelist rejection is not a refusal");
        need(r.visited == 1, "the rejected slot is visited once");
        need(state.period == 30, "a rejected slot produces no progress");
    }
    // word[obj+22] and the live predicate reject before any callee.
    for (int variant = 0; variant < 2; ++variant) {
        Pool pool(2);
        pool.objects[1].type0x14 = 78;
        if (variant == 0) pool.objects[1].word0x16 = 1;
        if (variant == 1) pool.objects[1].live0x04 = 2;
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::Ok, "word[+22]/live rejections are not refusals");
        need(r.visited == 1, "the rejected slot is still visited");
    }

    // Full pass: type 76 and type 78 both have records and report progress.
    {
        Pool pool(3);
        pool.objects[1].type0x14 = 76;
        pool.objects[1].live0x04 = 1;
        pool.objects[2].type0x14 = 78;
        pool.objects[2].live0x04 = 3;
        int record = 0;
        Calls calls;
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        allProviders(in, calls, &record, true, 1u);
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::Ok, "both slots complete");
        need(r.visited == 2, "both slots are visited");
        need(calls.index == 2 && calls.update == 2 && calls.step == 2,
             "0x005653C0/0x0056C880/0x0056D170 run once per accepted live slot");
        need(calls.handle == 1, "0x00564EA0 runs for type 76 only");
        need(calls.skip == 2, "0x00567610 runs for both slots");
        need(r.progress, "a true 0x0056D170 return sets the progress flag");
        need(state.period == 50, "progress selects period 50 at 0x00564C46");
        need(state.counter == 0, "the counter is reset after the scan");
    }
    // Progress off keeps 30 even though 0x0056D170 ran.
    {
        Pool pool(2);
        pool.objects[1].type0x14 = 78;
        pool.objects[1].live0x04 = 1;
        int record = 0;
        Calls calls;
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        allProviders(in, calls, &record, false, 1u);
        OriginalPostFigurePassState state;
        state.counter = 50;
        state.period = 50;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::Ok && !r.progress, "no progress reported");
        need(r.due, "the 51st invocation exceeds the supplied period 50");
        need(state.period == 30, "no progress writes 30");
    }
    // The type advance jumps over later slots: 1 + 5 >= 4 ends the loop.
    {
        Pool pool(4);
        pool.objects[1].type0x14 = 253;
        pool.objects[1].live0x04 = 1;
        for (size_t i = 2; i < 4; ++i) {
            pool.objects[i].type0x14 = 253;
            pool.objects[i].live0x04 = 1;
        }
        int record = 0;
        Calls calls;
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        allProviders(in, calls, &record, false, 5u);
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::Ok, "a large skip still terminates");
        need(r.visited == 1 && calls.skip == 1, "an advance of 5 leaves the 4-slot pool after one visit");
    }
    // Count <= 1 still resets the counter and rewrites the period (0x00564B7E).
    for (size_t count : {size_t(0), size_t(1)}) {
        Pool pool(count);
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        OriginalPostFigurePassState state;
        state.counter = 50;
        state.period = 50;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::Ok, "an empty/short pool completes");
        need(r.due, "the invocation is due before the count-1 short circuit");
        need(r.visited == 0, "count <= 1 visits no slot");
        need(state.counter == 0, "count <= 1 still resets the counter at 0x00564C2D");
        need(state.period == 30, "count <= 1 still writes period 30");
    }
    // After a successful progress scan the next window is 51 calls, not 31.
    {
        Pool pool(2);
        pool.objects[1].type0x14 = 78;
        pool.objects[1].live0x04 = 1;
        int record = 0;
        Calls calls;
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        allProviders(in, calls, &record, true, 1u);
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        need(advanceOriginalPostFigurePass(state, in).due, "the first scan is due at 31");
        need(state.period == 50, "progress moves the window to 50");
        std::vector<uint32_t> scans;
        for (uint32_t tick = 1; tick <= 120; ++tick)
            if (advanceOriginalPostFigurePass(state, in).due) scans.push_back(tick);
        need(scans == std::vector<uint32_t>({51, 102}),
             "the progressed window scans every 51st invocation and keeps 50");
    }
}
}  // namespace

int main() {
    try {
        whitelistTests();
        counterTests();
        requiredInputTests();
        refusalTests();
        scanTests();
        std::cout << "{\"passed\":true,\"authored_checks\":" << checks << "}\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
