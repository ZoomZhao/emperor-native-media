#include "scheduler_shape.h"
#include <limits.h>
#include <string.h>

/* Saved jump-table/caller observations. No state transition is executed here. */
static const OAbiSchedulerPhase phases[OABI_SCHEDULER_PHASE_COUNT] = {
    {0,0x4ac4c8,0,{0,0,0},{0,0,0},0,0,0},
    {1,0x4ac2fa,1,{0x4ac2fa,0,0},{0x4f1560,0,0},0,0,0},
    {2,0x4ac4c8,0,{0,0,0},{0,0,0},0,0,0},
    {3,0x4ac42d,0,{0,0,0},{0,0,0},0,1,0},
    {4,0x4ac304,1,{0x4ac304,0,0},{0x4819f0,0,0},0,0,0},
    {5,0x4ac30e,1,{0x4ac310,0,0},{0x54c6f0,0,0},0,0,0},
    {6,0x4ac31d,2,{0x4ac31d,0x4ac324,0},{0x5177b0,0x5185c0,0},0,0,0},
    {7,0x4ac32e,1,{0x4ac32e,0,0},{0x4af350,0,0},0,0,0},
    {8,0x4ac338,1,{0x4ac338,0,0},{0x4aeeb0,0,0},0,0,0},
    {9,0x4ac342,1,{0x4ac342,0,0},{0x40fc20,0,0},0,0,0},
    {10,0x4ac34c,1,{0x4ac34c,0,0},{0x4aeb30,0,0},0,0,0},
    {11,0x4ac356,1,{0x4ac356,0,0},{0x590a50,0,0},0,0,0},
    {12,0x4ac360,1,{0x4ac360,0,0},{0x4ace60,0,0},0,0,0},
    {13,0x4ac36a,2,{0x4ac384,0x4ac38e,0},{0x591ec0,0x5925a0,0},1,0,0},
    {14,0x4ac4c8,0,{0,0,0},{0,0,0},0,0,0},
    {15,0x4ac4c8,0,{0,0,0},{0,0,0},0,0,0},
    {16,0x4ac398,1,{0x4ac398,0,0},{0x5d3cb0,0,0},0,0,0},
    {17,0x4ac3a2,1,{0x4ac3a2,0,0},{0x4aedf0,0,0},0,0,0},
    {18,0x4ac3ac,2,{0x4ac3ae,0x4ac3b5,0},{0x426a60,0x5a8170,0},0,0,0},
    {19,0x4ac4c8,0,{0,0,0},{0,0,0},0,0,0},
    {20,0x4ac3bf,2,{0x4ac3bf,0x4ac3c4,0},{0x51e4a0,0x4bc440,0},0,0,0},
    {21,0x4ac3ce,1,{0x4ac3ce,0,0},{0x4acfc0,0,0},0,0,0},
    {22,0x4ac3d8,1,{0x4ac3d8,0,0},{0x4ad3d0,0,0},0,0,0},
    {23,0x4ac3e2,1,{0x4ac3e2,0,0},{0x4ad4a0,0,0},0,0,0},
    {24,0x4ac3ec,1,{0x4ac3ec,0,0},{0x4ae1a0,0,0},0,0,0},
    {25,0x4ac3f6,3,{0x4ac3f6,0x4ac3fb,0x4ac400},{0x4ae220,0x4ae7f0,0x4f19a0},0,0,0},
    {26,0x4ac4c8,0,{0,0,0},{0,0,0},0,0,0},
    {27,0x4ac40a,1,{0x4ac40a,0,0},{0x5253a0,0,0},0,0,0},
    {28,0x4ac414,1,{0x4ac414,0,0},{0x4ace30,0,0},0,0,0},
    {29,0x4ac41e,1,{0x4ac420,0,0},{0x54c6f0,0,0},0,0,0},
    {30,0x4ac42d,0,{0,0,0},{0,0,0},0,1,0},
    {31,0x4ac43c,1,{0x4ac43c,0,0},{0x416d20,0,0},0,0,0},
    {32,0x4ac446,2,{0x4ac446,0x4ac44b,0},{0x5dbcc0,0x4b02a0,0},0,0,0},
    {33,0x4ac452,2,{0x4ac452,0x4ac457,0},{0x4af230,0x4af2d0,0},0,0,0},
    {34,0x4ac4c8,0,{0,0,0},{0,0,0},0,0,0},
    {35,0x4ac45e,2,{0x4ac45e,0x4ac465,0},{0x5177b0,0x517b40,0},0,0,0},
    {36,0x4ac46c,2,{0x4ac46c,0x4ac473,0},{0x5177b0,0x517ad0,0},0,0,0},
    {37,0x4ac47a,2,{0x4ac47a,0x4ac481,0},{0x53c870,0x44f1d0,0},0,0,0},
    {38,0x4ac488,1,{0x4ac488,0,0},{0x4acd10,0,0},0,0,0},
    {39,0x4ac48f,1,{0x4ac48f,0,0},{0x519120,0,0},0,0,0},
    {40,0x4ac496,1,{0x4ac496,0,0},{0x4acc40,0,0},0,0,0},
    {41,0x4ac4c8,0,{0,0,0},{0,0,0},0,0,0},
    {42,0x4ac4c8,0,{0,0,0},{0,0,0},0,0,0},
    {43,0x4ac49d,1,{0x4ac49d,0,0},{0x4af9b0,0,0},0,0,0},
    {44,0x4ac4a4,1,{0x4ac4a4,0,0},{0x4af5d0,0,0},0,0,0},
    {45,0x4ac4ab,2,{0x4ac4b0,0x4ac4b5,0},{0x5f02c0,0x4b94a0,0},0,0,0},
    {46,0x4ac4c8,0,{0,0,0},{0,0,0},0,0,0},
    {47,0x4ac4bc,1,{0x4ac4bc,0,0},{0x4afd20,0,0},0,0,0},
    {48,0x4ac4c3,1,{0x4ac4c3,0,0},{0x4ae8b0,0,0},0,0,0},
    {49,0x4ac4c8,0,{0,0,0},{0,0,0},0,0,0},
    {50,0x4ac4c8,0,{0,0,0},{0,0,0},0,0,1},
};
const OAbiSchedulerPhase *oabi_describe_scheduler_phase(uint64_t phase) {
    if (phase >= OABI_SCHEDULER_PHASE_COUNT) return NULL;
    return &phases[(size_t)phase];
}
OAbiSchedulerReport oabi_validate_scheduler_declaration(const OAbiSchedulerInput *input) {
    OAbiSchedulerReport r={OABI_SHAPE_REJECTED,OABI_BINDING_DENIED,0,
        OABI_SCHED_RUNTIME_FROZEN|OABI_SCHED_TARGET_UNBOUND|OABI_SCHED_FRAME_UNBOUND|
        OABI_SCHED_EFFECTS_UNCLOSED|OABI_SCHED_CYCLE_UNPROVED,NULL};
    if (!input) {r.reasons|=OABI_SCHED_MISSING_INPUT;return r;}
    if (input->request_runtime_binding) {r.reasons|=OABI_SCHED_RUNTIME_REQUEST_FORBIDDEN;return r;}
    if (input->schema_version!=OABI_SCHEDULER_SCHEMA_VERSION ||
        memcmp(input->subject_sha256,OABI_SUBJECT_SHA256,64) ||
        memcmp(input->analysis_profile_digest,OABI_PROFILE_DIGEST,64)) {
        r.reasons|=OABI_SCHED_IDENTITY_MISMATCH;return r;
    }
    if (input->pointer_bits!=32 || input->phase>UINT32_MAX ||
        input->declared_entry>UINT32_MAX || input->declared_return>UINT32_MAX) {
        r.reasons|=OABI_SCHED_WIDTH_MISMATCH;return r;
    }
    r.description=oabi_describe_scheduler_phase(input->phase);
    if (!r.description) {r.reasons|=OABI_SCHED_PHASE_UNSUPPORTED;return r;}
    if (input->declared_entry!=UINT64_C(0x4ac2b0)) {r.reasons|=OABI_SCHED_ENTRY_UNSUPPORTED;return r;}
    /* The selected caller's integer return label; actual SP/frame is still unbound. */
    if (input->declared_return!=UINT64_C(0x5371c4)) {r.reasons|=OABI_SCHED_RETURN_UNSUPPORTED;return r;}
    if (input->claim_full_cycle) {r.reasons|=OABI_SCHED_BOUNDARY_INSUFFICIENT;return r;}
    r.shape_status=OABI_STATIC_SHAPE_ONLY;
    return r;
}
