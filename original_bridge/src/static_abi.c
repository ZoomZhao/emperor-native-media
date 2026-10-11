#include "static_abi.h"
#include <limits.h>
#include <string.h>

_Static_assert(CHAR_BIT == 8, "ABI descriptions require eight-bit bytes");
_Static_assert(sizeof(uint32_t) == 4 && sizeof(uint16_t) == 2, "fixed-width description carriers");

/* Closure of a local helper never registers a dynamic receiver's target. */
static const OAbiCallContract calls[] = {
    {0x4aef47,1,0xc8,1,1,{32,0},1,0x35,OABI_RESULT_AL8,0,0,0,0,0,OABI_GUARD_NONE,0,
     "REA_ABI_CONTRACTS.md:phase8; allowlist candidates phase8.predicate"},
    {0x4aef92,1,0x264,1,1,{32,0},0,0,OABI_RESULT_EAX32_BITS,0,0,0,0,1,OABI_GUARD_NONE,0,
     "REA_ABI_CONTRACTS.md:phase8; allowlist candidates phase8.aggregate first site"},
    {0x4aefaa,1,0x264,1,1,{32,0},0,0,OABI_RESULT_EAX32_BITS,0,0,0,0,2,OABI_GUARD_NONE,0,
     "REA_ABI_CONTRACTS.md:phase8; allowlist candidates phase8.aggregate second site"},
    {0x5fc139,1,0,1,1,{32,0},1,1,OABI_RESULT_UNUSED,0,0,0,0,0,OABI_GUARD_SIGNED_DECREMENT32_LE_ZERO,0,
     "REA_ABI_CONTRACTS.md:worker; allowlist candidates worker.release"},
    {0x5fbf47,0,0,0,2,{32,32},0,0,OABI_RESULT_UNKNOWN,1,8,0,0,0,OABI_GUARD_NONE,0x718850,
     "REA_ABI_CONTRACTS.md:construction/registration; allowlist candidates queue.copy"}
};
static const OAbiField fields[] = {
    {OABI_C_VECTOR,8,4,OABI_FIELD_POINTER32,"REA_ABI_CONTRACTS.md:phase8 C begin"},
    {OABI_C_VECTOR,12,4,OABI_FIELD_POINTER32,"REA_ABI_CONTRACTS.md:phase8 C end"},
    {OABI_P_POOL,4,4,OABI_FIELD_POINTER32,"REA_ABI_CONTRACTS.md:phase8 P begin"},
    {OABI_R_RECORD,8,2,OABI_FIELD_SIGNED16,"REA_ABI_CONTRACTS.md:phase8 signed16 ID"},
    {OABI_R_RECORD,12,1,OABI_FIELD_BYTE,"REA_ABI_CONTRACTS.md:phase8 enabled byte"},
    {OABI_O_OBJECT,0,4,OABI_FIELD_POINTER32,"REA_ABI_CONTRACTS.md:phase8 O vptr"},
    {OABI_O_OBJECT,4,1,OABI_FIELD_BYTE,"REA_ABI_CONTRACTS.md:phase8 object filter"},
    {OABI_O_OBJECT,0x14,2,OABI_FIELD_SIGNED16,"allowlist phase8.predicate base_matching_word_candidate"},
    {OABI_O_OBJECT,0x24,2,OABI_FIELD_SIGNED16,"REA_ABI_CONTRACTS.md:phase8 signed short filter"},
    {OABI_Q_QUEUE,0x30,4,OABI_FIELD_POINTER32,"REA_ABI_CONTRACTS.md:worker current payload"},
    {OABI_J_PAYLOAD,4,4,OABI_FIELD_SIGNED32,"REA_ABI_CONTRACTS.md:worker signed decrement result"},
    {OABI_J_PAYLOAD,0x24,4,OABI_FIELD_POINTER32,"REA_ABI_CONTRACTS.md:worker queue receiver field"},
    {OABI_SELECTED_AGG_RECORD,4,4,OABI_FIELD_BITS32,"allowlist selected_phase8_method_contracts aggregate record fields"},
    {OABI_SELECTED_AGG_RECORD,8,4,OABI_FIELD_BITS32,"allowlist selected_phase8_method_contracts aggregate record fields"}
};

const OAbiCallContract *oabi_describe_site(uint64_t site) {
    if (site > UINT32_MAX) return NULL;
    for (size_t i=0; i<sizeof(calls)/sizeof(calls[0]); ++i)
        if (calls[i].call_site_va == site) return &calls[i];
    return NULL;
}
const OAbiField *oabi_described_fields(size_t *count) {
    if (count) *count = sizeof(fields)/sizeof(fields[0]);
    return fields;
}
OAbiReport oabi_validate_static_input(const OAbiInput *input) {
    OAbiReport r = {OABI_SHAPE_REJECTED,OABI_BINDING_DENIED,
                    OABI_RUNTIME_FROZEN|OABI_LIFETIME_UNCLOSED|OABI_EXCEPTION_UNCLOSED,NULL};
    if (!input) { r.reasons |= OABI_MISSING_INPUT; return r; }
    if (input->request_runtime_binding) { r.reasons |= OABI_RUNTIME_REQUEST_FORBIDDEN; return r; }
    if (input->schema_version != OABI_SCHEMA_VERSION ||
        memcmp(input->subject_sha256,OABI_SUBJECT_SHA256,64) ||
        memcmp(input->analysis_profile_digest,OABI_PROFILE_DIGEST,64)) {
        r.reasons |= OABI_IDENTITY_MISMATCH; return r;
    }
    if (input->pointer_bits != 32 || input->call_site_va > UINT32_MAX ||
        input->receiver_guest > UINT32_MAX || input->requested_target_guest > UINT32_MAX ||
        input->declared_vtable_guest > UINT32_MAX || input->declared_vtable_bytes > UINT32_MAX) {
        r.reasons |= OABI_WIDTH_MISMATCH; return r;
    }
    r.description = oabi_describe_site(input->call_site_va);
    if (!r.description) { r.reasons |= OABI_UNKNOWN_SITE; return r; }
    const OAbiCallContract *c = r.description;
    if (c->receiver_in_ecx && !input->receiver_guest) { r.reasons |= OABI_MISSING_RECEIVER; return r; }
    if (input->stack_argument_count != c->stack_argument_count) { r.reasons |= OABI_ARGUMENT_MISMATCH; return r; }
    for (size_t i=0; i<2; ++i) {
        if (input->stack_argument_bits[i] != c->stack_argument_bits[i] ||
            (i >= c->stack_argument_count && input->stack_arguments[i] != 0)) {
            r.reasons |= OABI_ARGUMENT_MISMATCH; return r;
        }
        if (input->stack_arguments[i] > UINT32_MAX) { r.reasons |= OABI_WIDTH_MISMATCH; return r; }
    }
    if (c->literal_argument_known && input->stack_arguments[0] != c->literal_argument) {
        r.reasons |= OABI_ARGUMENT_MISMATCH; return r;
    }
    if (c->virtual_call) {
        r.reasons |= OABI_INSTANCE_UNBOUND|OABI_TABLE_UNKNOWN|OABI_UNKNOWN_TARGET;
        if (input->declared_vtable_bytes) {
            /* Validate only the declared byte interval. It does not prove the actual table's extent. */
            if (!input->declared_vtable_guest || input->declared_vtable_bytes % 4 ||
                input->declared_vtable_guest + input->declared_vtable_bytes > UINT64_C(0x100000000) ||
                (uint64_t)c->vtable_offset_bytes + 4 > input->declared_vtable_bytes) {
                r.reasons |= OABI_DECLARED_SLOT_OUTSIDE; return r;
            }
        }
    } else if (input->requested_target_guest != c->static_direct_target_va) {
        r.reasons |= OABI_UNKNOWN_TARGET;
    } else {
        r.reasons |= OABI_STATIC_TARGET_ONLY;
    }
    r.shape_status = OABI_STATIC_SHAPE_ONLY;
    return r;
}
