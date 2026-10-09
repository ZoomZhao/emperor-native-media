#ifndef S80_RUNTIME_H
#define S80_RUNTIME_H
#include "sparse_memory.h"
#include "thread_native.h"
#include "guarded_helpers.h"
#include "heap_native.h"
#include <setjmp.h>
typedef struct {uint32_t status,selectors[6],cr0;uint64_t gdtr[4],ldtr[4],fs_base,untrusted_gs;uint32_t fpcw,fpsw,fptag;uint64_t mantissa[8];uint16_t exponent[8];} S80Arch;
typedef struct {uint32_t pc;void *target;} S80Binding;
typedef struct {uint32_t pc,address,size,value;} S80Write;
typedef struct {uint32_t site,reg,value,slot,target;} S81DomainCase;
typedef struct {S79Thread threads[4];S79Threads thread_set;S80Arch arch[4];S78CPU cpu;S80Sparse *memory;S78Context helpers;S77State heap;uint32_t pc,status,fault,pending_address,pending_size;int pending,armed,authored_execution;uint64_t steps,step_cap,write_cap;S80Write *trace;size_t writes;const S80Binding *returns,*targets;size_t return_count,target_count;jmp_buf escape;} S80Runtime;
int s80_import_threads(S80Runtime *,const char *,uint64_t);
int s80_export_initial_threads(const S80Runtime *,const char *);
int s80_connect(S80Runtime *,S80Sparse *,S80Write *,size_t,uint64_t);
void s80_activate(S80Runtime *);
int s80_set_bindings(S80Runtime *,const S80Binding *,size_t,const S80Binding *,size_t);
int s81_import_heap(S80Runtime *,const char *);
int s80_switch_thread(S80Runtime *,uint32_t);
void s80_save_core(S80Runtime *);
int s80_read(void *,uint32_t,size_t,uint8_t *);
int s80_write(void *,uint32_t,size_t,const uint8_t *);
void stage72_step(uint32_t);
void stage72_guard(void *,uint32_t,uint32_t);
void stage72_note_write(void *,uint32_t,uint64_t);
void *stage72_lookup_return(uint32_t);
void *stage77_lookup_target(uint32_t,uint32_t);
enum {S80_CPU_FORMAT=100,S80_MEMORY,S80_BUDGET,S80_TRACE_PROTOCOL,S80_BINDING,S80_DYNAMIC_DOMAIN,S80_NO_DRIVER};
#endif
