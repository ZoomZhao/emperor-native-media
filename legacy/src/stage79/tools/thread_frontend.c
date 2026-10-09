#include "thread_native.h"
extern S79Threads *stage79_active_threads(void);
extern void stage79_thread_boundary(int) __attribute__((noreturn));
static S79Threads *active(void){S79Threads *r=stage79_active_threads();if(!r||!r->threads||r->current>=r->count)stage79_thread_boundary(S79_THREAD_UNKNOWN);return r;}
uint32_t stage79_GetCurrentThreadId_c(void){return s79_thread_id(active());}
uint32_t stage79_GetLastError_c(void){return s79_last_error(active());}
uint32_t stage79_SetLastError_c(uint32_t value){s79_set_error(active(),value);return 0;/* exact fixed reference ABI; Win32 return type is void */}
uint32_t stage79_TlsGetValue_c(uint32_t index){uint32_t value;int s=s79_tls_get(active(),index,&value);if(s)stage79_thread_boundary(s);return value;}
uint32_t stage79_TlsSetValue_c(uint32_t index,uint32_t value){int s=s79_tls_set(active(),index,value);if(s)stage79_thread_boundary(s);return 1;}
