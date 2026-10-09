#ifndef S79_THREAD_NATIVE_H
#define S79_THREAD_NATIVE_H
#include <stdint.h>
typedef struct {uint32_t tid,last_error,tls[64],core[10];uint64_t allocated;
 uint8_t opaque[5608];int public_CPU_known,cached_segments_proved,runnable;} S79Thread;
typedef struct {S79Thread *threads;unsigned count,current;} S79Threads;
enum {S79_THREAD_OK,S79_THREAD_UNKNOWN,S79_THREAD_CPU_UNKNOWN,S79_THREAD_CACHE_UNKNOWN,S79_THREAD_WAITING,S79_TLS_UNKNOWN};
int s79_thread_switch(S79Threads *,uint32_t);
uint32_t s79_thread_id(const S79Threads *);
uint32_t s79_last_error(const S79Threads *);
void s79_set_error(S79Threads *,uint32_t);
int s79_tls_get(S79Threads *,uint32_t,uint32_t *);
int s79_tls_set(S79Threads *,uint32_t,uint32_t);
#endif
