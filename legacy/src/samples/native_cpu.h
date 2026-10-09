/* Minimal ABI used by the generated LLVM module; runtime scaffold only. */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef struct {
    uint32_t eax,ecx,edx,ebx,esp,ebp,esi,edi,eflags,st_top,st_sw_cond,st_cw;
    double st[8];
    uint64_t pointer_offset;
} CPU;
_Static_assert(offsetof(CPU,pointer_offset)==112,"LLVM CPU layout mismatch");
static _Alignas(16) unsigned char native_guest_stack[65536];
static uint32_t pointer32(CPU *c,const void *p) {
    uintptr_t a=(uintptr_t)p;
    if(a<c->pointer_offset || a-c->pointer_offset>UINT32_MAX) return 0;
    return (uint32_t)(a-c->pointer_offset);
}
static int initialize_cpu(CPU *c,const void *anchor) {
    memset(c,0,sizeof(*c));
    c->pointer_offset=(uintptr_t)anchor&~UINT64_C(0xffffffff);
    c->esp=pointer32(c,native_guest_stack+sizeof(native_guest_stack)-16);
    c->eflags=0x3202;c->st_cw=0x037f;
    return c->esp!=0;
}
