#include "static_abi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
static void require(int condition, const char *name) {
    ++checks;
    if (!condition) { fprintf(stderr,"FAIL %s\n",name); exit(1); }
}
static OAbiInput input_for(uint32_t site) {
    OAbiInput in = {0};
    in.schema_version = 5;
    memcpy(in.subject_sha256,OABI_SUBJECT_SHA256,64);
    memcpy(in.analysis_profile_digest,OABI_PROFILE_DIGEST,64);
    in.pointer_bits = 32; in.call_site_va = site;
    in.receiver_guest = 0x1000;
    in.stack_argument_count = site == 0x5fbf47 ? 2 : 1;
    in.stack_argument_bits[0] = 32;
    if (site == 0x5fbf47) {
        in.stack_argument_bits[1] = 32;
        in.stack_arguments[0] = 0x2008; in.stack_arguments[1] = 0x3000;
        in.requested_target_guest = 0x718850;
    } else in.stack_arguments[0] = site == 0x4aef47 ? 0x35 : 1;
    return in;
}
static OAbiReport checked(const OAbiInput *in) {
    unsigned char before[sizeof(OAbiInput)]; if (in) memcpy(before,in,sizeof(before));
    OAbiReport r = oabi_validate_static_input(in);
    require(r.binding_decision == OABI_BINDING_DENIED && (r.reasons & OABI_RUNTIME_FROZEN),
            "no result creates a runtime binding or fake call success");
    require(!in || !memcmp(in,before,sizeof(before)),"validation leaves supplied descriptor unchanged");
    return r;
}
static void rejected(OAbiInput in, uint32_t reason) {
    const OAbiReport r = checked(&in);
    require(r.shape_status == OABI_SHAPE_REJECTED && (r.reasons & reason),"invalid descriptor rejected with precise reason");
}
static void descriptions(void) {
    const OAbiCallContract *predicate = oabi_describe_site(0x4aef47);
    const OAbiCallContract *first = oabi_describe_site(0x4aef92);
    const OAbiCallContract *second = oabi_describe_site(0x4aefaa);
    const OAbiCallContract *release = oabi_describe_site(0x5fc139);
    const OAbiCallContract *copy = oabi_describe_site(0x5fbf47);
    require(predicate && predicate->vtable_offset_bytes == 0xc8 && predicate->result_use == OABI_RESULT_AL8 &&
            predicate->literal_argument == 0x35 && !predicate->callee_cleanup_known,"predicate records AL use, not invented full EAX or cleanup");
    require(first && second && first != second && first->aggregate_call_ordinal == 1 && second->aggregate_call_ordinal == 2 &&
            first->vtable_offset_bytes == 0x264 && second->vtable_offset_bytes == 0x264 &&
            first->result_use == OABI_RESULT_EAX32_BITS && !first->callee_cleanup_known && !second->callee_cleanup_known,
            "two aggregate sites remain distinct; EAX bitpattern signedness/cleanup unclaimed");
    require(release && release->literal_argument == 1 && release->result_use == OABI_RESULT_UNUSED &&
            release->release_guard == OABI_GUARD_SIGNED_DECREMENT32_LE_ZERO && !release->callee_cleanup_known,
            "worker signed <=0 guard described without assigning base candidate ABI to actual target");
    require(copy && copy->stack_argument_count == 2 && copy->caller_cleanup_known && copy->caller_cleanup_bytes == 8 &&
            copy->static_direct_target_va == 0x718850 && copy->result_use == OABI_RESULT_UNKNOWN,
            "copy helper records only proven direct target/stack cleanup; unknown result retained");
    require(!oabi_describe_site(0x427180) && !oabi_describe_site(UINT64_C(0x1004aef47)),
            "callee label and host-width alias are not registered call sites");
    size_t count=0; const OAbiField *f=oabi_described_fields(&count);
    int record_id=0, aggregate_value=0, queue_current=0, payload_refcount=0;
    for (size_t i=0;i<count;++i) {
        require(f[i].source_anchor && f[i].source_anchor[0],"every field description names published source scope");
        if (f[i].role==OABI_R_RECORD && f[i].offset==8) record_id=f[i].width_bytes==2 && f[i].kind==OABI_FIELD_SIGNED16;
        if (f[i].role==OABI_SELECTED_AGG_RECORD && f[i].offset==8) aggregate_value=f[i].width_bytes==4 && f[i].kind==OABI_FIELD_BITS32;
        if (f[i].role==OABI_Q_QUEUE && f[i].offset==0x30) queue_current=f[i].kind==OABI_FIELD_POINTER32;
        if (f[i].role==OABI_J_PAYLOAD && f[i].offset==4) payload_refcount=f[i].kind==OABI_FIELD_SIGNED32;
    }
    require(record_id && aggregate_value && queue_current && payload_refcount &&
            OABI_SELECTED_AGG_RECORD_STRIDE_BYTES==16 && OABI_POINTER_BYTES==4,
            "record ID vs aggregate value and queue vs payload layouts are not conflated");
    require(oabi_described_fields(NULL)==f,"optional field count does not create a target or allocate state");
}
static void valid_shapes_still_deny(void) {
    const uint32_t sites[]={0x4aef47,0x4aef92,0x4aefaa,0x5fc139,0x5fbf47};
    for (size_t i=0;i<5;++i) {
        OAbiInput in=input_for(sites[i]); const OAbiReport r=checked(&in);
        require(r.shape_status==OABI_STATIC_SHAPE_ONLY && r.description &&
                (r.reasons&OABI_LIFETIME_UNCLOSED) && (r.reasons&OABI_EXCEPTION_UNCLOSED),
                "well-formed static request is still explicitly unavailable for runtime binding");
        if (i<4) require((r.reasons&OABI_UNKNOWN_TARGET) && (r.reasons&OABI_INSTANCE_UNBOUND) && (r.reasons&OABI_TABLE_UNKNOWN),
                         "dynamic site missing target/table/instance stays denied");
        else require((r.reasons&OABI_STATIC_TARGET_ONLY) && !(r.reasons&OABI_UNKNOWN_TARGET),
                     "known direct helper is distinguished from unknown but not enabled");
    }
    OAbiInput first=input_for(0x4aef92),second=input_for(0x4aefaa);
    first.stack_arguments[0]=1; second.stack_arguments[0]=9;
    require(checked(&first).shape_status==OABI_STATIC_SHAPE_ONLY && checked(&second).shape_status==OABI_STATIC_SHAPE_ONLY,
            "not requiring runtime equality of the two separately observed stack loads");
    for (size_t i=0;i<4;++i) {
        OAbiInput in=input_for(sites[i]);in.requested_target_guest=i==3?0x5fb520:0x427180;
        in.declared_vtable_guest=0x4000;in.declared_vtable_bytes=0x268;
        OAbiReport r=checked(&in);
        require(r.shape_status==OABI_STATIC_SHAPE_ONLY && (r.reasons&OABI_UNKNOWN_TARGET) &&
                (r.reasons&OABI_TABLE_UNKNOWN) && (r.reasons&OABI_INSTANCE_UNBOUND),
                "static candidate plus caller-claimed range never upgrades to a bound dynamic target");
    }
    OAbiInput direct=input_for(0x5fbf47);direct.requested_target_guest=0x123456;
    require(checked(&direct).reasons&OABI_UNKNOWN_TARGET,"unknown direct target is explicitly denied");
}
static void bad_inputs(void) {
    require(checked(NULL).reasons&OABI_MISSING_INPUT,"missing input cannot be accepted");
    OAbiInput good=input_for(0x4aef47),bad;
    bad=good;bad.schema_version=4;rejected(bad,OABI_IDENTITY_MISMATCH);
    bad=good;bad.subject_sha256[0]='0';rejected(bad,OABI_IDENTITY_MISMATCH);
    bad=good;bad.analysis_profile_digest[0]='0';rejected(bad,OABI_IDENTITY_MISMATCH);
    bad=good;bad.pointer_bits=64;rejected(bad,OABI_WIDTH_MISMATCH);
    bad=good;bad.call_site_va=UINT64_C(0x1004aef47);rejected(bad,OABI_WIDTH_MISMATCH);
    bad=good;bad.receiver_guest=UINT64_C(0x100000000);rejected(bad,OABI_WIDTH_MISMATCH);
    bad=good;bad.requested_target_guest=UINT64_MAX;rejected(bad,OABI_WIDTH_MISMATCH);
    bad=good;bad.declared_vtable_guest=UINT64_MAX;rejected(bad,OABI_WIDTH_MISMATCH);
    bad=good;bad.declared_vtable_bytes=UINT64_MAX;rejected(bad,OABI_WIDTH_MISMATCH);
    bad=good;bad.call_site_va=0x111111;rejected(bad,OABI_UNKNOWN_SITE);
    bad=good;bad.receiver_guest=0;rejected(bad,OABI_MISSING_RECEIVER);
    bad=good;bad.stack_argument_count=UINT32_MAX;rejected(bad,OABI_ARGUMENT_MISMATCH);
    bad=good;bad.stack_argument_bits[0]=16;rejected(bad,OABI_ARGUMENT_MISMATCH);
    bad=good;bad.stack_argument_bits[1]=32;rejected(bad,OABI_ARGUMENT_MISMATCH);
    bad=good;bad.stack_arguments[0]=UINT64_C(0x100000035);rejected(bad,OABI_WIDTH_MISMATCH);
    bad=good;bad.stack_arguments[0]=0x10035;rejected(bad,OABI_ARGUMENT_MISMATCH);
    bad=good;bad.stack_arguments[1]=1;rejected(bad,OABI_ARGUMENT_MISMATCH);
    bad=good;bad.stack_arguments[0]=0;rejected(bad,OABI_ARGUMENT_MISMATCH);
    bad=input_for(0x5fc139);bad.stack_arguments[0]=0x10001;rejected(bad,OABI_ARGUMENT_MISMATCH);
    bad=good;bad.request_runtime_binding=1;rejected(bad,OABI_RUNTIME_REQUEST_FORBIDDEN);
    bad=good;bad.declared_vtable_guest=0x4000;bad.declared_vtable_bytes=0xc8;rejected(bad,OABI_DECLARED_SLOT_OUTSIDE);
    bad=good;bad.declared_vtable_guest=0;bad.declared_vtable_bytes=0xcc;rejected(bad,OABI_DECLARED_SLOT_OUTSIDE);
    bad=good;bad.declared_vtable_guest=0x4000;bad.declared_vtable_bytes=0xcb;rejected(bad,OABI_DECLARED_SLOT_OUTSIDE);
    bad=good;bad.declared_vtable_guest=UINT32_MAX-3;bad.declared_vtable_bytes=0xcc;rejected(bad,OABI_DECLARED_SLOT_OUTSIDE);
    bad=input_for(0x4aef92);bad.declared_vtable_guest=0x4000;bad.declared_vtable_bytes=0x264;rejected(bad,OABI_DECLARED_SLOT_OUTSIDE);
    bad=input_for(0x4aefaa);bad.declared_vtable_guest=0x4000;bad.declared_vtable_bytes=0x267;rejected(bad,OABI_DECLARED_SLOT_OUTSIDE);
    OAbiInput edge=input_for(0x5fc139);edge.declared_vtable_guest=UINT32_MAX-3;edge.declared_vtable_bytes=4;
    require(checked(&edge).shape_status==OABI_STATIC_SHAPE_ONLY,"last four guest bytes accepted as a declared interval without execution");
    edge.declared_vtable_bytes=8;rejected(edge,OABI_DECLARED_SLOT_OUTSIDE);
    OAbiInput aggregate=input_for(0x4aef92);aggregate.stack_arguments[0]=UINT32_MAX;
    require(checked(&aggregate).shape_status==OABI_STATIC_SHAPE_ONLY,"32-bit aggregate argument bitpattern accepted without inventing signedness or business range");
    const uint32_t excluded[]={0x5451a0,0x546c40,0x5dc860,0x506240};
    for (size_t i=0;i<sizeof(excluded)/sizeof(excluded[0]);++i) {
        require(!oabi_describe_site(excluded[i]),"reviewed record-link/formula/order frontier is not a registered contract");
        OAbiInput frontier=input_for(excluded[i]);rejected(frontier,OABI_UNKNOWN_SITE);
    }
}
int main(void) {
    descriptions();valid_shapes_still_deny();bad_inputs();
    printf("PASS original_abi_static %u checks; runtime bindings=0; no guest/code/provider loaded\n",checks);
    return 0;
}
