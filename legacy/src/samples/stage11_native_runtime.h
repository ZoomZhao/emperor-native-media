#ifndef STAGE11_NATIVE_RUNTIME_H
#define STAGE11_NATIVE_RUNTIME_H
#include <stddef.h>
#include <stdint.h>
enum { S11_READ=1,S11_WRITE=2 };
void stage11_reset(uint64_t pointer_offset);
void stage11_region(void *pointer,size_t length,unsigned permissions);
void stage11_require(const void *pointer,size_t length,unsigned permissions);
uint32_t stage11_pointer32(const void *pointer);
void *stage11_pointer_host(uint32_t address,size_t length,unsigned permissions);
uint32_t stage11_CoCreateGuid_c(void *output);
uint32_t stage11_guid_hresult(uint32_t rpc_status);
typedef uint32_t (*stage11_uuid_provider)(unsigned char uuid_be[16]);
void stage11_test_guid_provider(stage11_uuid_provider provider);
uint32_t stage11_HeapCreate_c(uint32_t flags,uint32_t initial,uint32_t maximum);
uint32_t stage11_HeapDestroy_c(uint32_t heap);
void *stage11_HeapAlloc_c(uint32_t heap,uint32_t flags,uint32_t size);
uint32_t stage11_HeapFree_c(uint32_t heap,uint32_t flags,void *pointer);
uint32_t stage11_HeapSize_c(uint32_t heap,uint32_t flags,void *pointer);
void *stage11_HeapReAlloc_c(uint32_t heap,uint32_t flags,void *pointer,uint32_t size);
uint32_t stage11_TlsAlloc_c(void);
uint32_t stage11_TlsSetValue_c(uint32_t index,uint32_t value);
uint32_t stage11_TlsGetValue_c(uint32_t index);
uint32_t stage11_TlsFree_c(uint32_t index);
uint32_t stage11_GetVersionExA_c(void *output);
void stage11_OutputDebugStringA_c(const void *text);
unsigned stage11_debug_count(void);
#endif
