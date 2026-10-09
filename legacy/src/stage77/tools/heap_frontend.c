/* Exact guest-uint32 ABI. The active context and failure boundary must be
 * provided by a complete reviewed guest runtime; no fallback success exists. */
#include "heap_native.h"
extern S77State *stage77_active_heap_context(void);
extern void stage77_platform_boundary(unsigned kind,unsigned heap,unsigned pointer) __attribute__((noreturn));
static S77State *context(unsigned heap,unsigned pointer){S77State*s=stage77_active_heap_context();if(!s)stage77_platform_boundary(S77_BOUNDARY,heap,pointer);return s;}
uint32_t stage77_HeapAlloc_c(uint32_t heap,uint32_t flags,uint32_t size){uint32_t p;int result=s77_alloc(context(heap,0),heap,flags,size,&p);if(result)stage77_platform_boundary(result,heap,0);return p;}
uint32_t stage77_HeapFree_c(uint32_t heap,uint32_t flags,uint32_t p){int result=s77_free(context(heap,p),heap,flags,p);if(result)stage77_platform_boundary(result,heap,p);return 1;}
uint32_t stage77_HeapSize_c(uint32_t heap,uint32_t flags,uint32_t p){uint32_t size;int result=s77_size(context(heap,p),heap,flags,p,&size);if(result)stage77_platform_boundary(result,heap,p);return size;}
uint32_t stage77_HeapReAlloc_c(uint32_t heap,uint32_t flags,uint32_t p,uint32_t size){uint32_t fresh;int result=s77_realloc(context(heap,p),heap,flags,p,size,&fresh);if(result)stage77_platform_boundary(result,heap,p);return fresh;}
