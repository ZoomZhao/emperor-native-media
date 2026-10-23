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
// type-based skip advance), the 0x00562F70 type whitelist, and the callees ported
// in this header: the 0x005653C0 record lookup, the 0x00567610 skip advance (both
// over the 0x012A5DD0 array), the 0x00564EA0 types-76/77 step, and the exact
// prefixes of 0x0056C880 (the two record profiles plus the relative-index leave)
// and 0x0056D170 (the same entry leave). Every callee that is still unported is a
// caller-supplied provider; when a provider is absent the pass stops with a named
// refusal instead of guessing the callee's behaviour. No environment, file or
// clock is read here.
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

// The fields the exe reads at [obj+20], [obj+22] and [obj+4] through 0x0047F1B0 and
// 0x00426D10, named by offset. Nothing else about the object is consumed here.
struct OriginalPostFigureRegistryObject {
    int16_t type0x14 = 0;      // word[obj+20], signed, read at 0x00564B93
    int16_t word0x16 = 0;      // word[obj+22], must be zero at 0x00564BA4
    uint8_t live0x04 = 0;      // byte[obj+4], 0x00426D10 accepts 1 and 3
};

// ---------------------------------------------------------------------------
// 0x0056C880 and 0x0056D170: the exact part, and what stays refused
// ---------------------------------------------------------------------------
// VERIFIED from the executable:
//  * 0x0056C880 opens with 0x0056A910 on the record (0x0056C88B), asks
//    0x00564E80 (0x0056C890) and, when that answers true, calls 0x0056A940 on the
//    record (0x0056C89B). Both are pure writers of seven dwords at +16..+40 and
//    nothing else in them is conditional.
//  * It then reads registry[index] (0x0056C8AA) and forms index - word[object+22]
//    (0x0056C8B1); a value of at most zero leaves at once (0x0056C8BD). The body
//    below that point walks the record's containers at +44, +48, +64, +96 and +128
//    through 0x004F8210 / 0x004F8200, calls vtable slots +492, +36 and +4, and
//    calls 0x0056BB40, 0x00563550, 0x00563FD0, 0x0056E600, 0x0056B8C0..0x0056B910,
//    0x0056D0F0 and 0x0056D690. None of those structures is in the save and the
//    audit keeps the semantics unclear, so the walk stays a named refusal.
//  * 0x0056D170 has the same entry: registry[index] at 0x0056D19E, index minus
//    word[object+22] at 0x0056D1A5, and an early leave at 0x0056D1B3 that stores
//    the zero answer flag and writes nothing (0x0056D4AB). Its answer once the
//    body ran is (count(+80) > 0) || (count(+48) > 0) (0x0056D48D..0x0056D4A7,
//    counters from 0x0056B8E0 and 0x0056B8C0), but the body rewrites those
//    containers, so the answer cannot be stated without it.
//  * The two profiles are exactly doubled by the gate: 2,3,8,7,5,3,3 against
//    4,6,16,14,10,6,6 at +16,+20,+24,+28,+32,+36,+40.
inline constexpr int32_t kOriginalPostFigureRecordProfile0x56A910[7] = {2, 3, 8, 7, 5, 3, 3};
inline constexpr int32_t kOriginalPostFigureRecordProfile0x56A940[7] = {4, 6, 16, 14, 10, 6, 6};
inline constexpr size_t kOriginalPostFigureRecordProfileFirstOffset = 16;
inline constexpr size_t kOriginalPostFigureRecordProfileStride = 4;

// The record dwords the two profiles write, named by offset.
struct OriginalPostFigureRecordScalars {
    int32_t field0x10 = 0;  // dword+16
    int32_t field0x14 = 0;  // dword+20
    int32_t field0x18 = 0;  // dword+24
    int32_t field0x1C = 0;  // dword+28
    int32_t field0x20 = 0;  // dword+32
    int32_t field0x24 = 0;  // dword+36
    int32_t field0x28 = 0;  // dword+40
};

// 0x0056A910: the unconditional opening profile.
inline void originalPostFigureRecordProfile0x56A910(OriginalPostFigureRecordScalars& scalars) {
    scalars.field0x10 = kOriginalPostFigureRecordProfile0x56A910[0];
    scalars.field0x14 = kOriginalPostFigureRecordProfile0x56A910[1];
    scalars.field0x18 = kOriginalPostFigureRecordProfile0x56A910[2];
    scalars.field0x1C = kOriginalPostFigureRecordProfile0x56A910[3];
    scalars.field0x20 = kOriginalPostFigureRecordProfile0x56A910[4];
    scalars.field0x24 = kOriginalPostFigureRecordProfile0x56A910[5];
    scalars.field0x28 = kOriginalPostFigureRecordProfile0x56A910[6];
}

// 0x0056A940: the opening profile when 0x00564E80 answers true.
inline void originalPostFigureRecordProfile0x56A940(OriginalPostFigureRecordScalars& scalars) {
    scalars.field0x10 = kOriginalPostFigureRecordProfile0x56A940[0];
    scalars.field0x14 = kOriginalPostFigureRecordProfile0x56A940[1];
    scalars.field0x18 = kOriginalPostFigureRecordProfile0x56A940[2];
    scalars.field0x1C = kOriginalPostFigureRecordProfile0x56A940[3];
    scalars.field0x20 = kOriginalPostFigureRecordProfile0x56A940[4];
    scalars.field0x24 = kOriginalPostFigureRecordProfile0x56A940[5];
    scalars.field0x28 = kOriginalPostFigureRecordProfile0x56A940[6];
}

enum class OriginalPostFigure56C880Status {
    Ok,                      // the profile ran and the relative index left at 0x0056C8BD
    RefusedMissingInput,     // the gate or the scalar target was not supplied
    RefusedIndexOutsidePool, // registry[index] would be read out of range
    RefusedNullObject,       // registry[index] held no object
    RefusedContainers,       // the container walk below 0x0056C8C3 is not ported
};

inline const char* originalPostFigure56C880StatusName(OriginalPostFigure56C880Status status) {
    switch (status) {
        case OriginalPostFigure56C880Status::Ok: return "ok";
        case OriginalPostFigure56C880Status::RefusedMissingInput: return "refused_missing_input";
        case OriginalPostFigure56C880Status::RefusedIndexOutsidePool: return "refused_index_outside_pool";
        case OriginalPostFigure56C880Status::RefusedNullObject: return "refused_null_object";
        case OriginalPostFigure56C880Status::RefusedContainers: return "refused_containers";
    }
    return "unknown";
}

struct OriginalPostFigure56C880Inputs {
    // 0x00564E80 at 0x0056C890: true selects the doubled profile.
    std::optional<std::function<bool()>> gate0x564E80;
    // The record whose +16..+40 dwords the profiles write. No default.
    std::optional<std::function<OriginalPostFigureRecordScalars*()>> scalarsForRecord;
};

// The exact prefix of 0x0056C880: both profiles behind the gate, then the
// relative-index leave. A non-positive relative index ends the callee, so Ok is
// the complete answer for it; anything else is the named container refusal.
inline OriginalPostFigure56C880Status originalPostFigureUpdateRecord0x56C880(
    uint32_t index, const std::vector<const OriginalPostFigureRegistryObject*>& registry,
    const OriginalPostFigure56C880Inputs& inputs) {
    using Status = OriginalPostFigure56C880Status;
    if (!inputs.gate0x564E80 || !inputs.scalarsForRecord) return Status::RefusedMissingInput;
    OriginalPostFigureRecordScalars* scalars = (*inputs.scalarsForRecord)();
    if (!scalars) return Status::RefusedMissingInput;
    originalPostFigureRecordProfile0x56A910(*scalars);           // 0x0056C88B
    if ((*inputs.gate0x564E80)()) originalPostFigureRecordProfile0x56A940(*scalars);  // 0x0056C89B
    if (size_t(index) >= registry.size()) return Status::RefusedIndexOutsidePool;
    const OriginalPostFigureRegistryObject* slot = registry[index];
    if (!slot) return Status::RefusedNullObject;                 // 0x0056C8AF dereference
    if (int32_t(index) - int32_t(slot->word0x16) <= 0) return Status::Ok;  // 0x0056C8BD
    return Status::RefusedContainers;                            // 0x0056C8C3 onward
}

enum class OriginalPostFigure56D170Status {
    OkFalse,                 // 0x0056D1B3: the early leave answers false
    RefusedIndexOutsidePool, // registry[index] would be read out of range
    RefusedNullObject,       // registry[index] held no object
    RefusedContainers,       // the body below 0x0056D1B9 is not ported
};

inline const char* originalPostFigure56D170StatusName(OriginalPostFigure56D170Status status) {
    switch (status) {
        case OriginalPostFigure56D170Status::OkFalse: return "ok_false";
        case OriginalPostFigure56D170Status::RefusedIndexOutsidePool: return "refused_index_outside_pool";
        case OriginalPostFigure56D170Status::RefusedNullObject: return "refused_null_object";
        case OriginalPostFigure56D170Status::RefusedContainers: return "refused_containers";
    }
    return "unknown";
}

// The exact prefix of 0x0056D170: the relative-index leave, whose answer is false
// and whose only write is that answer. Every other path needs the containers.
inline OriginalPostFigure56D170Status originalPostFigureStepRecord0x56D170(
    uint32_t index, const std::vector<const OriginalPostFigureRegistryObject*>& registry) {
    using Status = OriginalPostFigure56D170Status;
    if (size_t(index) >= registry.size()) return Status::RefusedIndexOutsidePool;
    const OriginalPostFigureRegistryObject* slot = registry[index];
    if (!slot) return Status::RefusedNullObject;                 // 0x0056D1A3 dereference
    if (int32_t(index) - int32_t(slot->word0x16) <= 0) return Status::OkFalse;  // 0x0056D1B3
    return Status::RefusedContainers;                            // 0x0056D1B9 onward
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
    Refused564EA0Contract,       // 0x00564EA0 input precondition (index/null)
    RefusedUnported567610,       // 0x00564C0B
    Refused567610ZeroAdvance,    // 0x00567610 answered 0: the index would not advance
    // Narrow 0x0056C880 / 0x0056D170 ports: the exact profile and entry prefix
    // ran, and the container walk below it stays refused by name.
    Refused56C880Containers,     // 0x0056C8C3 onward
    Refused56C880Contract,       // registry[index] unusable at the 0x0056C8AA read
    Refused56D170Containers,     // 0x0056D1B9 onward
    Refused56D170Contract,       // registry[index] unusable at the 0x0056D19E read
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
        case OriginalPostFigurePassStatus::Refused564EA0Contract: return "refused_564ea0_contract";
        case OriginalPostFigurePassStatus::RefusedUnported567610: return "refused_unported_567610";
        case OriginalPostFigurePassStatus::Refused567610ZeroAdvance: return "refused_567610_zero_advance";
        case OriginalPostFigurePassStatus::Refused56C880Containers: return "refused_56c880_containers";
        case OriginalPostFigurePassStatus::Refused56C880Contract: return "refused_56c880_contract";
        case OriginalPostFigurePassStatus::Refused56D170Containers: return "refused_56d170_containers";
        case OriginalPostFigurePassStatus::Refused56D170Contract: return "refused_56d170_contract";
    }
    return "unknown";
}

// The two live globals. 0x012A6E78 is the countdown, 0x00859DA0 the current period.
// Neither has a save representation; see the "not serialized" note above.
struct OriginalPostFigurePassState {
    uint32_t counter = 0;  // 0x012A6E78
    uint32_t period = 0;   // 0x00859DA0
};

// ---------------------------------------------------------------------------
// 0x012A5DD0 record table, 0x005653C0 and 0x00567610
// ---------------------------------------------------------------------------
// Both lookups walk one array: 144-byte records starting at 0x012A5DD0, entry
// count at 0x012A6E7C. VERIFIED from the executable:
//  * The count is read with a signed check (0x005653DE, 0x00567619); a count of at
//    most zero leaves at once with a null / zero answer (0x00565401, 0x00567635).
//  * The key is dword[record+4]. 0x005653EB reads word[object+20] with a
//    sign-extending load and 0x005653EF compares it with dword[record+4];
//    0x00567626 compares the same dword with the caller's argument.
//  * 0x005653C0 resolves its own argument through 0x0047F1B0 (0x005653C8), reads
//    word[slot+20] and answers the first record whose key matches: the scan
//    leaves as soon as a match is stored (0x005653E7), and it answers null when
//    the whole array is exhausted (0x00565401, null answer).
//  * 0x00567610 answers dword[record+12] (the hit index is scaled by 9 then by
//    16 at 0x00567639/0x0056763C and offset by 0x012A5DDC = base+12) and zero
//    when no record matches (0x00567635).
//  * Capacity and construction: 0x00567650 reuses a record whose key already
//    exists (0x00567666), refuses once the count has reached 27 (0x00567675) and
//    initialises the fresh record through 0x0056B270. 0x0056488B zeroes the
//    count and the following registrations supply the keys from code, so the
//    array is built at run time.
//
// Not serialized: the SAV19 read window 0x005311DC..0x00531873 holds 91 read
// requests (the shared read entry point 0x00780533) and none of them names
// 0x012A5DD0, 0x012A5DD4, 0x012A5DDC or 0x012A6E7C. Together with the
// registration path above, a loaded save cannot supply this array: the caller
// must state the records and their order. Only the two fields the lookups read
// are modelled; the other 132 bytes belong to other consumers.
inline constexpr uint32_t kOriginalPostFigureRecordTableVa = 0x012A5DD0u;
inline constexpr uint32_t kOriginalPostFigureRecordKeyVa = 0x012A5DD4u;
inline constexpr uint32_t kOriginalPostFigureRecordAdvanceVa = 0x012A5DDCu;
inline constexpr uint32_t kOriginalPostFigureRecordCountVa = 0x012A6E7Cu;
inline constexpr size_t kOriginalPostFigureRecordSize = 144;
inline constexpr size_t kOriginalPostFigureRecordCapacity = 27;
inline constexpr size_t kOriginalPostFigureRecordKeyOffset = 4;
inline constexpr size_t kOriginalPostFigureRecordAdvanceOffset = 12;

// One table record, named by offset: key0x04 is the type both lookups compare,
// advance0x0C is the skip count 0x00567610 answers.
struct OriginalPostFigureRecord {
    int32_t key0x04 = 0;
    int32_t advance0x0C = 0;
};

// The whole array in slot order. records.size() is the 0x012A6E7C count, so an
// empty table is the exe's "count of at most zero" case.
struct OriginalPostFigureRecordTable {
    std::vector<OriginalPostFigureRecord> records;
};

// One 0x00567610 invocation: the skip count for the type, zero when the type has
// no record. The answer is signed in the exe and used as a signed count by the
// caller at 0x00564C13.
inline int32_t originalPostFigureSkipAdvance0x567610(const OriginalPostFigureRecordTable& table,
                                                     int32_t type) {
    for (const OriginalPostFigureRecord& record : table.records) {
        if (record.key0x04 == type) return record.advance0x0C;  // 0x00567639 answer
    }
    return 0;  // 0x00567635: the array was exhausted
}

// One 0x005653C0 invocation: the first record whose key equals the object's type
// word, or null. The exe re-resolves 0x008C7634[index] at 0x005653C8 only to read
// that word; the caller already holds the same slot, so the object is handed over
// directly instead of an index.
inline const OriginalPostFigureRecord* originalPostFigureFindRecord0x5653C0(
    const OriginalPostFigureRecordTable& table, const OriginalPostFigureRegistryObject& object) {
    const int32_t type = object.type0x14;  // word[object+20], sign extended at 0x005653EB
    for (const OriginalPostFigureRecord& record : table.records) {
        if (record.key0x04 == type) return &record;  // 0x005653F4 stores the hit
    }
    return nullptr;  // 0x00565401
}

// ---------------------------------------------------------------------------
// 0x00564EA0: the types-76/77 step
// ---------------------------------------------------------------------------
// VERIFIED from the executable (body 0x00564EA0..0x00565380, single exit path
// through 0x00565371/0x00565379, discarding its one stack argument):
//  * Entry: an index of at most zero leaves at once (0x00564EDB). Otherwise the
//    linked index is index - word[object+22] (0x00564EF0/0x00564EF4), the linked
//    object's state pointer comes from vtable slot +0x1EC (0x00564F0B), and the
//    step leaves when that state's byte +96 is non-zero (0x00564F1A), when
//    0x00564E90 answers false (0x00564F27), when the linked object's type is
//    neither 76 nor 77 (0x00564F39/0x00564F3F), or when the state's dword +20 is
//    not 4 (0x00564F4D). None of those leaves writes anything.
//  * The candidate count is 0x00567610(linked type) at 0x00564F57. A count of at
//    most zero skips the scan and jumps straight to the commit decision with no
//    candidate recorded (0x00564F61), so only the marker at +96 is written.
//  * The scan walks candidates at index+0 .. index+count-1 (0x00564F71 is the
//    first candidate, 0x0056521E/0x00565230 stop after `count` visits). For each
//    candidate it takes the state pointer from vtable slot +0x1EC, asks
//    0x005635A0 for a neighbour record, and reads that record's dword +4 and +8.
//    The five accepted (dword+4, dword+8) shapes and their resources:
//      (-1,-2) resource 42 when the 0x004812E0 flag is set, else 44; type 23/22
//      (-1, 1) resource 226, only when the argument object's type is 77; type 21
//      ( 1,-2) resource 46, only when that type is 77; type 26
//      ( 2,-2) resource 47; type 24
//      ( 2, 1) resource 43; type 25
//    The (+4 == -1) branch is the only one that also reaches the (+8 == 1) case.
//    Each accepted shape converts one state dword to a value:
//      0x00564FB6 dword+64 with 700.0 for type 76 and 1100.0 otherwise,
//      0x00565072 dword+76 with 700.0, 0x005650EC dword+80 with 400.0,
//      0x00565158 dword+68 with 400.0 for type 76 and 1000.0 otherwise,
//      0x005651C2 dword+72 with 600.0 for type 76 and 900.0 otherwise.
//    "type 76" here is always the argument object's own type word: 0x00564FB2,
//    0x00565154 and 0x005651BE read the object saved at 0x00564EA9, not the
//    linked or candidate object. The linked type only gates entry (76 or 77) and
//    supplies the candidate count.
//    Constant bytes read back from the file: 0x007B8A98 = 700.0,
//    0x007B8A90 = 1100.0, 0x007ACA28 = 400.0, 0x007AE110 = 1000.0,
//    0x007B8A88 = 600.0, 0x007B8A80 = 900.0.
//    The subtraction is `constant - dword` (the x87 reverse subtract at each
//    site) and 0x007650FC converts it to a signed 32-bit integer by truncation.
//  * 0x00402A50(resource) decides the accumulator: non-zero compares the value
//    with the best available score (0x00564FEB etc.) and records the state plus
//    a type code; zero counts the candidate (0x00564FFF etc.) and compares with
//    the best unavailable score. Both comparisons are strict and signed, so the
//    first candidate wins a tie.
//  * Commit (0x00565236): when at least one candidate was unavailable the best
//    unavailable one is charged, otherwise the best available one. The type code
//    21..26 selects the state dword: 21 -> +76, 22 and 23 -> +64, 24 -> +68,
//    25 -> +72, 26 -> +80 (dispatch tables at 0x00565384 for the unavailable
//    branch and 0x0056539C for the available branch; both list those targets in
//    that order). A code outside 21..26 charges nothing (0x00565371). Every
//    commit path then writes byte +96 = 1 on the linked object's state.
//
// Not serialized: this callee owns no global. Its inputs are the registry, the
// 0x012A5DD0 array and the state/neighbour objects, all run-time structures; the
// SAV19 window names none of them. The vtable slot at +0x1EC, the 0x005635A0
// neighbour record and the five probes are therefore explicit inputs below, with
// no default: an absent one refuses instead of being replaced by a guess.
inline constexpr double kOriginalPostFigureType7677Anchor76 = 700.0;    // 0x007B8A98
inline constexpr double kOriginalPostFigureType7677AnchorOther = 1100.0; // 0x007B8A90
inline constexpr double kOriginalPostFigureType7677Resource76 = 400.0;   // 0x007ACA28
inline constexpr double kOriginalPostFigureType7677Resource1000 = 1000.0; // 0x007AE110
inline constexpr double kOriginalPostFigureType7677Resource600 = 600.0;  // 0x007B8A88
inline constexpr double kOriginalPostFigureType7677Resource900 = 900.0;  // 0x007B8A80
inline constexpr uint32_t kOriginalPostFigureType7677CommitTableVa = 0x00565384u;
inline constexpr uint32_t kOriginalPostFigureType7677CommitTableAvailableVa = 0x0056539Cu;

// The state object reached through vtable slot +0x1EC, named by offset. The step
// reads dword +20 and byte +96 from the linked object's state and charges one of
// the five dwords on the winning candidate's state.
struct OriginalPostFigureType7677State {
    int32_t field0x14 = 0;  // dword+20, must equal 4 on the linked object's state
    int32_t field0x40 = 0;  // dword+64
    int32_t field0x44 = 0;  // dword+68
    int32_t field0x48 = 0;  // dword+72
    int32_t field0x4C = 0;  // dword+76
    int32_t field0x50 = 0;  // dword+80
    uint8_t byte0x60 = 0;   // byte+96, set to 1 when the step commits
};

// The neighbour record 0x005635A0 answers for a candidate index: only these two
// dwords are read. 0x005635A0 itself walks the 0x012A5DD0 record's +8 pointer and
// calls 0x00563530; neither is ported, so the record is an input.
struct OriginalPostFigureType7677Neighbour {
    int32_t field0x04 = 0;
    int32_t field0x08 = 0;
};

// Every external question the step asks. All five are required; a missing one is
// a refusal, never a default.
struct OriginalPostFigureType7677Inputs {
    // vtable slot +0x1EC of registry[index]: the per-object state. Called at
    // 0x00564F0B for the linked index and once per candidate at 0x00564F82.
    std::optional<std::function<OriginalPostFigureType7677State*(uint32_t)>> stateForIndex;
    // 0x005635A0(index, out) at 0x00564F90: the candidate's neighbour record.
    std::optional<std::function<const OriginalPostFigureType7677Neighbour*(uint32_t)>> neighbourForIndex;
    // 0x004812E0(0x00480740(23)) at 0x00564EC7/0x00564ECE: the branch flag that
    // picks resource 42 over 44 and type code 23 over 22.
    std::optional<std::function<bool()>> flag0x4812E0;
    // 0x00564E90 at 0x00564F20: a session gate that suppresses the whole step.
    std::optional<std::function<bool()>> probe0x564E90;
    // 0x00402A50(resource) at 0x00564FDF, 0x0056501A, 0x00565087, 0x005650FE,
    // 0x00565179 and 0x005651E3: true selects the available accumulator.
    std::optional<std::function<bool(uint32_t)>> probe0x402A50;
};

inline bool originalPostFigureType7677InputsComplete(const OriginalPostFigureType7677Inputs& inputs) {
    return inputs.stateForIndex.has_value() && inputs.neighbourForIndex.has_value() &&
           inputs.flag0x4812E0.has_value() && inputs.probe0x564E90.has_value() &&
           inputs.probe0x402A50.has_value();
}

// 0x007650FC: the x87-to-integer helper truncates toward zero and answers the
// indefinite value 0x80000000 when the operand does not fit a signed 32-bit
// integer. Both operands here are integers, so the double arithmetic is exact.
inline int32_t originalPostFigureType7677Truncate(double value) {
    if (!(value < 2147483648.0) || !(value >= -2147483648.0)) return int32_t(0x80000000u);
    return static_cast<int32_t>(value);
}

enum class OriginalPostFigureType7677Status {
    Ok,                      // left through 0x00565371 or a commit path
    RefusedMissingInput,     // one of the five inputs was not supplied
    RefusedIndexOutsidePool, // the linked or candidate index left the registry
    RefusedNullObject,       // a registry slot held no object
    RefusedNullState,        // vtable slot +0x1EC answered null
    RefusedNullNeighbour,    // 0x005635A0 answered null (the exe dereferences it)
};

inline const char* originalPostFigureType7677StatusName(OriginalPostFigureType7677Status status) {
    switch (status) {
        case OriginalPostFigureType7677Status::Ok: return "ok";
        case OriginalPostFigureType7677Status::RefusedMissingInput: return "refused_missing_input";
        case OriginalPostFigureType7677Status::RefusedIndexOutsidePool: return "refused_index_outside_pool";
        case OriginalPostFigureType7677Status::RefusedNullObject: return "refused_null_object";
        case OriginalPostFigureType7677Status::RefusedNullState: return "refused_null_state";
        case OriginalPostFigureType7677Status::RefusedNullNeighbour: return "refused_null_neighbour";
    }
    return "unknown";
}

// One 0x00564EA0 invocation. `object` is the registry[index] object the caller
// resolved at 0x00564B8C; `registry` is the 0x008C7634 pool; `records` is the
// 0x012A5DD0 array for the nested 0x00567610. Every leave that writes nothing is
// reported as Ok because the exe treats it as "nothing to do".
inline OriginalPostFigureType7677Status originalPostFigureHandleTypes7677(
    const OriginalPostFigureRegistryObject& object, uint32_t index,
    const std::vector<const OriginalPostFigureRegistryObject*>& registry,
    const OriginalPostFigureRecordTable& records, const OriginalPostFigureType7677Inputs& inputs) {
    using Status = OriginalPostFigureType7677Status;
    if (!originalPostFigureType7677InputsComplete(inputs)) return Status::RefusedMissingInput;

    const bool flag = (*inputs.flag0x4812E0)();  // 0x00564EC7/0x00564ECE
    if (int32_t(index) <= 0) return Status::Ok;  // 0x00564EDB signed
    if (size_t(index) >= registry.size()) return Status::RefusedIndexOutsidePool;
    const OriginalPostFigureRegistryObject* slot = registry[index];
    if (!slot) return Status::RefusedNullObject;

    // 0x00564EF0..0x00564EF4: the linked index is relative to the object's +22 word.
    const int32_t linkedIndex = int32_t(index) - int32_t(slot->word0x16);
    if (linkedIndex < 0 || size_t(linkedIndex) >= registry.size()) {
        return Status::RefusedIndexOutsidePool;
    }
    const OriginalPostFigureRegistryObject* linked = registry[size_t(linkedIndex)];
    if (!linked) return Status::RefusedNullObject;

    OriginalPostFigureType7677State* linkedState = (*inputs.stateForIndex)(uint32_t(linkedIndex));
    if (!linkedState) return Status::RefusedNullState;              // 0x00564F0B
    if (linkedState->byte0x60 != 0) return Status::Ok;              // 0x00564F1A
    if (!(*inputs.probe0x564E90)()) return Status::Ok;              // 0x00564F27
    if (linked->type0x14 != 76 && linked->type0x14 != 77) return Status::Ok;  // 0x00564F31
    if (linkedState->field0x14 != 4) return Status::Ok;             // 0x00564F4D

    const int32_t remaining = originalPostFigureSkipAdvance0x567610(records, linked->type0x14);
    int32_t bestAvailable = 0;    // the exe keeps this in one register
    int32_t bestUnavailable = 0;  // and this in another; both start at zero
    int32_t unavailableCount = 0;
    OriginalPostFigureType7677State* availableState = nullptr;
    int32_t availableCode = 0;
    OriginalPostFigureType7677State* unavailableState = nullptr;
    int32_t unavailableCode = 0;
    const int16_t callerType = object.type0x14;

    auto charge = [&](uint32_t resource, int32_t value, int32_t code,
                      OriginalPostFigureType7677State* candidate) {
        if ((*inputs.probe0x402A50)(resource)) {
            if (value > bestAvailable) {  // strict and signed
                bestAvailable = value;
                availableCode = code;
                availableState = candidate;
            }
        } else {
            ++unavailableCount;
            if (value > bestUnavailable) {
                bestUnavailable = value;
                unavailableCode = code;
                unavailableState = candidate;
            }
        }
    };

    int32_t cursor = linkedIndex;
    int32_t left = remaining;
    while (left > 0) {  // 0x00564F71 .. 0x00565230
        const uint32_t current = uint32_t(cursor);
        if (size_t(current) >= registry.size()) return Status::RefusedIndexOutsidePool;
        const OriginalPostFigureRegistryObject* candidateObject = registry[current];
        if (!candidateObject) return Status::RefusedNullObject;
        OriginalPostFigureType7677State* candidate = (*inputs.stateForIndex)(current);
        if (!candidate) return Status::RefusedNullState;  // 0x00564F82
        const OriginalPostFigureType7677Neighbour* neighbour = (*inputs.neighbourForIndex)(current);
        if (!neighbour) return Status::RefusedNullNeighbour;  // 0x00564F90

        if (neighbour->field0x04 == -1) {  // 0x00564FA2
            if (neighbour->field0x08 == -2) {  // 0x00564FA8
                const double base = callerType == 76 ? kOriginalPostFigureType7677Anchor76
                                                     : kOriginalPostFigureType7677AnchorOther;
                const int32_t value = originalPostFigureType7677Truncate(
                    base - double(candidate->field0x40));  // 0x00564FB6
                charge(flag ? 42u : 44u, value, flag ? 23 : 22, candidate);
            }
            if (neighbour->field0x08 == 1 && callerType == 77) {  // 0x00565059/0x00565067
                const int32_t value = originalPostFigureType7677Truncate(
                    kOriginalPostFigureType7677Anchor76 - double(candidate->field0x4C));
                charge(226u, value, 21, candidate);  // 0x00565072
            }
        } else if (neighbour->field0x04 == 1) {  // 0x005650CE
            if (neighbour->field0x08 == -2 && callerType == 77) {  // 0x005650D3/0x005650DD
                const int32_t value = originalPostFigureType7677Truncate(
                    kOriginalPostFigureType7677Resource76 - double(candidate->field0x50));
                charge(46u, value, 26, candidate);  // 0x005650EC
            }
        } else if (neighbour->field0x04 == 2) {  // 0x00565145
            if (neighbour->field0x08 == -2) {  // 0x0056514E
                const double base = callerType == 76 ? kOriginalPostFigureType7677Resource76
                                                     : kOriginalPostFigureType7677Resource1000;
                const int32_t value = originalPostFigureType7677Truncate(
                    base - double(candidate->field0x44));
                charge(47u, value, 24, candidate);  // 0x00565158
            }
            if (neighbour->field0x08 == 1) {  // 0x005651B4
                const double base = callerType == 76 ? kOriginalPostFigureType7677Resource600
                                                     : kOriginalPostFigureType7677Resource900;
                const int32_t value = originalPostFigureType7677Truncate(
                    base - double(candidate->field0x48));
                charge(43u, value, 25, candidate);  // 0x005651C2
            }
        }
        ++cursor;  // 0x00565226
        --left;    // 0x00565227
    }

    // 0x00565236: an unavailable candidate takes precedence, otherwise the best
    // available one is charged. Code 21..26 selects the dword; anything else
    // charges nothing but still marks the state.
    if (unavailableCount > 0) {
        if (unavailableState) {
            switch (unavailableCode) {
                case 21: unavailableState->field0x4C += bestUnavailable; break;
                case 22:
                case 23: unavailableState->field0x40 += bestUnavailable; break;
                case 24: unavailableState->field0x44 += bestUnavailable; break;
                case 25: unavailableState->field0x48 += bestUnavailable; break;
                case 26: unavailableState->field0x50 += bestUnavailable; break;
                default: break;  // 0x00565371
            }
        }
    } else if (availableState) {
        switch (availableCode) {
            case 21: availableState->field0x4C += bestAvailable; break;
            case 22:
            case 23: availableState->field0x40 += bestAvailable; break;
            case 24: availableState->field0x44 += bestAvailable; break;
            case 25: availableState->field0x48 += bestAvailable; break;
            case 26: availableState->field0x50 += bestAvailable; break;
            default: break;
        }
    }
    linkedState->byte0x60 = 1;  // 0x0056526D / 0x00565315 / 0x00565371
    return Status::Ok;
}

// Session inputs. Every field is std::optional and has no default value: an absent
// field refuses, it is never replaced by 0 or by a save byte. The first five are the
// process/session state the SAV19 window does not carry; the five providers are the
// callees this skeleton does not implement, and each provider has a ported
// counterpart field below that may stand in when the provider is absent. `registry`
// is the 0x008C7634 pool as the
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

    // 0x00564EA0's ported implementation, used for types 76 and 77 when the
    // provider above is absent. It also needs recordTable0x12A5DD0 for the nested
    // 0x00567610 lookup.
    std::optional<OriginalPostFigureType7677Inputs> types7677_0x564EA0;

    // The 0x012A5DD0 array, for the two lookups that are ported in this header.
    // When the matching provider above is present it wins; when it is absent and
    // this array is supplied, the ported lookup runs instead of refusing. An
    // absent array with an absent provider keeps the named refusal.
    std::optional<OriginalPostFigureRecordTable> recordTable0x12A5DD0;

    // The exact prefix of 0x0056C880, run for a found record when the provider
    // above is absent. Its two fields (the 0x00564E80 gate and the record's
    // +16..+40 scalar target) are not in the save, so the caller must supply
    // them. Absent with the provider absent keeps refused_unported_56c880.
    std::optional<OriginalPostFigure56C880Inputs> updateProfile0x56C880;
    // Selects the exact prefix of 0x0056D170 when stepRecord0x56D170 is absent.
    // The prefix reads only the registry, so this is a wiring switch rather than
    // save state. A supplied provider still wins over it.
    bool stepPrefix0x56D170 = false;
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
                if (!inputs.indexRecord0x5653C0 && !inputs.recordTable0x12A5DD0) {
                    result.status = OriginalPostFigurePassStatus::RefusedUnported5653C0;
                    return result;
                }
                const void* record = nullptr;
                if (inputs.indexRecord0x5653C0) {
                    record = (*inputs.indexRecord0x5653C0)(*object, cursor);
                } else {
                    record = originalPostFigureFindRecord0x5653C0(*inputs.recordTable0x12A5DD0, *object);
                }
                if (record) {  // 0x00564BC2 leaves for 0x00564BEE when the record is null
                    if (*inputs.pause0x10DE0D0 == 0 && *inputs.mode0x10DE045 != 86) {
                        if (inputs.updateRecord0x56C880) {
                            (*inputs.updateRecord0x56C880)(record, cursor);
                        } else if (inputs.updateProfile0x56C880) {
                            const auto update = originalPostFigureUpdateRecord0x56C880(
                                cursor, inputs.registry, *inputs.updateProfile0x56C880);
                            if (update != OriginalPostFigure56C880Status::Ok) {
                                switch (update) {
                                    case OriginalPostFigure56C880Status::RefusedContainers:
                                        result.status = OriginalPostFigurePassStatus::Refused56C880Containers;
                                        break;
                                    case OriginalPostFigure56C880Status::RefusedIndexOutsidePool:
                                    case OriginalPostFigure56C880Status::RefusedNullObject:
                                        result.status = OriginalPostFigurePassStatus::Refused56C880Contract;
                                        break;
                                    case OriginalPostFigure56C880Status::RefusedMissingInput:
                                    case OriginalPostFigure56C880Status::Ok:
                                        result.status = OriginalPostFigurePassStatus::RefusedUnported56C880;
                                        break;
                                }
                                return result;
                            }
                        } else {
                            result.status = OriginalPostFigurePassStatus::RefusedUnported56C880;
                            return result;
                        }
                        if (inputs.stepRecord0x56D170) {
                            if ((*inputs.stepRecord0x56D170)(record, cursor)) result.progress = true;
                        } else if (inputs.stepPrefix0x56D170) {
                            const auto step = originalPostFigureStepRecord0x56D170(cursor, inputs.registry);
                            if (step != OriginalPostFigure56D170Status::OkFalse) {
                                result.status = step == OriginalPostFigure56D170Status::RefusedContainers
                                                    ? OriginalPostFigurePassStatus::Refused56D170Containers
                                                    : OriginalPostFigurePassStatus::Refused56D170Contract;
                                return result;
                            }
                        } else {
                            result.status = OriginalPostFigurePassStatus::RefusedUnported56D170;
                            return result;
                        }
                    }
                }
                if (type == 76 || type == 77) {  // 0x00564BF2/0x00564BFC reach 0x00564C01
                    if (inputs.handleTypes76_77_0x564EA0) {
                        (*inputs.handleTypes76_77_0x564EA0)(*object, cursor);
                    } else if (inputs.types7677_0x564EA0 && inputs.recordTable0x12A5DD0) {
                        const auto step = originalPostFigureHandleTypes7677(
                            *object, cursor, inputs.registry, *inputs.recordTable0x12A5DD0,
                            *inputs.types7677_0x564EA0);
                        if (step != OriginalPostFigureType7677Status::Ok) {
                            result.status =
                                step == OriginalPostFigureType7677Status::RefusedMissingInput
                                    ? OriginalPostFigurePassStatus::RefusedUnported564EA0
                                    : OriginalPostFigurePassStatus::Refused564EA0Contract;
                            return result;
                        }
                    } else {
                        result.status = OriginalPostFigurePassStatus::RefusedUnported564EA0;
                        return result;
                    }
                }
                if (!inputs.skipAdvance0x567610 && !inputs.recordTable0x12A5DD0) {
                    result.status = OriginalPostFigurePassStatus::RefusedUnported567610;
                    return result;
                }
                const int32_t advance = inputs.skipAdvance0x567610
                                            ? int32_t((*inputs.skipAdvance0x567610)(type))
                                            : originalPostFigureSkipAdvance0x567610(
                                                  *inputs.recordTable0x12A5DD0, type);
                if (advance == 0) {
                    // 0x00564C13 subtracts one from the answer and 0x00564C1C adds one
                    // back: a zero count leaves the scan index where it was, so the exe
                    // would repeat this slot forever. Treat the missing table entry as
                    // a named refusal rather than silently advancing by one.
                    result.status = OriginalPostFigurePassStatus::Refused567610ZeroAdvance;
                    return result;
                }
                // Net of the -1 at 0x00564C13 and the shared increment at 0x00564C1C is
                // exactly the signed answer; a negative answer walks the index back.
                cursor = uint32_t(int32_t(cursor) + advance);
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
