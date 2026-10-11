#include "scheduler_shape.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
static void check(int yes) {++checks;if(!yes){fprintf(stderr,"failed check %u\n",checks);exit(1);}}
static OAbiSchedulerInput input(uint64_t phase) {
    OAbiSchedulerInput x={0};x.schema_version=OABI_SCHEDULER_SCHEMA_VERSION;
    memcpy(x.subject_sha256,OABI_SUBJECT_SHA256,64);memcpy(x.analysis_profile_digest,OABI_PROFILE_DIGEST,64);
    x.pointer_bits=32;x.phase=phase;x.declared_entry=0x4ac2b0;x.declared_return=0x5371c4;return x;
}
static void rejected(OAbiSchedulerInput x,uint32_t reason) {
    OAbiSchedulerReport r=oabi_validate_scheduler_declaration(&x);
    check(r.shape_status==OABI_SHAPE_REJECTED);check((r.reasons&reason)!=0);
    check(r.binding_decision==OABI_BINDING_DENIED && !r.cycle_completion_proved);
}
int main(void) {
    OAbiSchedulerReport absent=oabi_validate_scheduler_declaration(NULL);
    check(absent.shape_status==OABI_SHAPE_REJECTED && (absent.reasons&OABI_SCHED_MISSING_INPUT));
    check(absent.binding_decision==OABI_BINDING_DENIED && !absent.cycle_completion_proved);
    const uint64_t boundary_phases[]={0,8,13,30,50};
    for(size_t i=0;i<sizeof(boundary_phases)/sizeof(boundary_phases[0]);++i) {
        OAbiSchedulerInput x=input(boundary_phases[i]),before=x;
        OAbiSchedulerReport r=oabi_validate_scheduler_declaration(&x);
        check(r.shape_status==OABI_STATIC_SHAPE_ONLY);
        check((r.reasons&(OABI_SCHED_TARGET_UNBOUND|OABI_SCHED_FRAME_UNBOUND|OABI_SCHED_CYCLE_UNPROVED))==
              (OABI_SCHED_TARGET_UNBOUND|OABI_SCHED_FRAME_UNBOUND|OABI_SCHED_CYCLE_UNPROVED));
        check(r.binding_decision==OABI_BINDING_DENIED && !r.cycle_completion_proved);
        check(!memcmp(&x,&before,sizeof(x)));
        x.claim_full_cycle=1;rejected(x,OABI_SCHED_BOUNDARY_INSUFFICIENT);
    }
    OAbiSchedulerInput good=input(8),bad=good;
    bad.declared_entry=0x4aeeb0;bad.declared_return=0x4ac33d;rejected(bad,OABI_SCHED_ENTRY_UNSUPPORTED);
    bad=good;bad.declared_return=0x4ac33d;rejected(bad,OABI_SCHED_RETURN_UNSUPPORTED);
    bad=good;bad.phase=51;rejected(bad,OABI_SCHED_PHASE_UNSUPPORTED);
    bad=good;bad.phase=UINT64_C(0x100000008);rejected(bad,OABI_SCHED_WIDTH_MISMATCH);
    bad=good;bad.phase=UINT64_MAX;rejected(bad,OABI_SCHED_WIDTH_MISMATCH);
    bad=good;bad.declared_entry=UINT64_C(0x1004ac2b0);rejected(bad,OABI_SCHED_WIDTH_MISMATCH);
    bad=good;bad.declared_return=UINT64_C(0x1005371c4);rejected(bad,OABI_SCHED_WIDTH_MISMATCH);
    bad=good;bad.pointer_bits=64;rejected(bad,OABI_SCHED_WIDTH_MISMATCH);
    bad=good;bad.schema_version=2;rejected(bad,OABI_SCHED_IDENTITY_MISMATCH);
    for(size_t i=0;i<64;++i) {
        bad=good;bad.subject_sha256[i]^=1;rejected(bad,OABI_SCHED_IDENTITY_MISMATCH);
        bad=good;bad.analysis_profile_digest[i]^=1;rejected(bad,OABI_SCHED_IDENTITY_MISMATCH);
    }
    bad=good;bad.request_runtime_binding=1;rejected(bad,OABI_SCHED_RUNTIME_REQUEST_FORBIDDEN);
    check(oabi_describe_scheduler_phase(51)==NULL && oabi_describe_scheduler_phase(UINT64_MAX)==NULL);
    OAbiSchedulerPhase snapshots[51];
    for(size_t i=0;i<51;++i) snapshots[i]=*oabi_describe_scheduler_phase(i);
    for(size_t i=0;i<51;++i) {OAbiSchedulerInput x=input(i);(void)oabi_validate_scheduler_declaration(&x);}
    for(size_t i=0;i<51;++i) check(!memcmp(snapshots+i,oabi_describe_scheduler_phase(i),sizeof(*snapshots)));
    printf("PASS scheduler declaration %u checks; binding denied; cycle proof=0\n",checks);return 0;
}
