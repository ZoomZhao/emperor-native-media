/* Independent finite bump-allocation model copied from authored stage77 source.
 * Numeric addresses are logical keys in a private vector, not executable mappings.
 * No original instruction, checkpoint loader or engine is included. */
#include "heap_native.h"
#include <stdlib.h>
#include <string.h>
#define HEAP_LOW UINT32_C(0x10000000)
#define HEAP_LIMIT UINT32_C(0x20000000)
#define RECORD_LIMIT 1000000u
static uint64_t rounded(uint32_t n){uint64_t r=((uint64_t)n+7)&~UINT64_C(7);return r<8?8:r;}
static int ready(const S77State*s){
 return s&&!s->poisoned&&s->heaps&&s->allocations&&s->heap_count<=4096&&
  s->count<=s->capacity&&s->capacity>=128&&s->capacity<=RECORD_LIMIT&&
  s->next>=HEAP_LOW&&s->next<=s->mapped_end&&s->mapped_end<=HEAP_LIMIT&&
  !(s->next&7)&&!(s->mapped_end&0xffffff)&&s->memory.check&&s->memory.map&&
  s->memory.fill&&s->memory.copy;
}
static int empty(const S77State*s){
 return s&&!s->heaps&&!s->heap_count&&!s->allocations&&!s->count&&!s->capacity&&
  !s->next&&!s->mapped_end&&!s->poisoned&&!s->memory.context&&!s->memory.check&&
  !s->memory.map&&!s->memory.fill&&!s->memory.copy;
}
static int heap_known(const S77State*s,uint32_t h){for(size_t i=0;i<s->heap_count;i++)if(s->heaps[i].handle==h)return 1;return 0;}
static size_t find(const S77State*s,uint32_t p){
 size_t low=0,high=s->count;
 while(low<high){size_t mid=low+(high-low)/2;uint32_t q=s->allocations[mid].pointer;if(q<p)low=mid+1;else high=mid;}
 return low<s->count&&s->allocations[low].pointer==p&&s->allocations[low].live?low:SIZE_MAX;
}
void s77_dispose(S77State*s){if(!s)return;free(s->heaps);free(s->allocations);memset(s,0,sizeof(*s));}
int s77_import(S77State*out,const S77Heap*heaps,size_t nh,const S77Allocation*rows,size_t nr,uint32_t next,uint32_t mapped,S77Memory memory){
 if(!empty(out)||!heaps||!nh||nh>4096||nr>RECORD_LIMIT||(!rows&&nr)||!memory.check||!memory.map||!memory.fill||!memory.copy||next<HEAP_LOW||next>mapped||mapped>HEAP_LIMIT||(next&7)||(mapped&0xffffff)||!memory.check(memory.context,HEAP_LOW,mapped-HEAP_LOW,3))return S77_METADATA;
 S77State fresh={0};fresh.heap_count=nh;fresh.next=next;fresh.mapped_end=mapped;fresh.memory=memory;
 fresh.heaps=malloc(nh*sizeof(*fresh.heaps));fresh.capacity=nr>128?nr:128;fresh.allocations=malloc(fresh.capacity*sizeof(*fresh.allocations));
 if(!fresh.heaps||!fresh.allocations){s77_dispose(&fresh);return S77_METADATA;}
 memcpy(fresh.heaps,heaps,nh*sizeof(*heaps));
 for(size_t i=0;i<nh;i++){
  if(!heaps[i].handle||(heaps[i].options&~1u)){s77_dispose(&fresh);return S77_METADATA;}
  for(size_t j=0;j<i;j++)if(heaps[j].handle==heaps[i].handle){s77_dispose(&fresh);return S77_METADATA;}
 }
 uint64_t prior=HEAP_LOW;
 for(size_t i=0;i<nr;i++){
  uint64_t actual=rounded(rows[i].size),end=(uint64_t)rows[i].pointer+actual;
  if(rows[i].live!=1||rows[i].pointer<prior||(rows[i].pointer&7)||end>next||!heap_known(&fresh,rows[i].heap)||!memory.check(memory.context,rows[i].pointer,(uint32_t)actual,3)){s77_dispose(&fresh);return S77_METADATA;}
  fresh.allocations[i]=rows[i];prior=end;
 }
 fresh.count=nr;*out=fresh;return S77_OK;
}
static int capacity(S77State*s){
 if(s->count<s->capacity)return S77_OK;
 if(s->count>=RECORD_LIMIT)return S77_BOUNDARY;
 size_t n=s->capacity*2;if(n>RECORD_LIMIT)n=RECORD_LIMIT;
 void*p=realloc(s->allocations,n*sizeof(*s->allocations));if(!p)return S77_METADATA;
 s->allocations=p;s->capacity=n;return S77_OK;
}
int s77_alloc(S77State*s,uint32_t heap,uint32_t flags,uint32_t size,uint32_t*result){
 if(!ready(s)||!result||!heap_known(s,heap)||(flags&~9u))return S77_BOUNDARY;
 uint64_t actual=rounded(size),end=(uint64_t)s->next+actual;
 if(end>HEAP_LIMIT){*result=0;return S77_OK;} /* finite model OOM: no allocation or byte mutation */
 int status=capacity(s);if(status)return status;
 uint32_t new_end=s->mapped_end;
 if(end>new_end){new_end=(uint32_t)((end+0xffffff)&~UINT64_C(0xffffff));if(!s->memory.map(s->memory.context,s->mapped_end,new_end)){s->poisoned=1;return S77_MEMORY;}}
 if(!s->memory.check(s->memory.context,s->next,(uint32_t)actual,3)||!s->memory.fill(s->memory.context,s->next,(uint32_t)actual,(flags&8)?0:0xcd)){s->poisoned=1;return S77_MEMORY;}
 uint32_t pointer=s->next;
 s->allocations[s->count++]=(S77Allocation){pointer,heap,size,1};s->next=(uint32_t)end;s->mapped_end=new_end;*result=pointer;return S77_OK;
}
int s77_size(const S77State*s,uint32_t heap,uint32_t flags,uint32_t p,uint32_t*result){
 if(!ready(s)||!result||(flags&~1u)||!heap_known(s,heap))return S77_BOUNDARY;
 size_t i=find(s,p);if(i==SIZE_MAX||s->allocations[i].heap!=heap)return S77_BOUNDARY;
 *result=s->allocations[i].size;return S77_OK;
}
int s77_free(S77State*s,uint32_t heap,uint32_t flags,uint32_t p){
 if(!ready(s)||(flags&~1u)||!heap_known(s,heap))return S77_BOUNDARY;
 size_t i=find(s,p);if(i==SIZE_MAX||s->allocations[i].heap!=heap)return S77_BOUNDARY;
 s->allocations[i].live=0;return S77_OK;
}
int s77_realloc(S77State*s,uint32_t heap,uint32_t flags,uint32_t p,uint32_t size,uint32_t*result){
 if(!ready(s)||!result||(flags&~9u)||!heap_known(s,heap))return S77_BOUNDARY;
 size_t i=find(s,p);if(i==SIZE_MAX||s->allocations[i].heap!=heap)return S77_BOUNDARY;
 uint32_t keep=s->allocations[i].size<size?s->allocations[i].size:size;
 if(keep&&!s->memory.check(s->memory.context,p,keep,1)){s->poisoned=1;return S77_MEMORY;}
 uint32_t fresh;int status=s77_alloc(s,heap,flags,size,&fresh);if(status)return status;
 if(!fresh){*result=0;return S77_OK;} /* old ownership and bytes retained */
 if(keep&&!s->memory.copy(s->memory.context,fresh,p,keep)){s->poisoned=1;return S77_MEMORY;}
 /* Index survives metadata-vector growth; a saved struct pointer would not. */
 s->allocations[i].live=0;*result=fresh;return S77_OK;
}
/* Retire a model heap and its allocation records. Bump bytes are not reclaimed. */
int s77_destroy_heap(S77State*s,uint32_t heap){
 if(!ready(s)||!heap_known(s,heap))return S77_BOUNDARY;
 for(size_t i=0;i<s->count;i++)if(s->allocations[i].heap==heap)s->allocations[i].live=0;
 for(size_t i=0;i<s->heap_count;i++)if(s->heaps[i].handle==heap){
  size_t remaining=s->heap_count-i-1;
  if(remaining)memmove(s->heaps+i,s->heaps+i+1,remaining*sizeof(*s->heaps));
  s->heap_count--;break;
 }
 return S77_OK;
}
size_t s77_export_live(const S77State*s,S77Allocation*out,size_t room){
 if(!ready(s))return 0;
 size_t n=0;for(size_t i=0;i<s->count;i++)if(s->allocations[i].live){if(out&&n<room)out[n]=s->allocations[i];n++;}return n;
}
