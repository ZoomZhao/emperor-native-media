/* Compiled API boundary only. No game library is linked into the own tests.
 * The current-context and abort hooks remain unresolved in the build archive. */
#include "guarded_helpers.h"
extern S78Context *stage78_current_context(void);
extern void stage78_abort_boundary(int) __attribute__((noreturn));
static S78Result run(S78CPU *cpu,int op,unsigned width){
 S78Context *ctx=stage78_current_context();if(!ctx)stage78_abort_boundary(S78_SEGMENT_UNKNOWN);
 S78Result r=s78_rep(cpu,ctx,op,width);if(r.status)stage78_abort_boundary(r.status);return r;
}
void x86_rep_movsb(S78CPU *c){run(c,0,1);}
void x86_rep_movsd(S78CPU *c){run(c,0,4);}
void x86_rep_stosd(S78CPU *c){run(c,1,4);}
uint32_t x86_repne_scasb(S78CPU *c){return run(c,2,1).last_byte;}
uint32_t x86_read_fs_dword(uint32_t o){
 S78Context *c=stage78_current_context();uint32_t v;int s=c?s78_fs_read(c,o,&v):S78_SEGMENT_UNKNOWN;
 if(s)stage78_abort_boundary(s);return v;
}
void x86_write_fs_dword(uint32_t o,uint32_t v){
 S78Context *c=stage78_current_context();int s=c?s78_fs_write(c,o,v):S78_SEGMENT_UNKNOWN;
 if(s)stage78_abort_boundary(s);
}
