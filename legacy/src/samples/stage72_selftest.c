/* Self-authored buffers/addresses only. Never links a generated game object. */
#include "stage72_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
S72Return stage72_return_bindings[1];uint32_t stage72_return_bindings_count=1;
void stage72_entry(_cpu*c){stage72_stop_c(c);}
int main(void){unsigned cases=0;S72Region r[]={{0x1000,8,3},{0x1008,8,3},{0x1020,8,1}};
    assert(!stage72_check_regions(r,3));cases++;
    for(unsigned n=1;n<=4;n++){assert(!stage72_check_access(r,3,0x1007,n,3));cases++;}
    assert(stage72_check_access(r,3,0x100e,4,1));cases++;
    assert(stage72_check_access(r,3,0x1020,1,2));cases++;
    assert(!stage72_check_access(r,3,0x1020,4,1));cases++;
    assert(stage72_check_access(r,3,0xfffffffe,4,1));cases++;
    assert(stage72_check_access(r,3,0x1000,0,1));cases++;
    assert(stage72_check_access(r,3,0x1000,8,1));cases++;
    S72Region alias[]={{0x1000,8,3},{0x1004,8,3}};assert(stage72_check_regions(alias,2));cases++;
    S72Region dup[]={{0x1000,8,3},{0x1000,8,3}};assert(stage72_check_regions(dup,2));cases++;
    S72Region overflow[]={{0xfffffff0,32,3}};assert(stage72_check_regions(overflow,1));cases++;
    S72Region zero[]={{0x1000,0,3}};assert(stage72_check_regions(zero,1));cases++;
    S72Return a[]={{1,(void*)UINT64_C(0x140000000)},{2,(void*)UINT64_C(0x140000100)}};
    assert(!stage72_check_returns(a,2,UINT64_C(0x100000000)));cases++;
    a[1].pc=1;assert(stage72_check_returns(a,2,UINT64_C(0x100000000)));cases++;
    a[1].pc=2;a[1].target=a[0].target;assert(stage72_check_returns(a,2,UINT64_C(0x100000000)));cases++;
    a[1].target=(void*)UINT64_C(0x200000000);assert(stage72_check_returns(a,2,UINT64_C(0x100000000)));cases++;
    a[1].target=(void*)UINT64_C(0xffffffff);assert(stage72_check_returns(a,2,UINT64_C(0x100000000)));cases++;
    uint8_t bytes[64]={0};_cpu c={0};c._pointer_offset=(uintptr_t)bytes-0x1000;stage72_return_bindings[0]=(S72Return){1,(void*)(uintptr_t)(c._pointer_offset+0x3000)};
    assert(!stage72_configure(&c,r,3,100,100,UINT64_MAX));cases++;
    stage72_step(0x1234);
    for(unsigned n=1;n<=4;n*=2){stage72_guard(bytes+1,n,2);assert(!stage72_result()->status);uint32_t value=0x87654321;memcpy(bytes+1,&value,n);stage72_note_write(bytes+1,n,value& (n==4?UINT32_MAX:(1u<<(8*n))-1));cases++;}
    assert(stage72_result()->writes==3&&stage72_writes()[2].address==0x1001);cases++;
    stage72_guard((void*)(uintptr_t)(c._pointer_offset-1),1,1);assert(stage72_result()->status==3);cases++;
    assert(!stage72_configure(&c,r,3,100,100,UINT64_MAX));c._ecx=2;c._edi=0x1004;c._eflags=0;stage72_validate_rep(&c,32);assert(!stage72_result()->status);cases++;
    c._eflags=DF;stage72_validate_rep(&c,32);assert(!stage72_result()->status);cases++;
    c._edi=0;c._ecx=2;stage72_validate_rep(&c,32);assert(stage72_result()->status==8);cases++;
    assert(!stage72_configure(&c,r,3,100,100,UINT64_MAX));c._edi=0x100c;c._ecx=2;c._eflags=0;stage72_validate_rep(&c,32);assert(stage72_result()->status==8&&stage72_result()->writes==0);cases++;
    stage72_dispose();printf("{\"passed\":true,\"cases\":%u,\"game_object_linked\":false,\"REP_invalid_declined_not_fault_equivalence\":true}\n",cases);return 0;
}
