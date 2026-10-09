#ifndef S78_GUARDED_HELPERS_H
#define S78_GUARDED_HELPERS_H
#include <stddef.h>
#include <stdint.h>
/* Exact integer/FP container prefix of the approved SR ABI. Architectural
 * FP80, segment caches and the complete engine context stay in a sidecar;
 * they are never converted to the double slots to claim full CPU restoration. */
typedef struct { uint32_t r[9],st_top,st_sw_cond,st_cw; double st[8];
 uint64_t pointer_offset; int64_t st_result; void *stack_bottom,*stack_top; } S78CPU;
enum {S78_EAX,S78_ECX,S78_EDX,S78_EBX,S78_ESP,S78_EBP,S78_ESI,S78_EDI,S78_FLAGS};
enum {S78_OK,S78_READ_BOUNDARY,S78_WRITE_BOUNDARY,S78_ADDRESS_BOUNDARY,
 S78_BUDGET_BOUNDARY,S78_SEGMENT_UNKNOWN,S78_DOMAIN_UNKNOWN,S78_TARGET_DENIED};
typedef int (*S78Read)(void *,uint32_t,size_t,uint8_t *);
typedef int (*S78Write)(void *,uint32_t,size_t,const uint8_t *);
/* A callback either completes this whole element, or returns failure with no
 * intra-element effects. This is a stated adapter contract, not a proof of
 * arbitrary x86 page-fault/Unicorn intra-element write behavior. */
typedef struct {S78Read read;S78Write write;void *opaque;uint64_t budget;
 uint32_t fs_base,fs_limit,active_teb;int fs_cache_proved,fs_writable;
 int flat_DS_ES_proved;} S78Context;
typedef struct {int status;uint64_t completed;uint32_t last_byte;} S78Result;
typedef struct {uint32_t site;const uint32_t *targets;size_t count;int proved;} S78Domain;
int s78_span(uint32_t start,uint32_t count,unsigned width,int backward,
 uint64_t *low,uint64_t *end);
S78Result s78_rep(S78CPU *,S78Context *,int operation,unsigned width);
/* operation 0 MOVS, 1 STOS, 2 REPNE SCASB. No SEH dispatcher is implemented. */
int s78_fs_read(S78Context *,uint32_t,uint32_t *);
int s78_fs_write(S78Context *,uint32_t,uint32_t);
int s78_target(const S78Domain *,size_t,uint32_t,uint32_t);
#endif
