/* Self-authored finite platform runtime. Source program bytes are elsewhere. */
#include "stage11_native_runtime.h"
#include "stage10_native_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {uintptr_t start;size_t size;unsigned permission;} Region;
typedef struct {void *pointer;uint32_t heap,size;int live;} Block;
static _Thread_local Region regions[32];
static _Thread_local unsigned region_count,heap_count,block_count,tls_count,debug_count;
static _Thread_local uint64_t offset;
static _Thread_local unsigned char tls_live[64];
static _Thread_local uint32_t tls_values[64];
static _Thread_local Block blocks[128];
static _Thread_local stage11_uuid_provider provider;
static _Alignas(16) unsigned char heap_arena[65536];
static _Thread_local size_t heap_used;
static void stop(const char *reason) {fprintf(stderr,"stage11 boundary: %s\n",reason);exit(86);}
void stage11_reset(uint64_t pointer_offset) {
    offset=pointer_offset;region_count=heap_count=block_count=tls_count=debug_count=0;heap_used=0;provider=NULL;
    memset(tls_live,0,sizeof(tls_live));memset(tls_values,0,sizeof(tls_values));memset(blocks,0,sizeof(blocks));
    stage10_SetLastError_c(0);stage11_region(heap_arena,sizeof(heap_arena),S11_READ|S11_WRITE);
}
void stage11_region(void *p,size_t n,unsigned permissions) {
    if(region_count>=32||!p||!n||n>UINTPTR_MAX-(uintptr_t)p)stop("region contract");
    regions[region_count++]=(Region){(uintptr_t)p,n,permissions};
}
void stage11_require(const void *p,size_t n,unsigned permissions) {
    uintptr_t a=(uintptr_t)p;
    for(unsigned i=0;i<region_count;++i) {
        Region r=regions[i];
        if(a>=r.start&&n<=r.size&&a-r.start<=r.size-n&&(r.permission&permissions)==permissions)return;
    }
    stop("unregistered or invalid memory; exception dispatch unsupported");
}
uint32_t stage11_pointer32(const void *p) {
    uintptr_t a=(uintptr_t)p;if(!p)return 0;
    if(a<offset||a-offset>UINT32_MAX)stop("host address outside 32-bit domain");return (uint32_t)(a-offset);
}
void *stage11_pointer_host(uint32_t a,size_t n,unsigned permissions) {
    if(!a||n>UINT64_C(0x100000000)-a)stop("guest null/overflow");
    void *p=(void*)(uintptr_t)(offset+a);stage11_require(p,n,permissions);return p;
}
uint32_t stage11_guid_hresult(uint32_t status) {
    if(status==0||status==1824)return 0;
    return status&0x80000000?status:0x80070000|(status&65535);
}
void stage11_test_guid_provider(stage11_uuid_provider p) {provider=p;}
uint32_t stage11_CoCreateGuid_c(void *output) {
    if(!output)return 0x80070057;
    stage11_require(output,16,S11_WRITE);
    unsigned char be[16];uint32_t status=0;
    if(provider)status=provider(be);
    else {arc4random_buf(be,16);be[6]=(be[6]&15)|0x40;be[8]=(be[8]&63)|0x80;}
    uint32_t result=stage11_guid_hresult(status);
    if(result)return result;
    unsigned char le[16]={be[3],be[2],be[1],be[0],be[5],be[4],be[7],be[6]};
    memcpy(le+8,be+8,8);memcpy(output,le,16);return 0;
}
static void heap_flags(uint32_t flags) {if(flags&~9u)stop("unimplemented heap flags");}
static void heap_valid(uint32_t h) {if(h<0x3001||h>=0x3001+heap_count)stop("invalid heap handle");}
static Block *block_valid(uint32_t h,void *p) {
    heap_valid(h);for(unsigned i=0;i<block_count;++i)if(blocks[i].live&&blocks[i].pointer==p&&blocks[i].heap==h)return &blocks[i];
    stop("invalid allocation ownership");return NULL;
}
uint32_t stage11_HeapCreate_c(uint32_t flags,uint32_t initial,uint32_t maximum) {
    (void)initial;if(flags&~1u||maximum||heap_count>=8)stop("unmeasured HeapCreate contract");return 0x3001+heap_count++;
}
uint32_t stage11_HeapDestroy_c(uint32_t h) {
    heap_valid(h);for(unsigned i=0;i<block_count;++i)if(blocks[i].live&&blocks[i].heap==h)stop("destroy live heap not implemented");
    stop("HeapDestroy life cycle not implemented");return 0;
}
void *stage11_HeapAlloc_c(uint32_t h,uint32_t flags,uint32_t n) {
    heap_valid(h);heap_flags(flags);uint64_t rounded=((uint64_t)n+7)&~UINT64_C(7);if(rounded<8)rounded=8;
    if(rounded>sizeof(heap_arena)-heap_used||block_count>=128)return NULL;
    void *p=heap_arena+heap_used;heap_used+=(size_t)rounded;memset(p,flags&8?0:0xcd,(size_t)rounded);
    blocks[block_count++]=(Block){p,h,n,1};return p;
}
uint32_t stage11_HeapFree_c(uint32_t h,uint32_t flags,void *p) {
    if(flags&~1u)stop("HeapFree flags");Block *b=block_valid(h,p);b->live=0;return 1;
}
uint32_t stage11_HeapSize_c(uint32_t h,uint32_t flags,void *p) {
    if(flags&~1u)stop("HeapSize flags");return block_valid(h,p)->size;
}
void *stage11_HeapReAlloc_c(uint32_t h,uint32_t flags,void *p,uint32_t n) {
    heap_flags(flags);Block *old=block_valid(h,p);uint32_t error=stage10_GetLastError_c();
    void *next=stage11_HeapAlloc_c(h,flags,n);stage10_SetLastError_c(error);
    if(next) {memcpy(next,p,old->size<n?old->size:n);old->live=0;}return next;
}
static void tls_valid(uint32_t i) {if(i>=64||!tls_live[i])stop("unmeasured invalid TLS index");}
uint32_t stage11_TlsAlloc_c(void) {
    if(tls_count>=64)stop("TLS capacity beyond finite contract");uint32_t i=tls_count++;tls_live[i]=1;tls_values[i]=0;return i;
}
uint32_t stage11_TlsSetValue_c(uint32_t i,uint32_t v) {tls_valid(i);tls_values[i]=v;return 1;}
uint32_t stage11_TlsGetValue_c(uint32_t i) {tls_valid(i);stage10_SetLastError_c(0);return tls_values[i];}
uint32_t stage11_TlsFree_c(uint32_t i) {tls_valid(i);tls_live[i]=0;return 1;}
uint32_t stage11_GetVersionExA_c(void *p) {
    uint32_t size;stage11_require(p,4,S11_READ);memcpy(&size,p,4);
    size_t n=(size==148||size==156)?size:4;
    stage11_require(p,n,(n==4)?S11_READ:S11_READ|S11_WRITE);
    stage10_bind_buffer(p,n,1);return stage10_GetVersionExA_c(p);
}
void stage11_OutputDebugStringA_c(const void *text) {
    const unsigned char *p=text;
    for(unsigned i=0;i<4096;++i) {stage11_require(p+i,1,S11_READ);if(!p[i]) {++debug_count;return;}}
    stop("unterminated debug text");
}
unsigned stage11_debug_count(void) {return debug_count;}
