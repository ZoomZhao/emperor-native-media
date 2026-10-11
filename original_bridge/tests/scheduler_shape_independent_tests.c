#include "scheduler_shape.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Synthetic declarations only. Expected boundaries come from the published
 * schema1 contract, not from a fabricated game clock, state or trace. */
static unsigned cases, checks;
static uint64_t seed = UINT64_C(0x45e86bc20261011);
static const uint32_t unknown = OABI_SCHED_RUNTIME_FROZEN |
    OABI_SCHED_TARGET_UNBOUND | OABI_SCHED_FRAME_UNBOUND |
    OABI_SCHED_EFFECTS_UNCLOSED | OABI_SCHED_CYCLE_UNPROVED;

static void check(int yes, const char *why) {
    ++checks;
    if (!yes) {
        fprintf(stderr, "FAIL case %u check %u: %s\n", cases, checks, why);
        exit(1);
    }
}
static uint64_t random_bits(void) {
    seed ^= seed << 13; seed ^= seed >> 7; seed ^= seed << 17;
    return seed;
}
static OAbiSchedulerInput declaration(uint64_t phase) {
    OAbiSchedulerInput x = {0};
    x.schema_version = 1;
    memcpy(x.subject_sha256,
        "8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753", 64);
    memcpy(x.analysis_profile_digest,
        "e3402f2d7c487875edf3c6215fa95d67ee1fa133ba68c2390a9338b38656a7b2", 64);
    x.pointer_bits = 32; x.phase = phase;
    x.declared_entry = UINT64_C(0x4ac2b0);
    x.declared_return = UINT64_C(0x5371c4);
    return x;
}
static OAbiSchedulerReport verify(OAbiSchedulerInput x, int shape_only,
                                   uint32_t rejection_reason) {
    unsigned char before[sizeof(x)];
    memcpy(before, &x, sizeof(x));
    ++cases;
    OAbiSchedulerReport r = oabi_validate_scheduler_declaration(&x);
    check(r.shape_status == (shape_only ? OABI_STATIC_SHAPE_ONLY : OABI_SHAPE_REJECTED),
          "declaration shape has the required acceptance boundary");
    check(r.binding_decision == OABI_BINDING_DENIED, "no declaration enables runtime binding");
    check(r.cycle_completion_proved == 0, "no declaration is a completed cycle trace");
    check((r.reasons & unknown) == unknown, "all five unknown/frozen reasons retained");
    check((r.reasons & rejection_reason) == rejection_reason, "specific rejection explained");
    check(memcmp(before, &x, sizeof(x)) == 0, "caller descriptor unchanged");
    if (shape_only) {
        check(r.description == oabi_describe_scheduler_phase(x.phase), "own immutable descriptor");
        check(r.description && r.description->phase == x.phase, "phase is descriptive only");
    }
    return r;
}
static void dump(void) {
    for (uint64_t phase = 0; phase < 51; ++phase) {
        const OAbiSchedulerPhase *p = oabi_describe_scheduler_phase(phase);
        if (!p) exit(2);
        printf("%" PRIu32 " %" PRIu32 " %" PRIu32, p->phase, p->dispatch_block, p->call_count);
        for (unsigned i = 0; i < 3; ++i) printf(" %" PRIu32, p->call_sites[i]);
        for (unsigned i = 0; i < 3; ++i) printf(" %" PRIu32, p->target_labels[i]);
        printf(" %" PRIu32 " %" PRIu32 " %" PRIu32 "\n", p->conditional_call_mask,
               p->local_flag_store, p->wrap_after_normal_advance);
    }
}
int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--dump") == 0) { dump(); return 0; }
    OAbiSchedulerReport absent = oabi_validate_scheduler_declaration(NULL);
    ++cases;
    check(absent.shape_status == OABI_SHAPE_REJECTED && !absent.description,
          "missing descriptor rejected");
    check(absent.binding_decision == OABI_BINDING_DENIED && !absent.cycle_completion_proved,
          "missing descriptor cannot complete or bind");
    check((absent.reasons & (unknown | OABI_SCHED_MISSING_INPUT)) ==
          (unknown | OABI_SCHED_MISSING_INPUT), "null preserves frozen unknowns");
    OAbiSchedulerPhase snapshots[51];
    const uint32_t nonzero[] = {1, 2, 255, UINT32_MAX};
    for (uint64_t phase = 0; phase < 51; ++phase) {
        snapshots[phase] = *oabi_describe_scheduler_phase(phase);
        OAbiSchedulerInput x = declaration(phase);
        (void)verify(x, 1, 0);
        for (size_t j = 0; j < sizeof(nonzero)/sizeof(nonzero[0]); ++j) {
            x = declaration(phase); x.claim_full_cycle = nonzero[j];
            (void)verify(x, 0, OABI_SCHED_BOUNDARY_INSUFFICIENT);
            x = declaration(phase); x.request_runtime_binding = nonzero[j];
            (void)verify(x, 0, OABI_SCHED_RUNTIME_REQUEST_FORBIDDEN);
        }
        x = declaration(UINT64_C(0x100000000) + phase);
        (void)verify(x, 0, OABI_SCHED_WIDTH_MISMATCH);
    }
    /* A declaration of each phase in order still supplies no execution trace. */
    for (unsigned repeat = 0; repeat < 3; ++repeat)
        for (uint64_t phase = 0; phase < 51; ++phase)
            (void)verify(declaration(phase), 1, 0);

    const uint64_t unsupported_phase[] = {51, 52, 10000, UINT32_MAX, UINT64_C(0x80000000)};
    for (size_t i = 0; i < sizeof(unsupported_phase)/sizeof(unsupported_phase[0]); ++i) {
        OAbiSchedulerInput x = declaration(unsupported_phase[i]);
        (void)verify(x, 0, OABI_SCHED_PHASE_UNSUPPORTED);
        check(oabi_describe_scheduler_phase(x.phase) == NULL, "unsupported phase is not narrowed");
    }
    const uint64_t high[] = {UINT64_C(0x100000000), UINT64_C(0x1004ac2b0),
        UINT64_C(0x1005371c4), UINT64_MAX};
    for (size_t i = 0; i < sizeof(high)/sizeof(high[0]); ++i) {
        OAbiSchedulerInput x = declaration(high[i]);
        (void)verify(x, 0, OABI_SCHED_WIDTH_MISMATCH);
        check(oabi_describe_scheduler_phase(x.phase) == NULL, "high phase cannot alias a low phase");
        x = declaration(8); x.declared_entry = high[i];
        (void)verify(x, 0, OABI_SCHED_WIDTH_MISMATCH);
        x = declaration(8); x.declared_return = high[i];
        (void)verify(x, 0, OABI_SCHED_WIDTH_MISMATCH);
    }
    const uint64_t other_entries[] = {0, 0x4aeeb0, 0x4ac338, 0x4ac650, 0x5371a0, 0x536fe0};
    const uint64_t other_returns[] = {0, 0x4ac33d, 0x4ac4e7, 0x4ac4e2, 0x4acc34, 0x536fe7};
    for (size_t i = 0; i < sizeof(other_entries)/sizeof(other_entries[0]); ++i) {
        OAbiSchedulerInput x = declaration(8); x.declared_entry = other_entries[i];
        (void)verify(x, 0, OABI_SCHED_ENTRY_UNSUPPORTED);
        x = declaration(50); x.declared_return = other_returns[i];
        (void)verify(x, 0, OABI_SCHED_RETURN_UNSUPPORTED);
    }
    /* Identity fields are exactly 64 bytes, not NUL-terminated guest strings. */
    for (size_t byte = 0; byte < 64; ++byte) {
        OAbiSchedulerInput x = declaration(8); x.subject_sha256[byte] = '\0';
        (void)verify(x, 0, OABI_SCHED_IDENTITY_MISMATCH);
        x = declaration(8); x.analysis_profile_digest[byte] ^= 1;
        (void)verify(x, 0, OABI_SCHED_IDENTITY_MISMATCH);
    }
    const uint32_t widths[] = {0, 1, 16, 31, 33, 64, UINT32_MAX};
    for (size_t i = 0; i < sizeof(widths)/sizeof(widths[0]); ++i) {
        OAbiSchedulerInput x = declaration(8); x.pointer_bits = widths[i];
        (void)verify(x, 0, OABI_SCHED_WIDTH_MISMATCH);
    }
    const uint32_t schemas[] = {0, 2, 5, 10, UINT32_MAX};
    for (size_t i = 0; i < sizeof(schemas)/sizeof(schemas[0]); ++i) {
        OAbiSchedulerInput x = declaration(8); x.schema_version = schemas[i];
        (void)verify(x, 0, OABI_SCHED_IDENTITY_MISMATCH);
    }
    /* Fixed seed: fuzz one independent published boundary at a time. */
    for (unsigned i = 0; i < 4096; ++i) {
        uint64_t bits = random_bits();
        OAbiSchedulerInput x = declaration(bits % 51);
        uint32_t reason = 0;
        switch (i % 10) {
        case 0: break;
        case 1: x.phase = UINT64_C(0x100000000) | bits; reason = OABI_SCHED_WIDTH_MISMATCH; break;
        case 2: x.phase = 51 + bits % 100000; reason = OABI_SCHED_PHASE_UNSUPPORTED; break;
        case 3: x.declared_entry ^= (bits % UINT32_MAX) + 1; reason = OABI_SCHED_ENTRY_UNSUPPORTED; break;
        case 4: x.declared_return ^= (bits % UINT32_MAX) + 1; reason = OABI_SCHED_RETURN_UNSUPPORTED; break;
        case 5: x.pointer_bits = (uint32_t)bits | 1; reason = OABI_SCHED_WIDTH_MISMATCH; break;
        case 6: x.claim_full_cycle = (uint32_t)bits | 1; reason = OABI_SCHED_BOUNDARY_INSUFFICIENT; break;
        case 7: x.request_runtime_binding = (uint32_t)bits | 1; reason = OABI_SCHED_RUNTIME_REQUEST_FORBIDDEN; break;
        case 8: x.subject_sha256[bits % 64] ^= 1; reason = OABI_SCHED_IDENTITY_MISMATCH; break;
        case 9: x.analysis_profile_digest[bits % 64] = '\0'; reason = OABI_SCHED_IDENTITY_MISMATCH; break;
        }
        (void)verify(x, reason == 0, reason);
    }
    for (uint64_t phase = 0; phase < 51; ++phase)
        check(memcmp(&snapshots[phase], oabi_describe_scheduler_phase(phase),
                     sizeof(snapshots[phase])) == 0, "descriptors unchanged after adversarial calls");
    printf("PASS independent declaration: %u cases / %u checks; cycle=0, binding denied; seed=45e86bc20261011\n",
           cases, checks);
    return 0;
}
