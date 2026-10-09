/* Own boundary checks and tracing; contains no translated game instructions. */
#include "stage72_runtime.h"
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
static _cpu *active;static S72Region *regions;static size_t region_count;
static S72Write *logbuf;static uint64_t stepcap,writecap,deadline;static S72Result result;
static jmp_buf failure;static int running;
extern void stage72_entry(_cpu*);
static uint64_t now_ns(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return(uint64_t)t.tv_sec*1000000000+t.tv_nsec;}
static void fail(uint32_t code,uint32_t address){result.status=code;result.fault_address=address;if(running)longjmp(failure,1);}
int stage72_check_regions(const S72Region*r,size_t n){
    uint64_t prior=0;
    for(size_t i=0;i<n;i++){uint64_t end=(uint64_t)r[i].low+r[i].size;
        if(!r[i].size||end>UINT64_C(0x100000000)||r[i].permissions&~7u||(i&&r[i].low<prior))return 1;
        prior=end;
    }return 0;
}
int stage72_check_access(const S72Region*r,size_t n,uint32_t address,unsigned size,unsigned permission){
    if(!size||size>4||(uint64_t)address+size>UINT64_C(0x100000000))return 1;
    uint64_t at=address,end=at+size;
    while(at<end){size_t i;for(i=0;i<n;i++)if(at>=r[i].low&&at<(uint64_t)r[i].low+r[i].size)break;
        if(i==n||(r[i].permissions&permission)!=permission)return 1;
        uint64_t high=(uint64_t)r[i].low+r[i].size;at=high<end?high:end;
    }return 0;
}
static uint32_t guest_address(void*p){uintptr_t a=(uintptr_t)p;uint64_t off=active->_pointer_offset;
    if(a<off||a-off>UINT32_MAX){fail(3,0);return 0;}return(uint32_t)(a-off);}
int stage72_check_returns(const S72Return*r,size_t n,uint64_t off){
    for(size_t i=0;i<n;i++){uintptr_t p=(uintptr_t)r[i].target;if(!p||p<off||p-off>UINT32_MAX)return 1;
        for(size_t j=0;j<i;j++)if(r[i].pc==r[j].pc||r[i].target==r[j].target)return 1;
    }return 0;
}
int stage72_configure(_cpu*c,const S72Region*r,size_t n,uint64_t maxsteps,uint64_t maxwrites,uint64_t absolute_deadline){
    if(stage72_check_regions(r,n)||stage72_check_returns(stage72_return_bindings,stage72_return_bindings_count,c->_pointer_offset)||!maxsteps||!maxwrites||maxwrites>2000000)return 1;
    stage72_dispose();active=c;regions=malloc(n*sizeof(*r));logbuf=malloc(maxwrites*sizeof(*logbuf));
    if(!regions||!logbuf){stage72_dispose();return 2;}memcpy(regions,r,n*sizeof(*r));region_count=n;
    stepcap=maxsteps;writecap=maxwrites;deadline=absolute_deadline;memset(&result,0,sizeof(result));return 0;
}
void stage72_step(uint32_t pc){result.pc=pc;if(result.steps++>=stepcap||now_ns()>deadline)fail(4,0);}
void stage72_guard(void*p,uint32_t size,uint32_t permission){uint32_t a=guest_address(p);if(result.status)return;
    if(stage72_check_access(regions,region_count,a,size,permission))fail(5,a);}
void stage72_note_write(void*p,uint32_t size,uint64_t value){uint32_t a=guest_address(p);if(result.status)return;
    if(result.writes>=writecap){fail(6,a);return;}
    logbuf[result.writes++]=(S72Write){result.pc,a,size,(uint32_t)value};
}
void stage72_validate_rep(_cpu*c,uint32_t bits){
    if(c!=active||bits!=32){fail(7,0);return;}uint64_t count=c->_ecx;if(!count)return;
    uint64_t bytes=count*4,low=c->_edi,end;
    if(c->_eflags&DF){if((count-1)*4>low){fail(8,c->_edi);return;}low-=(count-1)*4;end=(uint64_t)c->_edi+4;}
    else end=low+bytes;
    if(end>UINT64_C(0x100000000)){fail(8,c->_edi);return;}
    /* Real input must be entirely valid before entering the unchanged helper.
       Invalid REP is declined, not claimed equivalent to original partial fault. */
    uint64_t at=low;
    while(at<end){size_t i;for(i=0;i<region_count;i++)if(at>=regions[i].low&&at<(uint64_t)regions[i].low+regions[i].size)break;
        if(i==region_count||!(regions[i].permissions&2)){fail(8,(uint32_t)at);return;}
        uint64_t hi=(uint64_t)regions[i].low+regions[i].size;at=hi<end?hi:end;
    }
    if(result.steps+count>stepcap||result.writes+count>writecap){fail(4,c->_edi);return;}result.steps+=count;
}
void *stage72_lookup_return(uint32_t pc){
    for(uint32_t i=0;i<stage72_return_bindings_count;i++)if(stage72_return_bindings[i].pc==pc){
        uintptr_t target=(uintptr_t)stage72_return_bindings[i].target;
        if(target<active->_pointer_offset||target-active->_pointer_offset>UINT32_MAX){fail(9,pc);return NULL;}
        return stage72_return_bindings[i].target;
    }fail(10,pc);return NULL;
}
void stage72_stop_c(_cpu*c){if(c!=active||c->_esp!=0x700ffd8c){fail(11,c->_esp);return;}result.pc=0x4ac333;result.status=0;}
int stage72_run(void){running=1;if(!setjmp(failure))stage72_entry(active);running=0;if(!result.status&&result.pc!=0x4ac333)result.status=12;return result.status;}
const S72Result *stage72_result(void){return &result;}
const S72Write *stage72_writes(void){return logbuf;}
void stage72_dispose(void){free(regions);free(logbuf);regions=NULL;logbuf=NULL;active=NULL;region_count=0;}
