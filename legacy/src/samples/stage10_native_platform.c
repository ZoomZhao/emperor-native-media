/* Self-authored arm64 platform adapter, no Emperor logic or external runtime. */
#include "stage10_native_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static _Thread_local uint32_t last_error;
static _Thread_local uintptr_t region_start;
static _Thread_local size_t region_length;
static _Thread_local int region_writable;

static void boundary(const char *reason) {
    fprintf(stderr,"stage10 native boundary: %s\n",reason);
    exit(86);
}
void stage10_bind_buffer(void *buffer,size_t length,int writable) {
    region_start=(uintptr_t)buffer;region_length=length;region_writable=writable;
}
static void require_bytes(const void *pointer,size_t length,int write) {
    uintptr_t address=(uintptr_t)pointer;
    if(!address || address<region_start || length>region_length ||
       address-region_start>region_length-length || (write&&!region_writable))
        boundary("unregistered, truncated or read-only buffer; exception semantics unsupported");
}
uint32_t stage10_GetACP_c(void) { return 936; }
uint32_t stage10_GetVersion_c(void) { return UINT32_C(0x1db10106); }
uint32_t stage10_GetLastError_c(void) { return last_error; }
void stage10_SetLastError_c(uint32_t value) { last_error=value; }

uint32_t stage10_GetCPInfo_c(uint32_t cp,void *output) {
    unsigned char bytes[18]={2,0,0,0,0x3f,0,0x81,0xfe,0,0,0,0,0,0,0,0,0,0};
    size_t length=18;
    if(cp==UINT32_MAX) { last_error=87;return 0; }
    switch(cp) {
      case 0:case 1:case 3:case 936:break;
      case 2:bytes[6]=0xa1;bytes[7]=0xa9;bytes[8]=0xb0;bytes[9]=0xf7;break;
      case 65001:bytes[0]=4;bytes[6]=0;bytes[7]=0;length=8;break;
      case 1252:bytes[0]=1;bytes[6]=0;bytes[7]=0;break;
      default:boundary("unmeasured GetCPInfo code page");
    }
    require_bytes(output,length,1);memcpy(output,bytes,length);return 1;
}
uint32_t stage10_GetVersionExA_c(void *output) {
    uint32_t size;require_bytes(output,4,0);memcpy(&size,output,4);
    if(size!=148&&size!=156) { last_error=122;return 0; }
    unsigned char *bytes=output;
    /* Validate all writes before modifying any byte. The caller's size word,
     * unused service-pack tail and EX reserved byte remain untouched. */
    require_bytes(bytes+4,16,1);require_bytes(bytes+20,15,1);
    if(size==156)require_bytes(bytes+148,7,1);
    const uint32_t fields[4]={6,1,7601,2};
    const unsigned char extra[7]={1,0,0,0,0,0,1};
    memcpy(bytes+4,fields,16);memcpy(bytes+20,"Service Pack 1",15);
    if(size==156)memcpy(bytes+148,extra,7);
    return 1;
}
