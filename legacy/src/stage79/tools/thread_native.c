#include "thread_native.h"
int s79_thread_switch(S79Threads *r,uint32_t tid){
 for(unsigned i=0;i<r->count;i++)if(r->threads[i].tid==tid){
  S79Thread *t=r->threads+i;if(!t->runnable)return S79_THREAD_WAITING;
  if(!t->public_CPU_known)return S79_THREAD_CPU_UNKNOWN;
  if(!t->cached_segments_proved)return S79_THREAD_CACHE_UNKNOWN;
  r->current=i;return S79_THREAD_OK;
 }
 return S79_THREAD_UNKNOWN;
}
uint32_t s79_thread_id(const S79Threads *r){return r->threads[r->current].tid;}
uint32_t s79_last_error(const S79Threads *r){return r->threads[r->current].last_error;}
void s79_set_error(S79Threads *r,uint32_t value){r->threads[r->current].last_error=value;}
int s79_tls_get(S79Threads *r,uint32_t slot,uint32_t *out){
 S79Thread *t=r->threads+r->current;if(slot>=64||!(t->allocated&(UINT64_C(1)<<slot)))return S79_TLS_UNKNOWN;
 *out=t->tls[slot];t->last_error=0;return S79_THREAD_OK;
}
int s79_tls_set(S79Threads *r,uint32_t slot,uint32_t value){
 S79Thread *t=r->threads+r->current;if(slot>=64||!(t->allocated&(UINT64_C(1)<<slot)))return S79_TLS_UNKNOWN;
 t->tls[slot]=value;return S79_THREAD_OK;
}
