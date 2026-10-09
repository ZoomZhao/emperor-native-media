/* Exact translated original slices connected to a finite native platform. */
#include "native_cpu.h"
#include "stage11_native_runtime.h"
#include "stage10_native_platform.h"
#include <stdio.h>
#include <stdlib.h>
extern void c_crt_heap_init(CPU*),c_guid_consumer(CPU*);
extern void c_stage11_HeapAlloc(CPU*),c_stage11_HeapReAlloc(CPU*),c_stage11_HeapSize(CPU*),c_stage11_HeapFree(CPU*);
extern void c_stage11_TlsAlloc(CPU*),c_stage11_TlsGetValue(CPU*),c_stage11_TlsSetValue(CPU*),c_stage11_TlsFree(CPU*),c_stage11_CoCreateGuid(CPU*);
extern uint32_t crt_heap_handle,crt_heap_mode;
extern unsigned char crt_globals_page[4096],debug_string_page[4096];
extern void (*ptr_initialize_pointers)(uint64_t);
static _Alignas(16) unsigned char canary[24];
static unsigned abi_calls,heap_checks,tls_checks;
static void check(int value){if(!value) {fprintf(stderr,"stage11 check failed\n");exit(4);}}
void stage11_forbidden_api(uint32_t api){fprintf(stderr,"stage11 unimplemented original branch/API: %x\n",api);exit(86);}
void x86_rep_stosd(CPU *c){
    if(!c->ecx)return;
    uint64_t n=(uint64_t)c->ecx*4;check(n<=65536);
    uint32_t start=c->edi;if(c->eflags&0x400) {check(!c->ecx||start>=n-4);if(c->ecx)start-=(uint32_t)n-4;}
    unsigned char *p=stage11_pointer_host(start,(size_t)n,S11_WRITE);
    for(uint32_t i=0;i<c->ecx;++i)memcpy(p+i*4,&c->eax,4);
    c->edi+=(c->eflags&0x400)?-(uint32_t)n:(uint32_t)n;c->ecx=0;
}
static void init(CPU *c){
    check(initialize_cpu(c,crt_globals_page));memset(native_guest_stack,0xa5,sizeof(native_guest_stack));
    c->pointer_offset=(uintptr_t)native_guest_stack-UINT64_C(0x700f0000);c->esp=0x700fff00;c->eflags=0x202;c->st_cw=0x37f;
    stage11_reset(c->pointer_offset);stage11_region(native_guest_stack,sizeof(native_guest_stack),S11_READ|S11_WRITE);
    stage11_region(&crt_globals_page,4096,S11_READ|S11_WRITE);stage11_region(&debug_string_page,4096,S11_READ);
    stage11_region(canary,sizeof(canary),S11_READ|S11_WRITE);memset(&crt_globals_page,0,4096);
    if(ptr_initialize_pointers)ptr_initialize_pointers(c->pointer_offset);
    check(pointer32(c,c_crt_heap_init)&&pointer32(c,c_guid_consumer));
}
static uint32_t invoke(CPU *c,void(*fn)(CPU*),const uint32_t *args,unsigned n){
    uint32_t before=c->esp;c->ebx=0x13579bdf;c->esi=0x2468ace0;c->edi=0xaabbccdd;c->ebp=0xdeadbeef;
    for(unsigned i=n;i>0;--i) {c->esp-=4;memcpy((void*)(uintptr_t)(c->pointer_offset+c->esp),&args[i-1],4);}
    fn(c);check(c->esp==before&&c->ebx==0x13579bdf&&c->esi==0x2468ace0&&c->edi==0xaabbccdd&&c->ebp==0xdeadbeef);++abi_calls;return c->eax;
}
static uint32_t deterministic(unsigned char out[16]) {
    const unsigned char be[16]={1,0x23,0x45,0x67,0x89,0xab,0x4c,0xde,0x8f,0x10,0x21,0x32,0x43,0x54,0x65,0x76};memcpy(out,be,16);return 0;
}
static uint32_t failure(unsigned char out[16]){(void)out;return 14;}
static void write_bytes(FILE *f,const void *p,size_t n){check(fwrite(p,n,1,f)==1);}
int main(int argc,char **argv){
    if(argc==3&&!strcmp(argv[1],"--negative")){
        CPU c;init(&c);uint32_t h=stage11_HeapCreate_c(0,4096,0);
        if(!strcmp(argv[2],"heap"))stage11_HeapAlloc_c(0x9999,0,16);
        else if(!strcmp(argv[2],"freed")){void *p=stage11_HeapAlloc_c(h,0,16);stage11_HeapFree_c(h,0,p);stage11_HeapSize_c(h,0,p);}
        else if(!strcmp(argv[2],"realloc-flags")){void *p=stage11_HeapAlloc_c(h,0,16);stage11_HeapReAlloc_c(h,16,p,24);}
        else if(!strcmp(argv[2],"tls"))stage11_TlsGetValue_c(64);
        else if(!strcmp(argv[2],"pointer-overflow"))stage11_pointer_host(UINT32_MAX,16,S11_WRITE);
        else if(!strcmp(argv[2],"guid-pointer")){uint32_t a[]={1};invoke(&c,c_stage11_CoCreateGuid,a,1);}
        else return 3;
        return 5;
    }
    check(argc==2);FILE *f=fopen(argv[1],"wb");check(f!=NULL);CPU c;
    for(unsigned mode=0;mode<2;++mode){
        init(&c);uint32_t sp=c.esp;
        c.esp-=4;*(uint32_t*)(uintptr_t)(c.pointer_offset+c.esp)=1;c_crt_heap_init(&c);c.esp+=4;
        check(c.esp==sp&&c.eax==1&&crt_heap_handle==0x3001&&crt_heap_mode==1);
        write_bytes(f,&crt_globals_page,4096);
        uint32_t a[]={crt_heap_handle,0,200};uint32_t object=invoke(&c,c_stage11_HeapAlloc,a,3);
        unsigned char *p=stage11_pointer_host(object,200,S11_READ|S11_WRITE);uint32_t vtable=0xcafebabe;memcpy(p,&vtable,4);
        stage11_test_guid_provider(mode?failure:deterministic);stage10_SetLastError_c(0x13579);
        c.ecx=object;uint32_t preserved[]={c.ebx,c.esi,c.edi,c.ebp};c_guid_consumer(&c);
        check(c.esp==sp&&c.ebx==preserved[0]&&c.esi==preserved[1]&&c.edi==preserved[2]&&c.ebp==preserved[3]);
        uint32_t result[]={c.eax,stage10_GetLastError_c(),c.esp,stage11_debug_count()};
        write_bytes(f,result,sizeof(result));write_bytes(f,p,200);
    }
    init(&c);uint32_t h=stage11_HeapCreate_c(0,4096,0);
    for(unsigned n=0;n<3;++n){
        uint32_t sizes[]={0,7,200};uint32_t a[]={h,8,sizes[n]};uint32_t p=invoke(&c,c_stage11_HeapAlloc,a,3);check(p&&(p&7)==0);
        unsigned char *host=stage11_pointer_host(p,sizes[n],S11_READ|S11_WRITE);for(unsigned i=0;i<sizes[n];++i)check(host[i]==0);
        uint32_t sz[]={h,0,p};check(invoke(&c,c_stage11_HeapSize,sz,3)==sizes[n]);++heap_checks;
        uint32_t slot=invoke(&c,c_stage11_TlsAlloc,NULL,0);uint32_t ta[]={slot};stage10_SetLastError_c(0x13579);
        check(invoke(&c,c_stage11_TlsGetValue,ta,1)==0&&stage10_GetLastError_c()==0);++tls_checks;
        uint32_t set[]={slot,p};check(invoke(&c,c_stage11_TlsSetValue,set,2)==1);check(invoke(&c,c_stage11_TlsGetValue,ta,1)==p);++tls_checks;
        check(invoke(&c,c_stage11_TlsFree,ta,1)==1);check(invoke(&c,c_stage11_HeapFree,sz,3)==1);
    }
    uint32_t a[]={h,0,16};uint32_t old=invoke(&c,c_stage11_HeapAlloc,a,3);unsigned char *p=stage11_pointer_host(old,16,S11_WRITE);memset(p,0x5a,16);
    uint32_t grow[]={h,8,old,24};uint32_t next=invoke(&c,c_stage11_HeapReAlloc,grow,4);p=stage11_pointer_host(next,24,S11_READ);
    for(unsigned i=0;i<24;++i)check(p[i]==(i<16?0x5a:0));++heap_checks;
    uint32_t huge[]={h,0,next,UINT32_MAX};stage10_SetLastError_c(0x13579);check(invoke(&c,c_stage11_HeapReAlloc,huge,4)==0&&stage10_GetLastError_c()==0x13579);
    uint32_t size[]={h,0,next};check(invoke(&c,c_stage11_HeapSize,size,3)==24);++heap_checks;
    stage11_test_guid_provider(NULL);stage10_SetLastError_c(0x13579);
    for(unsigned i=0;i<32;++i){memset(canary,0xa5,sizeof(canary));uint32_t g[]={stage11_pointer32(canary+4)};check(invoke(&c,c_stage11_CoCreateGuid,g,1)==0);check(stage10_GetLastError_c()==0x13579);write_bytes(f,canary,24);}
    uint32_t null[]={0};check(invoke(&c,c_stage11_CoCreateGuid,null,1)==0x80070057&&stage10_GetLastError_c()==0x13579);
    check(stage11_guid_hresult(0)==0&&stage11_guid_hresult(1824)==0&&stage11_guid_hresult(14)==0x8007000e&&stage11_guid_hresult(87)==0x80070057&&stage11_guid_hresult(0x80004005)==0x80004005);
    check(fclose(f)==0);
    printf("{\"native_original_CRT_heap_returned\":true,\"native_original_GUID_success_and_failure_returned\":true,\"ABI_calls\":%u,\"heap_check_groups\":%u,\"TLS_checks\":%u,\"production_GUID_samples\":32,\"pointer_bits\":32,\"production_uses_OS_entropy\":true,\"single_thread_runtime\":true,\"complete_native_initializer_table\":false}\n",abi_calls,heap_checks,tls_checks);
    return 0;
}
