#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace game_lab {
// All executable facts below come from the user's own Emperor.exe, SHA-256
// 0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658, image base
// 0x400000; file offset = VA - 0x400000. See docs/ORIGINAL_TICK_WIRING_AUDIT.md,
// section "剩余两步的深度映射：0x466E90 与 0x564B50", subsections 2.2-2.5.
//
// Scope: the 0x00564B50 skeleton (counter + 30/50 period + ordered pool scan +
// type-based skip advance) and the 0x00562F70 type whitelist. Every callee that is
// still unported is a caller-supplied provider; when a provider is absent the pass
// stops with a named refusal instead of guessing the callee's behaviour. No
// environment, file or clock is read here.
//
// ---------------------------------------------------------------------------
// 0x00562F70 whitelist (type predicate)
// ---------------------------------------------------------------------------
// VERIFIED from the executable:
//  * The argument is reduced by 76 at 0x00562F74 and compared with 192 unsigned at
//    0x00562F77; a value above 192 leaves through 0x00562F7C and 0x00562F90, which
//    answers false. The reduction is 32-bit, so every argument below 76 (including
//    sign-extended negative words) wraps high and is rejected before the table.
//  * Otherwise the byte at 0x00562F9C + (type-76) becomes the index into the
//    two-entry dword table at 0x00562F94 (table read at 0x00562F80, dispatch at
//    0x00562F86). Entry 0 is the code at 0x00562F8D, which answers true; entry 1 is
//    the code at 0x00562F90, which answers false.
//  * The table is 193 bytes at VA 0x00562F9C = file offset 0x00162F9C, covering the
//    type range 76..268 inclusive. Bytes read back from the file are only 0 or 1:
//    29 zeros and 164 ones. A zero accepts the type, a one rejects it.
//  * Accepted types: 76..86 (11), 92-93 (2), 253..268 (16) = 29 of 193.
//    Rejected types: 87..91 (5) and 94..252 (159) = 164 of 193.
inline constexpr uint32_t kOriginalPostFigureWhitelistFirstType = 76;
inline constexpr uint32_t kOriginalPostFigureWhitelistLastType = 268;
inline constexpr size_t kOriginalPostFigureWhitelistSize = 193;
inline constexpr size_t kOriginalPostFigureWhitelistAcceptedCount = 29;
inline constexpr size_t kOriginalPostFigureWhitelistRejectedCount = 164;
inline constexpr uint32_t kOriginalPostFigureWhitelistTableVa = 0x00562F9Cu;
inline constexpr uint32_t kOriginalPostFigureWhitelistTableFileOffset = 0x00162F9Cu;
inline constexpr uint32_t kOriginalPostFigureWhitelistJumpTableVa = 0x00562F94u;

// Raw table bytes as stored at 0x00562F9C..0x0056305C (file 0x00162F9C..0x0016305C).
// Index i is type 76+i; value 0 selects the accepting answer of 0x00562F8D, value 1
// the rejecting answer of 0x00562F90. Rows are 16 bytes, in file order:
//   0x00: 11 zeros (76..86 accept), then five ones (87..91 reject)
//   0x10: 92-93 accept, 94..107 reject
//   0x20..0xA0: reject (108..175)
//   0xB0: 252 reject, 253..268 accept (the tail of the row is 0x00)
//   0xC0: 268 accept
inline constexpr std::array<uint8_t, kOriginalPostFigureWhitelistSize> kOriginalPostFigureWhitelistIndex{{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,
}};

// One 0x00562F70 invocation. The argument is a 32-bit value, so a type below 76
// wraps and is rejected; the table is only consulted for 76..268.
inline bool originalPostFigureWhitelisted(uint32_t type) {
    const uint32_t index = type - kOriginalPostFigureWhitelistFirstType;
    if (index > kOriginalPostFigureWhitelistSize - 1) return false;  // 0x00562F77 range refusal
    // Two-entry dispatch: a zero byte selects the accepting answer of 0x00562F8D.
    if (kOriginalPostFigureWhitelistIndex[index] == 0) return true;
    return false;
}

// ---------------------------------------------------------------------------
// 0x00564B50 post-figure periodic pass
// ---------------------------------------------------------------------------
// VERIFIED from the executable: the body runs 0x00564B50..0x00564C50 and leaves
// through its single exit at 0x00564C50 (0x00564C51..0x00564C5F is padding). Its only
// entry is the tail jump from 0x005371DC, and 0x005371D3 reads the session gate byte
// 0x0088EC38 and leaves at 0x005371DA when it is non-zero, so the body is never
// entered in that case.
//
// Countdown and period (two dwords, each referenced exactly three times in the whole
// listing, all inside this body):
//  * 0x012A6E78 is the scan countdown. It is read at 0x00564B50, incremented at
//    0x00564B5B and stored at 0x00564B5E; the store happens before the compare, so
//    the incremented value is already visible when the pass exits early.
//  * 0x00859DA0 is the current period, read at 0x00564B55. The comparison at
//    0x00564B5C/0x00564B63 is signed: when the incremented countdown is less than or
//    equal to the period, control leaves at 0x00564C50 without scanning. The scan
//    therefore runs on the invocation whose incremented value is strictly greater
//    than the period.
//  * After a scan, 0x00564C2D writes the countdown back to zero, 0x00564C39 writes
//    the period 30 and 0x00564C46 overwrites it with 50 when the progress flag is
//    set. So the visible period is 30 without progress and 50 with progress, and the
//    next window is 31 or 51 invocations from a zero countdown.
//
// Body:
//  * 0x00564B6B loads the registry object 0x008C7634, 0x00564B70 clears the progress
//    flag and 0x00564B72 starts the scan index at 1.
//  * 0x00564B77 asks 0x00554C00 for the slot count (the registry's end pointer minus
//    its begin pointer, divided by 4, zero when the registry is empty); 0x00564B7C/0x00564B7E leave for the finish block unsigned
//    when that count is at most 1, so the finish block runs even with no slot.
//  * The ascending loop is 0x00564B86..0x00564C24, with the slot count re-read
//    through 0x00554C00 at 0x00564C1D and the unsigned continuation decision at
//    0x00564C22/0x00564C24.
//  * 0x00564B8C resolves the registry slot through 0x0047F1B0 and dereferences the
//    slot dword at 0x00564B91; the exe performs no null check on it.
//  * 0x00564B93 reads the signed word at [obj+20] as the type. 0x00564B98 asks
//    0x00562F70; a false answer jumps to 0x00564C17, which increments the scan index
//    by one and performs no type advance.
//  * 0x00564BA4 rejects a non-zero word at [obj+22] and 0x00564BAF asks 0x00426D10
//    (the live byte at [obj+4] must be 1 or 3); either rejection also goes to
//    0x00564C17 with no type advance.
//  * 0x00564BBB asks 0x005653C0 for the 0x012A5DD0 record and keeps it in a local. A
//    null record jumps to 0x00564BEE.
//  * 0x00564BC6 reads the pause byte 0x010DE0D0 and 0x00564BCF compares the mode byte
//    0x010DE045 with 86; a non-zero pause byte or mode 86 also jumps to 0x00564BEE,
//    so only the entity work is skipped.
//  * 0x00564BDB asks 0x0056C880 and 0x00564BE3 asks 0x0056D170; a non-zero answer
//    from the second sets the progress flag at 0x00564BEC.
//  * 0x00564BEE tests the type and 0x00564C01 asks 0x00564EA0 for types 76 and 77.
//  * 0x00564C0B asks 0x00567610 for the skip count of the type; 0x00564C13 subtracts
//    one from that answer and the shared 0x00564C1C step adds one back, so the net
//    type advance is the returned count.
//
// Not serialized (all rejected as save-provided state):
//  * 0x012A6E78 and 0x00859DA0: each occurs exactly three times in the listing (the
//    addresses above) and none of them is a SAV19 read request. The SAV19 branch
//    0x005311DC..0x00531873 contains 91 read requests of the shape size immediate,
//    address immediate, then the shared read entry point at 0x00780533; searching
//    that range for 0x012A6E78, 0x00859DA0, 0x010DE0D0, 0x010DE045 or 0x0088EC38
//    finds nothing. They are process/session state.
//  * 0x010DE0D0 (16 references) and 0x010DE045 (391 references) are session bytes
//    written from many other paths; no SAV19 request names either.
//  * 0x0088EC38 (182 references, 14 writer sites per the audit) is the session gate
//    read by the caller at 0x005371B3 and 0x005371D3, not by this body.
// A loaded save therefore cannot supply any of the five; the caller must state them.

enum class OriginalPostFigurePassStatus {
    Ok,                          // left through the single exit at 0x00564C50
    SkippedGate,                 // 0x005371D3 gate non-zero: body not entered
    RefusedMissingSessionInput,  // a required session field was not supplied
    RefusedNullRegistrySlot,     // the slot dword read at 0x00564B91 is null
    RefusedUnported5653C0,       // 0x00564BBB
    RefusedUnported56C880,       // 0x00564BDB
    RefusedUnported56D170,       // 0x00564BE3
    RefusedUnported564EA0,       // 0x00564C01
    RefusedUnported567610,       // 0x00564C0B
    Refused567610ZeroAdvance,    // 0x00567610 answered 0: the index would not advance
};

inline const char* originalPostFigurePassStatusName(OriginalPostFigurePassStatus status) {
    switch (status) {
        case OriginalPostFigurePassStatus::Ok: return "ok";
        case OriginalPostFigurePassStatus::SkippedGate: return "skipped_gate_88ec38";
        case OriginalPostFigurePassStatus::RefusedMissingSessionInput: return "refused_missing_session_input";
        case OriginalPostFigurePassStatus::RefusedNullRegistrySlot: return "refused_null_registry_slot";
        case OriginalPostFigurePassStatus::RefusedUnported5653C0: return "refused_unported_5653c0";
        case OriginalPostFigurePassStatus::RefusedUnported56C880: return "refused_unported_56c880";
        case OriginalPostFigurePassStatus::RefusedUnported56D170: return "refused_unported_56d170";
        case OriginalPostFigurePassStatus::RefusedUnported564EA0: return "refused_unported_564ea0";
        case OriginalPostFigurePassStatus::RefusedUnported567610: return "refused_unported_567610";
        case OriginalPostFigurePassStatus::Refused567610ZeroAdvance: return "refused_567610_zero_advance";
    }
    return "unknown";
}

// The two live globals. 0x012A6E78 is the countdown, 0x00859DA0 the current period.
// Neither has a save representation; see the "not serialized" note above.
struct OriginalPostFigurePassState {
    uint32_t counter = 0;  // 0x012A6E78
    uint32_t period = 0;   // 0x00859DA0
};

// The fields the exe reads at [obj+20], [obj+22] and [obj+4] through 0x0047F1B0 and
// 0x00426D10, named by offset. Nothing else about the object is consumed here.
struct OriginalPostFigureRegistryObject {
    int16_t type0x14 = 0;      // word[obj+20], signed, read at 0x00564B93
    int16_t word0x16 = 0;      // word[obj+22], must be zero at 0x00564BA4
    uint8_t live0x04 = 0;      // byte[obj+4], 0x00426D10 accepts 1 and 3
};

// Session inputs. Every field is std::optional and has no default value: an absent
// field refuses, it is never replaced by 0 or by a save byte. The first five are the
// process/session state the SAV19 window does not carry; the five providers are the
// callees this skeleton does not implement. `registry` is the 0x008C7634 pool as the
// caller sees it: index i holds *[0x0047F1B0(0x008C7634, i)] and its size is
// 0x00554C00's answer. The skeleton never mutates it, so re-reading the count each
// iteration (0x00564C1D) sees the same value.
struct OriginalPostFigurePassInputs {
    std::optional<uint8_t> gate0x88EC38;      // caller gate at 0x005371D3
    std::optional<uint8_t> pause0x10DE0D0;    // 0x00564BC6
    std::optional<uint8_t> mode0x10DE045;     // 0x00564BCF, skip entity work when 86
    std::optional<uint32_t> counterInitial0x12A6E78;  // session-start countdown
    std::optional<uint32_t> periodInitial0x859DA0;    // session-start period
    std::vector<const OriginalPostFigureRegistryObject*> registry;  // 0x008C7634

    // 0x005653C0(object, index): the 0x012A5DD0 record for the owner type, or null.
    std::optional<std::function<const void*(const OriginalPostFigureRegistryObject&, uint32_t)>> indexRecord0x5653C0;
    // 0x0056C880(record, index): entity work, answer unused by the caller.
    std::optional<std::function<void(const void*, uint32_t)>> updateRecord0x56C880;
    // 0x0056D170(record, index): bool, sets the progress flag that selects period 50.
    std::optional<std::function<bool(const void*, uint32_t)>> stepRecord0x56D170;
    // 0x00564EA0(object, index): only reached for types 76 and 77.
    std::optional<std::function<void(const OriginalPostFigureRegistryObject&, uint32_t)>> handleTypes76_77_0x564EA0;
    // 0x00567610(type): the [rec+12] skip count for the type.
    std::optional<std::function<uint32_t(int32_t)>> skipAdvance0x567610;
};

struct OriginalPostFigurePassResult {
    OriginalPostFigurePassStatus status = OriginalPostFigurePassStatus::RefusedMissingSessionInput;
    bool due = false;       // the increment exceeded the period, body entered
    bool progress = false;  // progress flag, selects the 50 period
    uint32_t visited = 0;   // loop bodies entered
    uint32_t index = 0;     // scan index at the refusal/exit
};

// True when every field the pass requires has been supplied. The two counter
// initial values are part of the contract even though 0x00564B50 reads them only
// through the live state: a pass is never allowed to run on a session whose initial
// countdown/period were never declared.
inline bool originalPostFigurePassInputsComplete(const OriginalPostFigurePassInputs& inputs) {
    return inputs.gate0x88EC38.has_value() && inputs.pause0x10DE0D0.has_value() &&
           inputs.mode0x10DE045.has_value() && inputs.counterInitial0x12A6E78.has_value() &&
           inputs.periodInitial0x859DA0.has_value();
}

// Seeds the two live globals from the required session values. This is the only
// sanctioned way to build a state; absent inputs return nullopt.
inline std::optional<OriginalPostFigurePassState> originalPostFigurePassSessionState(
    const OriginalPostFigurePassInputs& inputs) {
    if (!inputs.counterInitial0x12A6E78 || !inputs.periodInitial0x859DA0) return std::nullopt;
    OriginalPostFigurePassState state;
    state.counter = *inputs.counterInitial0x12A6E78;
    state.period = *inputs.periodInitial0x859DA0;
    return state;
}

// One 0x00564B50 invocation. The countdown block (0x00564B50..0x00564B63) runs first,
// so a refusal raised later leaves the already-stored increment in place and the
// reset at 0x00564C2D / period writes at 0x00564C39 and 0x00564C46 unreached.
inline OriginalPostFigurePassResult advanceOriginalPostFigurePass(
    OriginalPostFigurePassState& state, const OriginalPostFigurePassInputs& inputs) {
    OriginalPostFigurePassResult result;
    if (!originalPostFigurePassInputsComplete(inputs)) {
        result.status = OriginalPostFigurePassStatus::RefusedMissingSessionInput;
        return result;
    }
    if (*inputs.gate0x88EC38 != 0) {  // 0x005371D3: the caller skips the pass entirely
        result.status = OriginalPostFigurePassStatus::SkippedGate;
        return result;
    }

    // Increment at 0x00564B5B, store at 0x00564B5E, signed comparison at 0x00564B63.
    const uint32_t next = state.counter + 1u;
    state.counter = next;
    const auto signedValue = [](uint32_t value) {
        return value > 0x7FFFFFFFu ? int32_t(int64_t(value) - 4294967296LL) : int32_t(value);
    };
    if (signedValue(next) <= signedValue(state.period)) {
        result.status = OriginalPostFigurePassStatus::Ok;
        return result;
    }
    result.due = true;

    const uint32_t count = uint32_t(inputs.registry.size());  // 0x00564B77
    uint32_t cursor = 1;                                      // 0x00564B72
    if (count > 1) {  // unsigned short circuit at 0x00564B7C/0x00564B7E
        for (;;) {
            ++result.visited;
            result.index = cursor;
            const OriginalPostFigureRegistryObject* object = inputs.registry[cursor];
            if (!object) {
                // 0x00564B91 dereferences the slot dword without a null check; a null
                // slot is an input contract violation, not something to dereference.
                result.status = OriginalPostFigurePassStatus::RefusedNullRegistrySlot;
                return result;
            }
            const int32_t type = object->type0x14;
            bool advanced = false;
            for (;;) {
                if (!originalPostFigureWhitelisted(uint32_t(int64_t(type)))) break;  // 0x00564B98
                if (object->word0x16 != 0) break;                                    // 0x00564BA4
                if (object->live0x04 != 1 && object->live0x04 != 3) break;           // 0x00564BAF
                if (!inputs.indexRecord0x5653C0) {
                    result.status = OriginalPostFigurePassStatus::RefusedUnported5653C0;
                    return result;
                }
                const void* record = (*inputs.indexRecord0x5653C0)(*object, cursor);
                if (record) {  // 0x00564BC2 leaves for 0x00564BEE when the record is null
                    if (*inputs.pause0x10DE0D0 == 0 && *inputs.mode0x10DE045 != 86) {
                        if (!inputs.updateRecord0x56C880) {
                            result.status = OriginalPostFigurePassStatus::RefusedUnported56C880;
                            return result;
                        }
                        (*inputs.updateRecord0x56C880)(record, cursor);
                        if (!inputs.stepRecord0x56D170) {
                            result.status = OriginalPostFigurePassStatus::RefusedUnported56D170;
                            return result;
                        }
                        if ((*inputs.stepRecord0x56D170)(record, cursor)) result.progress = true;
                    }
                }
                if (type == 76 || type == 77) {  // 0x00564BF2/0x00564BFC reach 0x00564C01
                    if (!inputs.handleTypes76_77_0x564EA0) {
                        result.status = OriginalPostFigurePassStatus::RefusedUnported564EA0;
                        return result;
                    }
                    (*inputs.handleTypes76_77_0x564EA0)(*object, cursor);
                }
                if (!inputs.skipAdvance0x567610) {
                    result.status = OriginalPostFigurePassStatus::RefusedUnported567610;
                    return result;
                }
                const uint32_t advance = (*inputs.skipAdvance0x567610)(type);
                if (advance == 0) {
                    // 0x00564C13 subtracts one from the answer and 0x00564C1C adds one
                    // back: a zero count leaves the scan index where it was, so the exe
                    // would repeat this slot forever. Treat the missing table entry as
                    // a named refusal rather than silently advancing by one.
                    result.status = OriginalPostFigurePassStatus::Refused567610ZeroAdvance;
                    return result;
                }
                cursor += advance;  // net of the -1 and the shared increment at 0x00564C1C
                advanced = true;
                break;
            }
            if (!advanced) ++cursor;  // 0x00564C17 path: increment by one only
            if (cursor >= uint32_t(inputs.registry.size())) break;  // 0x00564C22 unsigned below
        }
    }

    state.counter = 0;                            // 0x00564C2D
    state.period = result.progress ? 50u : 30u;   // 0x00564C39, overwritten at 0x00564C46
    result.status = OriginalPostFigurePassStatus::Ok;
    return result;
}
}  // namespace game_lab
