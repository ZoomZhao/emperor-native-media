#ifndef S80_SPARSE_MEMORY_H
#define S80_SPARSE_MEMORY_H
#include "guest_memory.h"
typedef struct {uint64_t low,end;} S80Backing;
typedef struct {S79Memory view;S80Backing backing[64];size_t backing_count;uint64_t offset;} S80Sparse;
int s80_sparse_open(S80Sparse *,const S79Region *,size_t,const char *,uint64_t);
void s80_sparse_close(S80Sparse *);
#endif
