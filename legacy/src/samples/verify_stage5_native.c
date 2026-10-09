/* Test/runtime plumbing only. No authored game rules or scheduler. */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "native_cpu.h"
#include "stage5_layout.h"
extern void c_feedback_update(CPU*);
extern const uint32_t stage5_code_count,stage5_code_sources[];
extern const void *const stage5_code_pointers[];
static unsigned entry_counts[4],forbidden_calls;
static int initialize_stage5(CPU *c){
    if(!initialize_cpu(c,stage5_pages[0]))return 0;
    /* Arithmetic on ESP observes its complete 32-bit value. Use the same
       guest stack VA as the independent engine, not ASLR-dependent low bits.
       pointer_offset is an address translation offset, not a 4-GiB alignment
       requirement. Every native continuation must still fit its 32-bit range. */
    c->pointer_offset=(uintptr_t)native_guest_stack-UINT64_C(0x70000000);
    c->esp=pointer32(c,native_guest_stack+65536-16);
    if(c->esp!=0x7000fff0u)return 0;
    for(unsigned i=0;i<stage5_code_count;i++)if(!pointer32(c,stage5_code_pointers[i]))return 0;
    for(unsigned i=0;i<4;i++)if(!pointer32(c,stage5_pages[i]))return 0;
    return 1;
}
void stage5_observe(uint32_t i){if(i>=4)abort();entry_counts[i]++;}
void stage5_forbidden(uint32_t va){forbidden_calls++;fprintf(stderr,"Unresolved original entry 0x%x executed\n",va);abort();}
/* x86 signed EDX:EAX / r32. Only the reviewed original IDIV reaches this.
   Undefined IDIV flags remain unobserved; division exceptions abort explicitly. */
void x86_idiv_64(CPU *c,uint32_t divisor){
    uint64_t bits=((uint64_t)c->edx<<32)|c->eax;int64_t a;memcpy(&a,&bits,8);int64_t b=(int32_t)divisor;
    if(!b||(a==INT64_MIN&&b==-1))abort();int64_t q=a/b,r=a%b;if(q<INT32_MIN||q>INT32_MAX)abort();c->eax=(uint32_t)q;c->edx=(uint32_t)r;
}
static uint32_t *regp(CPU *c,unsigned i){switch(i){case 0:return &c->eax;case 1:return &c->ecx;case 2:return &c->edx;case 3:return &c->ebx;case 4:return &c->esp;case 5:return &c->ebp;case 6:return &c->esi;default:return &c->edi;}}
static int guarded_call(CPU *c){if(S5_gate!=1)return 65;c_feedback_update(c);return 0;}
static int source_of_pointer(CPU *c,uint32_t p,uint32_t source){
    for(unsigned i=0;i<stage5_code_count;i++)if(pointer32(c,stage5_code_pointers[i])==p&&stage5_code_sources[i]==source)return 1;return 0;
}
int main(int argc,char **argv){
    CPU c;if(!initialize_stage5(&c))return 6;
    if(argc==2&&!strcmp(argv[1],"--negative")){S5_gate=0;int rc=guarded_call(&c);unsigned calls=0;for(unsigned i=0;i<4;i++)calls+=entry_counts[i];printf("{\"gate\":0,\"refusal_code\":%d,\"original_entries_executed\":%u,\"passed\":%s}\n",rc,calls,rc==65&&!calls?"true":"false");return rc==65&&!calls?0:1;}
    if(argc!=2)return 2;FILE *f=fopen(argv[1],"rb");if(!f)return 3;uint32_t h[4],row[90];uint8_t expected[16384];
    if(fread(h,4,4,f)!=4||h[0]!=0x53544735||h[2]!=90||h[3]!=16384)return 4;
    unsigned regs=0,flags=0,stack=0,memory=0,flow=0,pointer_slots=0;
    for(unsigned n=0;n<h[1];n++){
        if(fread(row,4,90,f)!=90||fread(expected,1,16384,f)!=16384)return 5;
        if(row[0]){
            if(!initialize_stage5(&c))return 6;
            for(unsigned i=0;i<4;i++)memset(stage5_pages[i],0,4096);
            memset(native_guest_stack,0xa5,sizeof(native_guest_stack));
            for(unsigned i=0;i<8;i++)if(i!=4)*regp(&c,i)=(row[1]*0x9e3779b9u)^(i*0x13579bdfu)^0xa1b2c3d4u;
            c.eflags=row[10];S5_feedback=row[5];S5_positive=row[6];S5_negative=row[7];S5_negative_cooldown=row[8];S5_positive_cooldown=row[9];
        }
        S5_score=row[2];S5_metric=row[3];S5_restriction=row[4];S5_gate=row[11];memset(entry_counts,0,sizeof(entry_counts));uint32_t sp=c.esp;
        if(guarded_call(&c))return 65;
        unsigned br=0;for(unsigned i=0;i<8;i++)if(*regp(&c,i)!=(i==4?sp:row[12+i]))br|=1u<<i;regs+=br!=0;
        unsigned bf=((c.eflags^row[20])&(row[21]|~0x8d5u))!=0;flags+=bf;
        uint32_t actual[32];memcpy(actual,(void*)(uintptr_t)(c.pointer_offset+sp-128),128);unsigned bs=0;
        for(unsigned i=0;i<32;i++){
            uint32_t marker=row[54+i];if(marker){pointer_slots++;if(!source_of_pointer(&c,actual[i],marker))bs++;}
            else if(actual[i]!=row[22+i])bs++;
        }stack+=bs!=0;
        unsigned bm=0;for(unsigned i=0;i<4;i++)if(memcmp(stage5_pages[i],expected+4096*i,4096))bm++;memory+=bm!=0;
        unsigned bc=0;for(unsigned i=0;i<4;i++)if(entry_counts[i]!=row[86+i])bc++;flow+=bc!=0;
        if((br||bf||bs||bm||bc)&&regs+flags+stack+memory+flow<=12)fprintf(stderr,"round=%u regs=%x flags=%u stack=%u data_pages=%u flow=%u flags_actual=%x expected=%x mask=%x\n",n,br,bf,bs,bm,bc,c.eflags,row[20],row[21]);
    }
    fclose(f);unsigned failures=regs+flags+stack+memory+flow+forbidden_calls;
    printf("{\"rounds\":%u,\"register_failures\":%u,\"defined_and_control_flag_failures\":%u,\"stack_failures\":%u,\"full_data_failures\":%u,\"original_call_flow_failures\":%u,\"forbidden_boundary_calls\":%u,\"exact_mapped_return_slots_checked\":%u,\"data_bytes_per_round\":16384,\"stack_bytes_per_round\":128,\"passed\":%s}\n",h[1],regs,flags,stack,memory,flow,forbidden_calls,pointer_slots,failures?"false":"true");return failures?1:0;
}
