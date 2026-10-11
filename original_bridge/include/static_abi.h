#ifndef ORIGINAL_BRIDGE_STATIC_ABI_H
#define ORIGINAL_BRIDGE_STATIC_ABI_H
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Own descriptive data only. No guest reads, code pointers, dispatch or execution API. */
#define OABI_SCHEMA_VERSION 5u
#define OABI_POINTER_BYTES 4u
#define OABI_SELECTED_AGG_RECORD_STRIDE_BYTES 16u
#define OABI_SUBJECT_SHA256 "8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753"
#define OABI_PROFILE_DIGEST "e3402f2d7c487875edf3c6215fa95d67ee1fa133ba68c2390a9338b38656a7b2"

typedef enum { OABI_RESULT_UNKNOWN, OABI_RESULT_AL8, OABI_RESULT_EAX32_BITS, OABI_RESULT_UNUSED } OAbiResultUse;
typedef enum { OABI_GUARD_NONE, OABI_GUARD_SIGNED_DECREMENT32_LE_ZERO } OAbiReleaseGuard;
typedef enum { OABI_FIELD_POINTER32, OABI_FIELD_SIGNED16, OABI_FIELD_BYTE, OABI_FIELD_SIGNED32, OABI_FIELD_BITS32 } OAbiFieldKind;
typedef enum { OABI_C_VECTOR, OABI_P_POOL, OABI_R_RECORD, OABI_O_OBJECT, OABI_Q_QUEUE, OABI_J_PAYLOAD, OABI_SELECTED_AGG_RECORD } OAbiObjectRole;
typedef struct {
    OAbiObjectRole role;
    uint32_t offset;
    uint32_t width_bytes;
    OAbiFieldKind kind;
    const char *source_anchor; /* Published documentation reference, not an executed instruction. */
} OAbiField;
typedef struct {
    uint32_t call_site_va;
    uint32_t virtual_call;
    uint32_t vtable_offset_bytes;
    uint32_t receiver_in_ecx;
    uint32_t stack_argument_count;
    uint32_t stack_argument_bits[2];
    uint32_t literal_argument_known;
    uint32_t literal_argument;
    OAbiResultUse result_use;
    uint32_t caller_cleanup_known;
    uint32_t caller_cleanup_bytes;
    uint32_t callee_cleanup_known; /* Actual dynamic callees remain unknown. */
    uint32_t callee_cleanup_bytes;
    uint32_t aggregate_call_ordinal; /* Distinct first/second sites; never merge them. */
    OAbiReleaseGuard release_guard;
    uint32_t static_direct_target_va; /* Integer label only; never cast to a pointer. */
    const char *source_anchor;
} OAbiCallContract;

/* Wide carriers let validation detect accidental host-width values before any narrowing. */
typedef struct {
    uint32_t schema_version;
    char subject_sha256[64];
    char analysis_profile_digest[64];
    uint32_t pointer_bits;
    uint64_t call_site_va;
    uint64_t receiver_guest;
    uint64_t requested_target_guest;
    uint32_t stack_argument_count;
    uint32_t stack_argument_bits[2];
    uint64_t stack_arguments[2];
    uint64_t declared_vtable_guest;
    uint64_t declared_vtable_bytes; /* 0=unknown; a caller claim is not table-membership evidence. */
    uint32_t request_runtime_binding;
} OAbiInput;

typedef enum { OABI_SHAPE_REJECTED, OABI_STATIC_SHAPE_ONLY } OAbiShapeStatus;
typedef enum { OABI_BINDING_DENIED = 0 } OAbiBindingDecision; /* No allowed state in this release. */
enum {
    OABI_RUNTIME_FROZEN = 1u << 0,
    OABI_IDENTITY_MISMATCH = 1u << 1,
    OABI_UNKNOWN_SITE = 1u << 2,
    OABI_WIDTH_MISMATCH = 1u << 3,
    OABI_ARGUMENT_MISMATCH = 1u << 4,
    OABI_MISSING_RECEIVER = 1u << 5,
    OABI_TABLE_UNKNOWN = 1u << 6,
    OABI_DECLARED_SLOT_OUTSIDE = 1u << 7,
    OABI_UNKNOWN_TARGET = 1u << 8,
    OABI_STATIC_TARGET_ONLY = 1u << 9,
    OABI_INSTANCE_UNBOUND = 1u << 10,
    OABI_LIFETIME_UNCLOSED = 1u << 11,
    OABI_EXCEPTION_UNCLOSED = 1u << 12,
    OABI_RUNTIME_REQUEST_FORBIDDEN = 1u << 13,
    OABI_MISSING_INPUT = 1u << 14
};
typedef struct {
    OAbiShapeStatus shape_status;
    OAbiBindingDecision binding_decision;
    uint32_t reasons;
    const OAbiCallContract *description; /* Points only to this module's static data. */
} OAbiReport;

const OAbiCallContract *oabi_describe_site(uint64_t call_site_va);
const OAbiField *oabi_described_fields(size_t *count);
OAbiReport oabi_validate_static_input(const OAbiInput *input);

#ifdef __cplusplus
}
#endif
#endif
