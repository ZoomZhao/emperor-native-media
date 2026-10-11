#ifndef ORIGINAL_BRIDGE_SCHEDULER_SHAPE_H
#define ORIGINAL_BRIDGE_SCHEDULER_SHAPE_H
#include "static_abi.h"
#ifdef __cplusplus
extern "C" {
#endif
#define OABI_SCHEDULER_SCHEMA_VERSION 1u
#define OABI_SCHEDULER_PHASE_COUNT 51u
/* Integer source labels only; never callable original targets. */
typedef struct {
    uint32_t phase;
    uint32_t dispatch_block;
    uint32_t call_count;
    uint32_t call_sites[3];
    uint32_t target_labels[3];
    uint32_t conditional_call_mask;
    uint32_t local_flag_store;
    uint32_t wrap_after_normal_advance;
} OAbiSchedulerPhase;
typedef struct {
    uint32_t schema_version;
    char subject_sha256[64];
    char analysis_profile_digest[64];
    uint32_t pointer_bits;
    uint64_t phase;
    uint64_t declared_entry;
    uint64_t declared_return;
    uint32_t claim_full_cycle;
    uint32_t request_runtime_binding;
} OAbiSchedulerInput;
enum {
    OABI_SCHED_RUNTIME_FROZEN=1u<<0,
    OABI_SCHED_TARGET_UNBOUND=1u<<1,
    OABI_SCHED_FRAME_UNBOUND=1u<<2,
    OABI_SCHED_EFFECTS_UNCLOSED=1u<<3,
    OABI_SCHED_CYCLE_UNPROVED=1u<<4,
    OABI_SCHED_MISSING_INPUT=1u<<5,
    OABI_SCHED_IDENTITY_MISMATCH=1u<<6,
    OABI_SCHED_WIDTH_MISMATCH=1u<<7,
    OABI_SCHED_PHASE_UNSUPPORTED=1u<<8,
    OABI_SCHED_ENTRY_UNSUPPORTED=1u<<9,
    OABI_SCHED_RETURN_UNSUPPORTED=1u<<10,
    OABI_SCHED_BOUNDARY_INSUFFICIENT=1u<<11,
    OABI_SCHED_RUNTIME_REQUEST_FORBIDDEN=1u<<12
};
typedef struct {
    OAbiShapeStatus shape_status;
    OAbiBindingDecision binding_decision;
    uint32_t cycle_completion_proved; /* Always zero: declarations are not a trace. */
    uint32_t reasons;
    const OAbiSchedulerPhase *description; /* Own immutable data only. */
} OAbiSchedulerReport;
const OAbiSchedulerPhase *oabi_describe_scheduler_phase(uint64_t phase);
OAbiSchedulerReport oabi_validate_scheduler_declaration(const OAbiSchedulerInput *input);
#ifdef __cplusplus
}
#endif
#endif
