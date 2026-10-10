#ifndef S77_HEAP_NATIVE_H
#define S77_HEAP_NATIVE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { S77_OK=0, S77_BOUNDARY=1, S77_MEMORY=2, S77_METADATA=3 };
typedef struct {uint32_t handle,options;} S77Heap;
typedef struct {uint32_t low,end,permissions;} S77Region;
typedef struct {uint32_t pointer,heap,size;unsigned live;} S77Allocation;
/* Caller owns memory and scheduling. check/map/fill/copy must validate entire
 * ranges; failure must be reported before a write. Every write must be logged
 * by the eventual guest-memory adapter. No host-pointer rebasing is guessed. */
typedef struct {
 void *context;
 int (*check)(void*,uint32_t,uint32_t,unsigned);
 int (*map)(void*,uint32_t,uint32_t);
 int (*fill)(void*,uint32_t,uint32_t,unsigned char);
 int (*copy)(void*,uint32_t,uint32_t,uint32_t);
} S77Memory;
typedef struct {
 S77Heap *heaps;size_t heap_count;
 S77Allocation *allocations;size_t count,capacity;
 uint32_t next,mapped_end;
 unsigned poisoned;
 S77Memory memory;
} S77State;
/* out must be fully zero initialized. Failed import leaves it unchanged. */
int s77_import(S77State*,const S77Heap*,size_t,const S77Allocation*,size_t,uint32_t,uint32_t,S77Memory);
void s77_dispose(S77State*);
int s77_alloc(S77State*,uint32_t,uint32_t,uint32_t,uint32_t*);
int s77_free(S77State*,uint32_t,uint32_t,uint32_t);
int s77_size(const S77State*,uint32_t,uint32_t,uint32_t,uint32_t*);
int s77_realloc(S77State*,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t*);
/* Retires this model's heap/records only; no byte reclamation or OS lifecycle.
 * Other heaps remain available. This is not Windows HeapDestroy equivalence. */
int s77_destroy_heap(S77State*,uint32_t);
/* Invalid/disposed/poisoned state exports zero and leaves output unchanged. */
size_t s77_export_live(const S77State*,S77Allocation*,size_t);
#ifdef __cplusplus
}
#endif
#endif
