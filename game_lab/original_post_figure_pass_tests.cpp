#include "original_post_figure_pass.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>
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

// ---------------------------------------------------------------------------
// 0x012A5DD0 record table: 0x00567610 skip advance and 0x005653C0 lookup
// ---------------------------------------------------------------------------
void recordTableTests() {
    need(kOriginalPostFigureRecordTableVa == 0x012A5DD0u, "record array base 0x012A5DD0");
    need(kOriginalPostFigureRecordKeyVa == kOriginalPostFigureRecordTableVa + 4,
         "key dword is record+4 (0x012A5DD4)");
    need(kOriginalPostFigureRecordAdvanceVa == kOriginalPostFigureRecordTableVa + 12,
         "advance dword is record+12 (0x012A5DDC)");
    need(kOriginalPostFigureRecordCountVa == 0x012A6E7Cu, "entry count at 0x012A6E7C");
    need(kOriginalPostFigureRecordSize == 144, "one record is 144 bytes (0x005653F7 scale)");
    need(kOriginalPostFigureRecordCapacity == 27, "registration stops at 27 (0x00567675)");
    need(kOriginalPostFigureRecordKeyOffset == 4, "key offset is 4");
    need(kOriginalPostFigureRecordAdvanceOffset == 12, "advance offset is 12");

    OriginalPostFigureRecordTable table;
    table.records.push_back({78, 7});
    table.records.push_back({76, 1});
    table.records.push_back({93, 4});
    table.records.push_back({78, 9});  // duplicate key: the first record must win

    // 0x00567610: found answers dword+12, missing and empty answer zero.
    need(originalPostFigureSkipAdvance0x567610(table, 78) == 7,
         "the first matching record answers its +12 dword");
    need(originalPostFigureSkipAdvance0x567610(table, 76) == 1, "type 76 answers 1");
    need(originalPostFigureSkipAdvance0x567610(table, 93) == 4, "type 93 answers 4");
    need(originalPostFigureSkipAdvance0x567610(table, 79) == 0,
         "a type with no record answers zero (0x00567635)");
    need(originalPostFigureSkipAdvance0x567610(table, 0) == 0, "type 0 has no record");
    need(originalPostFigureSkipAdvance0x567610(table, -78) == 0, "the key is signed");
    need(originalPostFigureSkipAdvance0x567610(OriginalPostFigureRecordTable{}, 78) == 0,
         "an empty array answers zero");
    // A negative +12 is answered unchanged; the caller's net advance is signed.
    OriginalPostFigureRecordTable negative;
    negative.records.push_back({253, -3});
    need(originalPostFigureSkipAdvance0x567610(negative, 253) == -3,
         "a negative +12 is not clamped");

    // 0x005653C0: same key comparison, but the first hit is answered as a pointer
    // and a miss answers null.
    OriginalPostFigureRegistryObject object;
    object.type0x14 = 78;
    const OriginalPostFigureRecord* hit = originalPostFigureFindRecord0x5653C0(table, object);
    need(hit != nullptr, "a live key is found");
    need(hit == &table.records[0], "the first record with the key wins (0x005653F4)");
    need(hit->advance0x0C == 7, "the answered record is the one 0x00567610 would read");
    object.type0x14 = 93;
    need(originalPostFigureFindRecord0x5653C0(table, object) == &table.records[2],
         "type 93 resolves its own record");
    object.type0x14 = 269;
    need(originalPostFigureFindRecord0x5653C0(table, object) == nullptr,
         "an unregistered type answers null");
    object.type0x14 = -78;
    need(originalPostFigureFindRecord0x5653C0(table, object) == nullptr,
         "a negative type does not match the positive key");
    object.type0x14 = 78;
    need(originalPostFigureFindRecord0x5653C0(OriginalPostFigureRecordTable{}, object) == nullptr,
         "an empty array answers null (0x00565401)");

    // A negative key is reachable: the record key is a whole dword.
    OriginalPostFigureRecordTable signedKeys;
    signedKeys.records.push_back({-5, 2});
    object.type0x14 = -5;
    need(originalPostFigureFindRecord0x5653C0(signedKeys, object) == &signedKeys.records[0],
         "a negative type matches a negative key dword");
    need(originalPostFigureSkipAdvance0x567610(signedKeys, -5) == 2,
         "the skip lookup matches the same negative key");

    // The table must not be reordered or deduplicated: order defines the answer.
    OriginalPostFigureRecordTable order;
    order.records.push_back({76, 1});
    order.records.push_back({76, 2});
    need(originalPostFigureSkipAdvance0x567610(order, 76) == 1,
         "slot order decides which duplicate answers");

    // Wiring: with the array supplied and both providers absent, the pass runs the
    // ported lookups and reaches the next unported callee.
    {
        Pool pool(2);
        pool.objects[1].type0x14 = 78;
        pool.objects[1].live0x04 = 1;
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        in.recordTable0x12A5DD0 = table;
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::RefusedUnported56C880,
             "the ported 0x005653C0 finds a record and 0x0056C880 is the next gap");
        need(r.index == 1, "the refusal names the scanned slot");
    }
    // An unregistered type answers null, so the entity work is skipped and the
    // ported 0x00567610 still advances by one.
    {
        Pool pool(3);
        pool.objects[1].type0x14 = 78;
        pool.objects[1].live0x04 = 1;
        pool.objects[2].type0x14 = 78;
        pool.objects[2].live0x04 = 1;
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        OriginalPostFigureRecordTable empty;
        in.recordTable0x12A5DD0 = empty;
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::Refused567610ZeroAdvance,
             "a miss in 0x005653C0 skips the entity work; a miss in 0x00567610 is the zero refusal");
        need(r.index == 1, "the zero refusal happens on the first slot");
        need(state.counter == 31 && state.period == 30, "the zero refusal keeps both globals");
    }
    // The skip advance drives the cursor: type 78 answers 3, so slots 1 and 2 are
    // passed over and the scan ends.
    {
        Pool pool(5);
        for (size_t i = 1; i < 5; ++i) {
            pool.objects[i].type0x14 = 78;
            pool.objects[i].live0x04 = 1;
        }
        OriginalPostFigureRecordTable advanceThree;
        advanceThree.records.push_back({78, 3});
        advanceThree.records.push_back({79, 1});
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        in.recordTable0x12A5DD0 = advanceThree;
        in.updateRecord0x56C880 = [](const void*, uint32_t) {};
        in.stepRecord0x56D170 = [](const void*, uint32_t) { return false; };
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::Ok, "the scan completes");
        need(r.visited == 2 && r.index == 4,
             "an advance of 3 steps from slot 1 to slot 4, which is visited next");
        need(state.period == 30, "no progress keeps the period at 30");
    }
    // A supplied provider still wins over the array.
    {
        Pool pool(2);
        pool.objects[1].type0x14 = 78;
        pool.objects[1].live0x04 = 1;
        OriginalPostFigureRecordTable supplied;
        supplied.records.push_back({78, 9});
        Calls calls;
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        in.recordTable0x12A5DD0 = supplied;
        allProviders(in, calls, nullptr, false, 1u);
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::Ok, "provider-backed pass completes");
        need(calls.index == 1, "the supplied 0x005653C0 provider is used");
        need(calls.skip == 1, "the supplied 0x00567610 provider is used");
    }
    // A negative skip answer walks the index back exactly as 0x00564C13 does; the
    // unsigned continuation check at 0x00564C22 ends the scan once it wraps.
    {
        Pool pool(3);
        for (size_t i = 0; i < 3; ++i) {
            pool.objects[i].type0x14 = 78;
            pool.objects[i].live0x04 = 1;
        }
        OriginalPostFigureRecordTable backwards;
        backwards.records.push_back({78, -1});
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        in.recordTable0x12A5DD0 = backwards;
        in.updateRecord0x56C880 = [](const void*, uint32_t) {};
        in.stepRecord0x56D170 = [](const void*, uint32_t) { return false; };
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::Ok,
             "a negative advance still terminates through the unsigned bound");
        need(r.index == 0, "the last visited slot is slot 0 before the index wraps");
        need(r.visited == 2, "the walk visits slot 1 then slot 0");
    }
}

// ---------------------------------------------------------------------------
// 0x0056C880 / 0x0056D170: the exact profiles and the relative-index entry
// ---------------------------------------------------------------------------
// Both profiles write dwords +16..+40 of the record. The entry contract of each
// callee is registry[index] and relative index = index - word[obj+22]; a value of
// at most zero ends the callee (0x0056C8BD / 0x0056D1B3), a positive one enters
// the container walk that stays refused.
void updateStepPrefixTests() {
    need(kOriginalPostFigureRecordProfileFirstOffset == 16, "the profiles start at +16");
    need(kOriginalPostFigureRecordProfileStride == 4, "each profile dword is four bytes");

    // 0x0056A910 writes 2,3,8,7,5,3,3; 0x0056A940 supplies exactly twice that.
    {
        OriginalPostFigureRecordScalars scalars;
        scalars.field0x10 = scalars.field0x14 = scalars.field0x18 = -1;
        scalars.field0x1C = scalars.field0x20 = scalars.field0x24 = scalars.field0x28 = -1;
        originalPostFigureRecordProfile0x56A910(scalars);
        need(scalars.field0x10 == 2 && scalars.field0x14 == 3 && scalars.field0x18 == 8 &&
                 scalars.field0x1C == 7 && scalars.field0x20 == 5 && scalars.field0x24 == 3 &&
                 scalars.field0x28 == 3,
             "0x0056A910 writes 2,3,8,7,5,3,3 at +16..+40");
        originalPostFigureRecordProfile0x56A940(scalars);
        need(scalars.field0x10 == 4 && scalars.field0x14 == 6 && scalars.field0x18 == 16 &&
                 scalars.field0x1C == 14 && scalars.field0x20 == 10 && scalars.field0x24 == 6 &&
                 scalars.field0x28 == 6,
             "0x0056A940 writes 4,6,16,14,10,6,6 at +16..+40");
        for (size_t i = 0; i < 7; ++i) {
            need(kOriginalPostFigureRecordProfile0x56A940[i] ==
                     2 * kOriginalPostFigureRecordProfile0x56A910[i],
                 "every 0x0056A940 value is twice the 0x0056A910 value");
        }
    }

    // 0x0056C880: the profile runs first, then the relative-index leave.
    auto updateWith = [](uint32_t index,
                         const std::vector<const OriginalPostFigureRegistryObject*>& registry,
                         bool gate, OriginalPostFigureRecordScalars* target, unsigned& gateAsks) {
        OriginalPostFigure56C880Inputs inputs;
        inputs.gate0x564E80 = [&gateAsks, gate]() {
            ++gateAsks;
            return gate;
        };
        inputs.scalarsForRecord = [target]() { return target; };
        return originalPostFigureUpdateRecord0x56C880(index, registry, inputs);
    };
    const auto baseProfile = [](const OriginalPostFigureRecordScalars& s) {
        return s.field0x10 == 2 && s.field0x14 == 3 && s.field0x18 == 8 && s.field0x1C == 7 &&
               s.field0x20 == 5 && s.field0x24 == 3 && s.field0x28 == 3;
    };
    const auto doubledProfile = [](const OriginalPostFigureRecordScalars& s) {
        return s.field0x10 == 4 && s.field0x14 == 6 && s.field0x18 == 16 && s.field0x1C == 14 &&
               s.field0x20 == 10 && s.field0x24 == 6 && s.field0x28 == 6;
    };
    // Gate false, relative index 2: the base profile lands, the body is refused.
    {
        OriginalPostFigureRegistryObject slot;
        std::vector<const OriginalPostFigureRegistryObject*> registry(3, &slot);
        OriginalPostFigureRecordScalars scalars;
        unsigned gateAsks = 0;
        const auto status = updateWith(2, registry, false, &scalars, gateAsks);
        need(status == OriginalPostFigure56C880Status::RefusedContainers,
             "a positive relative index enters the unported container walk");
        need(baseProfile(scalars), "the base profile is written before the refusal");
        need(gateAsks == 1, "0x00564E80 is asked exactly once");
        need(originalPostFigure56C880StatusName(status)[0] == 'r', "the refusal has a name");
    }
    // Gate true: the doubled profile lands instead.
    {
        OriginalPostFigureRegistryObject slot;
        std::vector<const OriginalPostFigureRegistryObject*> registry(3, &slot);
        OriginalPostFigureRecordScalars scalars;
        unsigned gateAsks = 0;
        const auto status = updateWith(2, registry, true, &scalars, gateAsks);
        need(status == OriginalPostFigure56C880Status::RefusedContainers,
             "the doubled profile does not change the entry contract");
        need(doubledProfile(scalars), "gate true selects the doubled profile");
        need(gateAsks == 1, "the gate still runs once");
    }
    // A negative word increases the relative index, so the walk is entered.
    {
        OriginalPostFigureRegistryObject slot;
        slot.word0x16 = -2;
        std::vector<const OriginalPostFigureRegistryObject*> registry(3, &slot);
        OriginalPostFigureRecordScalars scalars;
        unsigned gateAsks = 0;
        need(updateWith(2, registry, false, &scalars, gateAsks) ==
                 OriginalPostFigure56C880Status::RefusedContainers,
             "index minus a negative word stays positive");
    }
    // Relative index exactly zero and negative both leave with Ok.
    for (int16_t word : {int16_t(2), int16_t(5), int16_t(7)}) {
        OriginalPostFigureRegistryObject slot;
        slot.word0x16 = word;
        std::vector<const OriginalPostFigureRegistryObject*> registry(3, &slot);
        OriginalPostFigureRecordScalars scalars;
        unsigned gateAsks = 0;
        need(updateWith(2, registry, false, &scalars, gateAsks) ==
                 OriginalPostFigure56C880Status::Ok,
             "a relative index of at most zero ends 0x0056C880");
        need(baseProfile(scalars), "the profile still runs on the early leave");
    }
    // Index zero with a zero word is also a zero relative index.
    {
        OriginalPostFigureRegistryObject slot;
        std::vector<const OriginalPostFigureRegistryObject*> registry(1, &slot);
        OriginalPostFigureRecordScalars scalars;
        unsigned gateAsks = 0;
        need(updateWith(0, registry, false, &scalars, gateAsks) ==
                 OriginalPostFigure56C880Status::Ok,
             "index 0 minus word 0 is zero, so the callee leaves");
    }
    // The registry read is after the profile: an out-of-pool index still writes it.
    {
        OriginalPostFigureRegistryObject slot;
        std::vector<const OriginalPostFigureRegistryObject*> registry(2, &slot);
        OriginalPostFigureRecordScalars scalars;
        unsigned gateAsks = 0;
        need(updateWith(3, registry, true, &scalars, gateAsks) ==
                 OriginalPostFigure56C880Status::RefusedIndexOutsidePool,
             "an index outside the pool refuses by name");
        need(doubledProfile(scalars), "the profile precedes the registry read at 0x0056C8AA");
    }
    // A null slot is refused rather than dereferenced; the profile already ran.
    {
        std::vector<const OriginalPostFigureRegistryObject*> registry(2, nullptr);
        OriginalPostFigureRecordScalars scalars;
        unsigned gateAsks = 0;
        need(updateWith(1, registry, false, &scalars, gateAsks) ==
                 OriginalPostFigure56C880Status::RefusedNullObject,
             "a null slot refuses at 0x0056C8AF");
        need(baseProfile(scalars), "a null slot does not undo the profile");
    }
    // Both required inputs are checked before anything is written.
    {
        OriginalPostFigureRegistryObject slot;
        std::vector<const OriginalPostFigureRegistryObject*> registry(2, &slot);
        OriginalPostFigureRecordScalars scalars;
        scalars.field0x10 = 77;
        OriginalPostFigure56C880Inputs noGate;
        noGate.scalarsForRecord = [&scalars]() { return &scalars; };
        need(originalPostFigureUpdateRecord0x56C880(1, registry, noGate) ==
                 OriginalPostFigure56C880Status::RefusedMissingInput,
             "a missing 0x00564E80 gate refuses");
        need(scalars.field0x10 == 77, "the missing gate writes nothing");
        OriginalPostFigure56C880Inputs noTarget;
        noTarget.gate0x564E80 = []() { return false; };
        need(originalPostFigureUpdateRecord0x56C880(1, registry, noTarget) ==
                 OriginalPostFigure56C880Status::RefusedMissingInput,
             "a missing scalar target refuses");
        unsigned gateAsks = 0;
        need(updateWith(1, registry, false, nullptr, gateAsks) ==
                 OriginalPostFigure56C880Status::RefusedMissingInput,
             "a null scalar target refuses before the gate runs");
        need(gateAsks == 0, "a null scalar target is rejected first");
    }
    // Status names are stable for the two refusals used by the pass.
    need(std::string(originalPostFigure56C880StatusName(
             OriginalPostFigure56C880Status::RefusedContainers)) == "refused_containers",
         "0x0056C880 names the container refusal");
    need(std::string(originalPostFigure56D170StatusName(
             OriginalPostFigure56D170Status::RefusedContainers)) == "refused_containers",
         "0x0056D170 names the container refusal");

    // 0x0056D170: the same entry contract, and its early leave answers false.
    for (int16_t word : {int16_t(2), int16_t(7), int16_t(32000)}) {
        OriginalPostFigureRegistryObject slot;
        slot.word0x16 = word;
        std::vector<const OriginalPostFigureRegistryObject*> registry(3, &slot);
        need(originalPostFigureStepRecord0x56D170(2, registry) ==
                 OriginalPostFigure56D170Status::OkFalse,
             "a relative index of at most zero answers false at 0x0056D1B3");
    }
    {
        OriginalPostFigureRegistryObject slot;
        std::vector<const OriginalPostFigureRegistryObject*> registry(1, &slot);
        need(originalPostFigureStepRecord0x56D170(0, registry) ==
                 OriginalPostFigure56D170Status::OkFalse,
             "index 0 minus word 0 answers false");
    }
    // A positive relative index, including a negative word that increases it.
    for (int16_t word : {int16_t(0), int16_t(-1)}) {
        OriginalPostFigureRegistryObject slot;
        slot.word0x16 = word;
        std::vector<const OriginalPostFigureRegistryObject*> registry(3, &slot);
        need(originalPostFigureStepRecord0x56D170(2, registry) ==
                 OriginalPostFigure56D170Status::RefusedContainers,
             "a positive relative index enters the unported body");
    }
    {
        OriginalPostFigureRegistryObject slot;
        std::vector<const OriginalPostFigureRegistryObject*> registry(2, &slot);
        need(originalPostFigureStepRecord0x56D170(2, registry) ==
                 OriginalPostFigure56D170Status::RefusedIndexOutsidePool,
             "an index outside the pool refuses by name");
        std::vector<const OriginalPostFigureRegistryObject*> nullSlot(2, nullptr);
        need(originalPostFigureStepRecord0x56D170(1, nullSlot) ==
                 OriginalPostFigure56D170Status::RefusedNullObject,
             "a null slot refuses at 0x0056D1A3");
    }

    // Pass wiring: with the exact 0x0056C880 prefix supplied and no provider, the
    // profiles land and the pass stops at the narrower container refusal. The
    // pass guarantees word[obj+22] == 0 (0x00564BA4), so the relative index is
    // always positive here and the early leave cannot trigger from the pass.
    auto passWithProfile = [](bool gate, unsigned& gateAsks) {
        OriginalPostFigurePassInputs in = session();
        Pool pool(2);
        pool.objects[1].type0x14 = 78;
        pool.objects[1].live0x04 = 1;
        in.registry = pool.slots;
        OriginalPostFigureRecordTable table;
        table.records.push_back({78, 1});
        in.recordTable0x12A5DD0 = table;
        OriginalPostFigureRecordScalars scalars;
        OriginalPostFigure56C880Inputs profile;
        profile.gate0x564E80 = [&gateAsks, gate]() {
            ++gateAsks;
            return gate;
        };
        profile.scalarsForRecord = [&scalars]() { return &scalars; };
        in.updateProfile0x56C880 = profile;
        in.stepPrefix0x56D170 = true;
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto result = advanceOriginalPostFigurePass(state, in);
        return std::make_tuple(result, state, scalars);
    };
    {
        unsigned gateAsks = 0;
        auto [result, state, scalars] = passWithProfile(false, gateAsks);
        need(result.status == OriginalPostFigurePassStatus::Refused56C880Containers,
             "the pass stops at the narrower 0x0056C880 container refusal");
        need(baseProfile(scalars), "the pass ran the base profile through the exact prefix");
        need(gateAsks == 1, "the pass asked the 0x00564E80 gate once");
        need(result.index == 1 && result.visited == 1, "the refusal names slot 1");
        need(!result.progress && state.counter == 31 && state.period == 30,
             "the refusal keeps the progress flag, counter and period unchanged");
    }
    {
        unsigned gateAsks = 0;
        auto [result, state, scalars] = passWithProfile(true, gateAsks);
        need(result.status == OriginalPostFigurePassStatus::Refused56C880Containers,
             "gate true still stops at the container walk");
        need(doubledProfile(scalars), "gate true via the pass selects the doubled profile");
        need(!result.progress && state.period == 30, "no progress is reported");
    }
    // The struct without its gate keeps the old name and writes nothing.
    {
        OriginalPostFigurePassInputs in = session();
        Pool pool(2);
        pool.objects[1].type0x14 = 78;
        pool.objects[1].live0x04 = 1;
        in.registry = pool.slots;
        OriginalPostFigureRecordTable table;
        table.records.push_back({78, 1});
        in.recordTable0x12A5DD0 = table;
        OriginalPostFigureRecordScalars scalars;
        scalars.field0x10 = 55;
        OriginalPostFigure56C880Inputs profile;
        profile.scalarsForRecord = [&scalars]() { return &scalars; };
        in.updateProfile0x56C880 = profile;
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto result = advanceOriginalPostFigurePass(state, in);
        need(result.status == OriginalPostFigurePassStatus::RefusedUnported56C880,
             "an incomplete profile falls back to the old 0x0056C880 name");
        need(scalars.field0x10 == 55, "an incomplete profile writes nothing");
    }
    // Without the profile and without the provider the old name is unchanged.
    {
        OriginalPostFigurePassInputs in = session();
        Pool pool(2);
        pool.objects[1].type0x14 = 78;
        pool.objects[1].live0x04 = 1;
        in.registry = pool.slots;
        OriginalPostFigureRecordTable table;
        table.records.push_back({78, 1});
        in.recordTable0x12A5DD0 = table;
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        need(advanceOriginalPostFigurePass(state, in).status ==
                 OriginalPostFigurePassStatus::RefusedUnported56C880,
             "no profile and no provider keep refused_unported_56c880");
    }
    // With the update provider present and the step prefix enabled, the pass
    // reaches 0x0056D170's exact entry and stops at its container walk.
    auto passWithStepPrefix = [](bool useStepProvider, bool providerAnswer) {
        OriginalPostFigurePassInputs in = session();
        Pool pool(2);
        pool.objects[1].type0x14 = 78;
        pool.objects[1].live0x04 = 1;
        in.registry = pool.slots;
        OriginalPostFigureRecordTable table;
        table.records.push_back({78, 1});
        in.recordTable0x12A5DD0 = table;
        in.updateRecord0x56C880 = [](const void*, uint32_t) {};
        in.stepPrefix0x56D170 = true;
        unsigned stepAsks = 0;
        if (useStepProvider) {
            in.stepRecord0x56D170 = [&stepAsks, providerAnswer](const void*, uint32_t) {
                ++stepAsks;
                return providerAnswer;
            };
        }
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto result = advanceOriginalPostFigurePass(state, in);
        return std::make_tuple(result, state, stepAsks);
    };
    {
        auto [result, state, stepAsks] = passWithStepPrefix(false, false);
        need(result.status == OriginalPostFigurePassStatus::Refused56D170Containers,
             "the pass stops at the narrower 0x0056D170 container refusal");
        need(stepAsks == 0, "no step provider ran");
        need(!result.progress && state.period == 30,
             "the container refusal never reports progress");
        need(state.counter == 31, "the counter increment is already stored");
    }
    // A supplied provider still wins over the exact prefix.
    {
        auto [result, state, stepAsks] = passWithStepPrefix(true, true);
        need(result.status == OriginalPostFigurePassStatus::Ok,
             "the step provider completes the pass");
        need(stepAsks == 1, "the supplied 0x0056D170 provider is used");
        need(result.progress && state.period == 50, "the provider answer selects period 50");
    }
    // Prefix off with no provider keeps the old name.
    {
        OriginalPostFigurePassInputs in = session();
        Pool pool(2);
        pool.objects[1].type0x14 = 78;
        pool.objects[1].live0x04 = 1;
        in.registry = pool.slots;
        OriginalPostFigureRecordTable table;
        table.records.push_back({78, 1});
        in.recordTable0x12A5DD0 = table;
        in.updateRecord0x56C880 = [](const void*, uint32_t) {};
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        need(advanceOriginalPostFigurePass(state, in).status ==
                 OriginalPostFigurePassStatus::RefusedUnported56D170,
             "no prefix and no provider keep refused_unported_56d170");
    }
}

// ---------------------------------------------------------------------------
// 0x00564EA0: the types-76/77 step
// ---------------------------------------------------------------------------
struct Type7677Pool {
    std::vector<OriginalPostFigureRegistryObject> objects;
    std::vector<const OriginalPostFigureRegistryObject*> slots;
    explicit Type7677Pool(size_t count) : objects(count), slots(count) {
        for (size_t i = 0; i < count; ++i) slots[i] = &objects[i];
    }
};

struct Type7677Fixture {
    Type7677Pool& pool;
    std::vector<OriginalPostFigureType7677State> states;
    std::vector<OriginalPostFigureType7677Neighbour> neighbours;
    std::vector<uint32_t> asked;
    std::vector<uint32_t> refusing;  // resource ids the probe answers false for
    bool flag = false;
    bool gate = true;
    unsigned gateCalls = 0;
    unsigned flagCalls = 0;
    OriginalPostFigureType7677Inputs inputs;

    explicit Type7677Fixture(Type7677Pool& p)
        : pool(p), states(p.objects.size()), neighbours(p.objects.size()) {
        inputs.stateForIndex = [this](uint32_t i) -> OriginalPostFigureType7677State* {
            if (i >= states.size()) return nullptr;
            return &states[i];
        };
        inputs.neighbourForIndex = [this](uint32_t i) -> const OriginalPostFigureType7677Neighbour* {
            if (i >= neighbours.size()) return nullptr;
            return &neighbours[i];
        };
        inputs.flag0x4812E0 = [this] {
            ++flagCalls;
            return flag;
        };
        inputs.probe0x564E90 = [this] {
            ++gateCalls;
            return gate;
        };
        inputs.probe0x402A50 = [this](uint32_t id) {
            asked.push_back(id);
            for (uint32_t refused : refusing)
                if (refused == id) return false;
            return true;
        };
    }
    bool refuses(uint32_t id) const {
        for (uint32_t refused : refusing)
            if (refused == id) return true;
        return false;
    }
};
OriginalPostFigureType7677Status run7677(Type7677Fixture& f, const OriginalPostFigureRecordTable& records,
                                         uint32_t index) {
    return originalPostFigureHandleTypes7677(f.pool.objects[index], index, f.pool.slots, records, f.inputs);
}

void types7677Tests() {
    // The six constants read back from the file's .rdata block.
    need(kOriginalPostFigureType7677Anchor76 == 700.0, "dword+64 base for type 76 is 700.0");
    need(kOriginalPostFigureType7677AnchorOther == 1100.0, "dword+64 base otherwise is 1100.0");
    need(kOriginalPostFigureType7677Resource76 == 400.0, "dword+80 and dword+68 base 400.0");
    need(kOriginalPostFigureType7677Resource1000 == 1000.0, "dword+68 base for type 77 is 1000.0");
    need(kOriginalPostFigureType7677Resource600 == 600.0, "dword+72 base for type 76 is 600.0");
    need(kOriginalPostFigureType7677Resource900 == 900.0, "dword+72 base otherwise is 900.0");
    need(kOriginalPostFigureType7677CommitTableVa == 0x00565384u, "unavailable commit table 0x565384");
    need(kOriginalPostFigureType7677CommitTableAvailableVa == 0x0056539Cu,
         "available commit table 0x56539C");

    // 0x007650FC truncates toward zero and answers the indefinite value on overflow.
    need(originalPostFigureType7677Truncate(699.75) == 699, "the conversion truncates");
    need(originalPostFigureType7677Truncate(-50.75) == -50, "truncation goes toward zero");
    need(originalPostFigureType7677Truncate(0.0) == 0, "zero stays zero");
    need(originalPostFigureType7677Truncate(2147483647.5) == 2147483647, "the largest fit survives");
    need(originalPostFigureType7677Truncate(2147483648.0) == int32_t(0x80000000u),
         "an out-of-range value answers the indefinite integer");
    need(originalPostFigureType7677Truncate(-2147483649.0) == int32_t(0x80000000u),
         "a negative out-of-range value answers the indefinite integer");

    // A missing input refuses before anything else, even with an out-of-range index.
    {
        Type7677Pool pool(3);
        Type7677Fixture f(pool);
        f.inputs.probe0x402A50.reset();
        OriginalPostFigureRecordTable records;
        need(run7677(f, records, 2) == OriginalPostFigureType7677Status::RefusedMissingInput,
             "an absent probe 0x00402A50 refuses");
        need(f.flagCalls == 0, "nothing runs before the completeness check");
        Type7677Fixture g(pool);
        g.inputs.stateForIndex.reset();
        need(run7677(g, records, 1) == OriginalPostFigureType7677Status::RefusedMissingInput,
             "an absent state source refuses");
        Type7677Fixture h(pool);
        h.inputs.neighbourForIndex.reset();
        need(run7677(h, records, 1) == OriginalPostFigureType7677Status::RefusedMissingInput,
             "an absent neighbour source refuses");
        Type7677Fixture i(pool);
        i.inputs.flag0x4812E0.reset();
        need(run7677(i, records, 1) == OriginalPostFigureType7677Status::RefusedMissingInput,
             "an absent 0x004812E0 flag refuses");
        Type7677Fixture j(pool);
        j.inputs.probe0x564E90.reset();
        need(run7677(j, records, 1) == OriginalPostFigureType7677Status::RefusedMissingInput,
             "an absent 0x00564E90 gate refuses");
    }

    // The five early leaves do nothing at all: index 0, marker set, gate false,
    // linked type outside 76/77, and the linked state's dword+20 not 4.
    {
        OriginalPostFigureRecordTable records;
        records.records.push_back({76, 1});
        for (int variant = 0; variant < 5; ++variant) {
            Type7677Pool pool(3);
            pool.objects[0].type0x14 = 76;
            pool.objects[1].type0x14 = 76;
            pool.objects[1].word0x16 = 1;
            Type7677Fixture f(pool);
            f.states[0].field0x14 = 4;
            f.neighbours[0].field0x04 = -1;
            f.neighbours[0].field0x08 = -2;
            f.states[0].field0x40 = 100;
            uint32_t index = 1;
            if (variant == 0) index = 0;
            if (variant == 1) f.states[0].byte0x60 = 1;
            if (variant == 2) f.gate = false;
            if (variant == 3) pool.objects[0].type0x14 = 80;
            if (variant == 4) f.states[0].field0x14 = 3;
            need(run7677(f, records, index) == OriginalPostFigureType7677Status::Ok,
                 "every early leave answers ok");
            need(f.states[0].field0x40 == 100, "an early leave charges nothing");
            need(f.asked.empty(), "an early leave asks no resource");
            if (variant != 1) need(f.states[0].byte0x60 == 0, "an early leave writes no marker");
            need(f.flagCalls == 1, "the 0x004812E0 flag is asked first in every case");
            need(f.gateCalls == (variant <= 1 ? 0u : 1u),
                 "the 0x00564E90 gate is asked only after the index and marker leaves");
        }
    }
    // The gate is not asked when the linked state already carries the marker.
    {
        Type7677Pool pool(3);
        pool.objects[0].type0x14 = 76;
        pool.objects[1].type0x14 = 76;
        pool.objects[1].word0x16 = 1;
        Type7677Fixture f(pool);
        f.states[0].byte0x60 = 1;
        OriginalPostFigureRecordTable records;
        need(run7677(f, records, 1) == OriginalPostFigureType7677Status::Ok, "the marker leaves at once");
        need(f.gateCalls == 0 && f.asked.empty(), "the marker leave asks nothing");
    }
    // The gate answers false: the whole step is suppressed.
    {
        Type7677Pool pool(3);
        pool.objects[0].type0x14 = 76;
        pool.objects[1].type0x14 = 76;
        pool.objects[1].word0x16 = 1;
        Type7677Fixture f(pool);
        f.gate = false;
        f.states[0].field0x14 = 4;
        OriginalPostFigureRecordTable records;
        records.records.push_back({76, 1});
        need(run7677(f, records, 1) == OriginalPostFigureType7677Status::Ok, "a closed gate is not a refusal");
        need(f.states[0].byte0x60 == 0, "a closed gate writes no marker");
        need(f.asked.empty(), "a closed gate asks no resource");
    }
    // A skip count of at most zero: only the marker is written.
    for (int variant = 0; variant < 2; ++variant) {
        Type7677Pool pool(3);
        pool.objects[0].type0x14 = 76;
        pool.objects[1].type0x14 = 76;
        pool.objects[1].word0x16 = 1;
        Type7677Fixture f(pool);
        f.states[0].field0x14 = 4;
        f.neighbours[0].field0x04 = -1;
        f.neighbours[0].field0x08 = -2;
        OriginalPostFigureRecordTable records;
        if (variant == 1) records.records.push_back({76, -2});
        need(run7677(f, records, 1) == OriginalPostFigureType7677Status::Ok,
             "a count of at most zero still answers ok");
        need(f.states[0].byte0x60 == 1, "a count of at most zero still writes the marker");
        need(f.asked.empty(), "a count of at most zero charges nothing");
        need(f.states[0].field0x40 == 0, "a count of at most zero charges no dword");
    }

    // One candidate, shape (-1,-2): resource 42 with the flag, 44 without, both
    // charging dword+64 with the 700 or 1100 base that the caller's own type picks.
    // The caller sits at index 1 and the linked object at index 0, so the two types
    // are independent.
    for (int variant = 0; variant < 4; ++variant) {
        Type7677Pool pool(3);
        pool.objects[0].type0x14 = 76;                               // linked type must be 76/77
        pool.objects[1].type0x14 = int16_t(variant >= 2 ? 77 : 76);  // caller type: the base
        pool.objects[1].word0x16 = 1;
        Type7677Fixture f(pool);
        f.states[0].field0x14 = 4;
        f.neighbours[0].field0x04 = -1;
        f.neighbours[0].field0x08 = -2;
        f.states[0].field0x40 = 100;
        f.flag = (variant % 2) == 1;
        OriginalPostFigureRecordTable records;
        records.records.push_back({76, 1});
        need(run7677(f, records, 1) == OriginalPostFigureType7677Status::Ok, "the step completes");
        need(f.asked.size() == 1, "one candidate asks one resource");
        const uint32_t expected = f.flag ? 42u : 44u;
        need(f.asked[0] == expected, "the flag selects resource 42 over 44");
        const int32_t base = variant >= 2 ? 1100 : 700;
        need(f.states[0].field0x40 == 100 + (base - 100),
             "dword+64 is charged with the base minus its current value");
        need(f.states[0].byte0x60 == 1, "the commit writes the marker");
        need(f.states[0].field0x44 == 0 && f.states[0].field0x48 == 0 &&
                 f.states[0].field0x4C == 0 && f.states[0].field0x50 == 0,
             "the other four dwords stay alone");
    }
    // Shape (-1,1) charges dword+76 with 700 only when the caller object is type 77.
    for (int callerType : {76, 77}) {
        Type7677Pool pool(3);
        pool.objects[0].type0x14 = 77;                 // linked type
        pool.objects[1].type0x14 = int16_t(callerType);  // caller type
        pool.objects[1].word0x16 = 1;
        Type7677Fixture f(pool);
        f.states[0].field0x14 = 4;
        f.neighbours[0].field0x04 = -1;
        f.neighbours[0].field0x08 = 1;
        f.states[0].field0x4C = 680;
        OriginalPostFigureRecordTable records;
        records.records.push_back({77, 1});
        need(run7677(f, records, 1) == OriginalPostFigureType7677Status::Ok,
             "the (-1,1) shape completes");
        if (callerType == 77) {
            need(f.asked.size() == 1 && f.asked[0] == 226, "the (-1,1) shape asks resource 226");
            need(f.states[0].field0x4C == 680 + (700 - 680),
                 "dword+76 is charged with 700 minus its value");
        } else {
            need(f.asked.empty(), "a type-76 caller never reaches the (-1,1) dword+76 shape");
            need(f.states[0].field0x4C == 680, "nothing is charged for a type-76 caller");
        }
        need(f.states[0].byte0x60 == 1, "the marker is written in both cases");
    }
    // Shape (1,-2) charges dword+80 with 400 only when the caller object is type 77.
    for (int callerType : {76, 77}) {
        Type7677Pool pool(3);
        pool.objects[0].type0x14 = 77;
        pool.objects[1].type0x14 = int16_t(callerType);
        pool.objects[1].word0x16 = 1;
        Type7677Fixture f(pool);
        f.states[0].field0x14 = 4;
        f.neighbours[0].field0x04 = 1;
        f.neighbours[0].field0x08 = -2;
        f.states[0].field0x50 = 390;
        OriginalPostFigureRecordTable records;
        records.records.push_back({77, 1});
        need(run7677(f, records, 1) == OriginalPostFigureType7677Status::Ok, "the (1,-2) shape completes");
        if (callerType == 77) {
            need(f.asked.size() == 1 && f.asked[0] == 46, "the (1,-2) shape asks resource 46");
            need(f.states[0].field0x50 == 390 + (400 - 390), "dword+80 is charged with 400 minus its value");
        } else {
            need(f.asked.empty(), "a type-76 caller never reaches the (1,-2) dword+80 shape");
        }
    }
    // Shape (2,-2) charges dword+68 with 400 for type 76 and 1000 otherwise.
    for (int callerType : {76, 77}) {
        Type7677Pool pool(3);
        pool.objects[0].type0x14 = 76;
        pool.objects[1].type0x14 = int16_t(callerType);
        pool.objects[1].word0x16 = 1;
        Type7677Fixture f(pool);
        f.states[0].field0x14 = 4;
        f.neighbours[0].field0x04 = 2;
        f.neighbours[0].field0x08 = -2;
        f.states[0].field0x44 = 50;
        OriginalPostFigureRecordTable records;
        records.records.push_back({76, 1});
        need(run7677(f, records, 1) == OriginalPostFigureType7677Status::Ok, "the (2,-2) shape completes");
        need(f.asked.size() == 1 && f.asked[0] == 47, "the (2,-2) shape asks resource 47");
        const int32_t base = callerType == 76 ? 400 : 1000;
        need(f.states[0].field0x44 == 50 + (base - 50),
             "dword+68 is charged with 400 or 1000 minus its value");
        need(f.states[0].field0x48 == 0, "the (2,-2) shape leaves dword+72 alone");
    }
    // Shape (2,1) charges dword+72 with 600 for type 76 and 900 otherwise, and a
    // single candidate with (2,1) also runs the (2,-2) block first, which finds
    // dword+8 != -2 and charges nothing.
    for (int callerType : {76, 77}) {
        Type7677Pool pool(3);
        pool.objects[0].type0x14 = 76;
        pool.objects[1].type0x14 = int16_t(callerType);
        pool.objects[1].word0x16 = 1;
        Type7677Fixture f(pool);
        f.states[0].field0x14 = 4;
        f.neighbours[0].field0x04 = 2;
        f.neighbours[0].field0x08 = 1;
        f.states[0].field0x48 = 40;
        OriginalPostFigureRecordTable records;
        records.records.push_back({76, 1});
        need(run7677(f, records, 1) == OriginalPostFigureType7677Status::Ok, "the (2,1) shape completes");
        need(f.asked.size() == 1 && f.asked[0] == 43, "the (2,1) shape asks resource 43");
        const int32_t base = callerType == 76 ? 600 : 900;
        need(f.states[0].field0x48 == 40 + (base - 40),
             "dword+72 is charged with 600 or 900 minus its value");
        need(f.states[0].field0x44 == 0, "the (2,-2) block stays out of the (2,1) case");
    }
    // An unrecognised dword+4 charges nothing but still marks the state.
    {
        Type7677Pool pool(3);
        pool.objects[0].type0x14 = 76;
        pool.objects[1].type0x14 = 76;
        pool.objects[1].word0x16 = 1;
        Type7677Fixture f(pool);
        f.states[0].field0x14 = 4;
        f.neighbours[0].field0x04 = 7;
        f.neighbours[0].field0x08 = -2;
        OriginalPostFigureRecordTable records;
        records.records.push_back({76, 1});
        need(run7677(f, records, 1) == OriginalPostFigureType7677Status::Ok, "an unknown shape completes");
        need(f.asked.empty(), "an unknown dword+4 asks no resource");
        need(f.states[0].byte0x60 == 1, "an unknown dword+4 still marks the state");
    }
    // A non-positive value never beats the zero seed, so nothing is charged but
    // the candidate still counts as unavailable.
    {
        Type7677Pool pool(3);
        pool.objects[0].type0x14 = 76;
        pool.objects[1].type0x14 = 76;
        pool.objects[1].word0x16 = 1;
        Type7677Fixture f(pool);
        f.states[0].field0x14 = 4;
        f.neighbours[0].field0x04 = -1;
        f.neighbours[0].field0x08 = -2;
        f.states[0].field0x40 = 900;  // 700 - 900 = -200
        f.refusing.push_back(44);
        OriginalPostFigureRecordTable records;
        records.records.push_back({76, 1});
        need(run7677(f, records, 1) == OriginalPostFigureType7677Status::Ok, "a negative value completes");
        need(f.states[0].field0x40 == 900, "a negative value charges nothing");
        need(f.states[0].byte0x60 == 1, "a negative value still marks the state");
    }

    // Two candidates: the unavailable one wins even though the available one has
    // the larger value. Candidate 0 is (2,1) with a large charge, candidate 1 is
    // (-1,-2) with a small one and its resource refuses.
    {
        Type7677Pool pool(4);
        pool.objects[2].type0x14 = 76;   // caller type selects the 700 base
        pool.objects[2].word0x16 = 1;   // linked index 1
        pool.objects[1].type0x14 = 76;
        Type7677Fixture f(pool);
        f.states[1].field0x14 = 4;
        f.neighbours[1].field0x04 = 2;
        f.neighbours[1].field0x08 = 1;
        f.states[1].field0x48 = 590;   // 600 - 590 = 10, available, resource 43
        f.neighbours[2].field0x04 = -1;
        f.neighbours[2].field0x08 = -2;
        f.states[2].field0x40 = 690;   // 700 - 690 = 10, unavailable, resource 44
        f.refusing.push_back(44);
        OriginalPostFigureRecordTable records;
        records.records.push_back({76, 2});
        need(run7677(f, records, 2) == OriginalPostFigureType7677Status::Ok, "two candidates complete");
        need(f.asked.size() == 2 && f.asked[0] == 43 && f.asked[1] == 44,
             "both candidates are asked in index order, with resource 44 without the flag");
        need(f.states[1].field0x48 == 590, "the available candidate is not charged when one is unavailable");
        need(f.states[2].field0x40 == 690 + 10, "the unavailable candidate is charged instead");
    }
    // With no unavailable candidate the best available one is charged, and only
    // the best: a smaller available value is dropped.
    {
        Type7677Pool pool(4);
        pool.objects[2].type0x14 = 76;   // caller type selects the 700 base
        pool.objects[2].word0x16 = 1;
        pool.objects[1].type0x14 = 76;
        Type7677Fixture f(pool);
        f.states[1].field0x14 = 4;
        f.neighbours[1].field0x04 = -1;
        f.neighbours[1].field0x08 = -2;
        f.states[1].field0x40 = 650;   // value 50
        f.neighbours[2].field0x04 = -1;
        f.neighbours[2].field0x08 = -2;
        f.states[2].field0x40 = 680;   // value 20
        OriginalPostFigureRecordTable records;
        records.records.push_back({76, 2});
        need(run7677(f, records, 2) == OriginalPostFigureType7677Status::Ok, "two available candidates complete");
        need(f.states[1].field0x40 == 650 + 50, "the larger available value is charged");
        need(f.states[2].field0x40 == 680, "the smaller available value is dropped");
    }
    // A tie keeps the first candidate: the comparison is strict.
    {
        Type7677Pool pool(4);
        pool.objects[2].type0x14 = 76;   // caller type selects the 700 base
        pool.objects[2].word0x16 = 1;
        pool.objects[1].type0x14 = 76;
        Type7677Fixture f(pool);
        f.states[1].field0x14 = 4;
        f.neighbours[1].field0x04 = -1;
        f.neighbours[1].field0x08 = -2;
        f.states[1].field0x40 = 200;
        f.neighbours[2].field0x04 = -1;
        f.neighbours[2].field0x08 = -2;
        f.states[2].field0x40 = 200;
        f.refusing.push_back(44);
        OriginalPostFigureRecordTable records;
        records.records.push_back({76, 2});
        need(run7677(f, records, 2) == OriginalPostFigureType7677Status::Ok, "a tie completes");
        need(f.states[1].field0x40 == 200 + 500, "the first candidate wins the tie");
        need(f.states[2].field0x40 == 200, "the second candidate is not charged");
    }
    // The candidate count bounds the scan: a count of 1 ignores the second candidate.
    {
        Type7677Pool pool(4);
        pool.objects[2].type0x14 = 76;   // caller type selects the 700 base
        pool.objects[2].word0x16 = 1;
        pool.objects[1].type0x14 = 76;
        Type7677Fixture f(pool);
        f.states[1].field0x14 = 4;
        f.neighbours[1].field0x04 = -1;
        f.neighbours[1].field0x08 = -2;
        f.neighbours[2].field0x04 = -1;
        f.neighbours[2].field0x08 = -2;
        f.states[2].field0x40 = 100;
        OriginalPostFigureRecordTable records;
        records.records.push_back({76, 1});
        need(run7677(f, records, 2) == OriginalPostFigureType7677Status::Ok, "a one-candidate scan completes");
        need(f.asked.size() == 1, "only the counted candidates are asked");
        need(f.states[2].field0x40 == 100, "the uncounted candidate is untouched");
    }
    // The linked index is index - word[object+22], so a non-zero word moves it.
    {
        Type7677Pool pool(5);
        pool.objects[3].type0x14 = 76;
        pool.objects[3].word0x16 = 2;   // linked index 1
        pool.objects[1].type0x14 = 76;
        Type7677Fixture f(pool);
        f.states[1].field0x14 = 4;
        f.neighbours[1].field0x04 = -1;
        f.neighbours[1].field0x08 = -2;
        f.states[1].field0x40 = 600;    // value 100
        OriginalPostFigureRecordTable records;
        records.records.push_back({76, 1});
        need(run7677(f, records, 3) == OriginalPostFigureType7677Status::Ok, "a shifted linked index completes");
        need(f.states[1].field0x40 == 600 + 100, "the shifted candidate is charged");
        need(f.states[3].field0x40 == 0, "the calling slot's own state is untouched");
    }
    // Contract refusals: no silent null dereference.
    {
        OriginalPostFigureRecordTable records;
        records.records.push_back({76, 1});
        Type7677Pool pool(3);
        pool.objects[1].type0x14 = 76;
        pool.objects[1].word0x16 = 4;  // linked index -3
        Type7677Fixture f(pool);
        need(run7677(f, records, 1) == OriginalPostFigureType7677Status::RefusedIndexOutsidePool,
             "a linked index below zero refuses");
        Type7677Pool far(2);
        far.objects[1].type0x14 = 76;
        far.objects[1].word0x16 = -8;  // linked index 9, outside a two-slot pool
        Type7677Fixture g(far);
        need(run7677(g, records, 1) == OriginalPostFigureType7677Status::RefusedIndexOutsidePool,
             "a linked index beyond the pool refuses");
        Type7677Pool wide(6);
        wide.objects[1].word0x16 = -9;
        Type7677Fixture h(wide);
        need(run7677(h, records, 1) == OriginalPostFigureType7677Status::RefusedIndexOutsidePool,
             "a wide linked index refuses before any state lookup");
        Type7677Pool nullSlot(3);
        nullSlot.objects[1].type0x14 = 76;
        nullSlot.objects[1].word0x16 = 1;
        nullSlot.slots[0] = nullptr;
        Type7677Fixture i(nullSlot);
        need(run7677(i, records, 1) == OriginalPostFigureType7677Status::RefusedNullObject,
             "a null linked slot refuses");
        Type7677Pool noState(3);
        noState.objects[1].type0x14 = 76;
        Type7677Fixture j(noState);
        j.states.clear();  // the state source answers null for every index
        j.inputs.stateForIndex = [](uint32_t) -> OriginalPostFigureType7677State* { return nullptr; };
        need(run7677(j, records, 1) == OriginalPostFigureType7677Status::RefusedNullState,
             "a null state refuses");
        Type7677Pool noNeighbour(3);
        noNeighbour.objects[1].type0x14 = 76;
        Type7677Fixture k(noNeighbour);
        k.states[1].field0x14 = 4;
        k.inputs.neighbourForIndex =
            [](uint32_t) -> const OriginalPostFigureType7677Neighbour* { return nullptr; };
        need(run7677(k, records, 1) == OriginalPostFigureType7677Status::RefusedNullNeighbour,
             "a null neighbour record refuses instead of dereferencing null");
        need(run7677(k, records, 0) == OriginalPostFigureType7677Status::Ok,
             "index 0 leaves before the neighbour lookup");
    }
}

// ---------------------------------------------------------------------------
// The pass with the ported types-76/77 step wired in
// ---------------------------------------------------------------------------
void passTypes7677Tests() {
    // 0x00564B50 rejects any object whose word+22 is non-zero (0x00564BA4), so
    // every 0x00564EA0 invocation from the pass has word+22 == 0 and so links to
    // its own index: the linked object and the state are the scanned slot's.
    {
        Type7677Pool pool(3);
        pool.objects[1].type0x14 = 76;
        pool.objects[1].live0x04 = 1;
        Type7677Fixture f(pool);
        f.states[1].field0x14 = 4;
        f.neighbours[1].field0x04 = -1;
        f.neighbours[1].field0x08 = -2;
        f.states[1].field0x40 = 600;   // 700 - 600 = 100

        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        OriginalPostFigureRecordTable table;
        table.records.push_back({76, 1});
        in.recordTable0x12A5DD0 = table;
        in.types7677_0x564EA0 = f.inputs;
        in.updateRecord0x56C880 = [](const void*, uint32_t) {};
        in.stepRecord0x56D170 = [](const void*, uint32_t) { return false; };

        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::Ok,
             "the ported 0x00564EA0 lets the pass finish the slot");
        need(f.states[1].field0x40 == 600 + 100, "the ported step charged the scanned state");
        need(f.states[1].byte0x60 == 1, "the ported step wrote the marker");
        need(f.asked.size() == 1 && f.asked[0] == 44,
             "no flag and a type-76 caller ask resource 44");
        need(f.gateCalls == 1, "the ported step asked the gate once");
        need(state.counter == 0 && state.period == 30, "the scan reset the countdown");
    }
    // A type-77 slot reaches the same invocation; the caller type 77 selects 1100
    // base and resource 42 when the flag is set.
    {
        Type7677Pool pool(3);
        pool.objects[1].type0x14 = 77;
        pool.objects[1].live0x04 = 3;
        Type7677Fixture f(pool);
        f.flag = true;
        f.states[1].field0x14 = 4;
        f.neighbours[1].field0x04 = -1;
        f.neighbours[1].field0x08 = -2;
        f.states[1].field0x40 = 100;

        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        OriginalPostFigureRecordTable table;
        table.records.push_back({77, 1});
        in.recordTable0x12A5DD0 = table;
        in.types7677_0x564EA0 = f.inputs;
        in.updateRecord0x56C880 = [](const void*, uint32_t) {};
        in.stepRecord0x56D170 = [](const void*, uint32_t) { return false; };

        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::Ok, "the type-77 slot completes");
        need(f.asked.size() == 1 && f.asked[0] == 42, "the flag selects resource 42");
        need(f.states[1].field0x40 == 100 + (1100 - 100),
             "the type-77 caller selects the 1100 base");
    }
    // A null state surfaces as its own named refusal, not as a silent success and
    // not as the missing-provider refusal.
    {
        Type7677Pool pool(3);
        pool.objects[1].type0x14 = 76;
        pool.objects[1].live0x04 = 1;
        Type7677Fixture f(pool);
        f.inputs.stateForIndex =
            [](uint32_t) -> OriginalPostFigureType7677State* { return nullptr; };
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        OriginalPostFigureRecordTable table;
        table.records.push_back({76, 1});
        in.recordTable0x12A5DD0 = table;
        in.types7677_0x564EA0 = f.inputs;
        in.updateRecord0x56C880 = [](const void*, uint32_t) {};
        in.stepRecord0x56D170 = [](const void*, uint32_t) { return false; };
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        const auto r = advanceOriginalPostFigurePass(state, in);
        need(r.status == OriginalPostFigurePassStatus::Refused564EA0Contract,
             "a null state refuses under the contract name");
        need(state.counter == 31 && state.period == 30,
             "the contract refusal keeps the stored increment and the old period");
    }
    // Incomplete types-76/77 inputs keep the missing-provider refusal.
    {
        Type7677Pool pool(3);
        pool.objects[1].type0x14 = 76;
        pool.objects[1].live0x04 = 1;
        Type7677Fixture f(pool);
        f.inputs.probe0x564E90.reset();
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        OriginalPostFigureRecordTable table;
        table.records.push_back({76, 1});
        in.recordTable0x12A5DD0 = table;
        in.types7677_0x564EA0 = f.inputs;
        in.updateRecord0x56C880 = [](const void*, uint32_t) {};
        in.stepRecord0x56D170 = [](const void*, uint32_t) { return false; };
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        need(advanceOriginalPostFigurePass(state, in).status ==
                 OriginalPostFigurePassStatus::RefusedUnported564EA0,
             "incomplete ported inputs keep the named unported refusal");
        need(f.gateCalls == 0, "an incomplete step is never entered");
    }
    // The types-76/77 port needs the record array for its nested 0x00567610; with
    // the array absent the pass refuses before calling it.
    {
        Type7677Pool pool(3);
        pool.objects[1].type0x14 = 76;
        pool.objects[1].live0x04 = 1;
        Type7677Fixture f(pool);
        OriginalPostFigurePassInputs in = session();
        in.registry = pool.slots;
        in.types7677_0x564EA0 = f.inputs;
        in.indexRecord0x5653C0 = [](const OriginalPostFigureRegistryObject&, uint32_t) {
            return static_cast<const void*>(nullptr);
        };
        in.skipAdvance0x567610 = [](int32_t) { return 1u; };
        OriginalPostFigurePassState state;
        state.counter = 30;
        state.period = 30;
        need(advanceOriginalPostFigurePass(state, in).status ==
                 OriginalPostFigurePassStatus::RefusedUnported564EA0,
             "the ported step refuses when the record array is missing");
        need(f.gateCalls == 0, "the ported step is not entered without the array");
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
        recordTableTests();
        updateStepPrefixTests();
        types7677Tests();
        passTypes7677Tests();
        scanTests();
        std::cout << "{\"passed\":true,\"authored_checks\":" << checks << "}\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
