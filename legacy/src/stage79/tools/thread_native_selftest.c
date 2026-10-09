#include "thread_native.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){S79Thread t[2];memset(t,0,sizeof(t));t[0].tid=1;t[1].tid=2;
 for(unsigned i=0;i<2;i++){t[i].allocated=1;t[i].public_CPU_known=t[i].cached_segments_proved=t[i].runnable=1;memset(t[i].opaque,(int)i+1,sizeof(t[i].opaque));}
 S79Threads r={t,2,0};S79Thread before[2];memcpy(before,t,sizeof(t));uint32_t v=0;
 assert(s79_thread_id(&r)==1);s79_set_error(&r,9);assert(s79_last_error(&r)==9);
 assert(!s79_tls_set(&r,0,0xf1234567));assert(s79_last_error(&r)==9);
 assert(!s79_thread_switch(&r,2));assert(!s79_tls_set(&r,0,0x80000000));assert(!s79_tls_get(&r,0,&v)&&v==0x80000000&&s79_last_error(&r)==0);
 assert(!s79_thread_switch(&r,1));assert(s79_last_error(&r)==9);assert(!s79_tls_get(&r,0,&v)&&v==0xf1234567);
 assert(!memcmp(t[0].opaque,before[0].opaque,5608)&&!memcmp(t[1].opaque,before[1].opaque,5608));
 memcpy(before,t,sizeof(t));assert(s79_tls_get(&r,64,&v)==S79_TLS_UNKNOWN);assert(s79_tls_set(&r,1,0)==S79_TLS_UNKNOWN);assert(!memcmp(before,t,sizeof(t)));
 t[1].public_CPU_known=0;assert(s79_thread_switch(&r,2)==S79_THREAD_CPU_UNKNOWN&&r.current==0);t[1].public_CPU_known=1;t[1].cached_segments_proved=0;assert(s79_thread_switch(&r,2)==S79_THREAD_CACHE_UNKNOWN&&r.current==0);t[1].runnable=0;assert(s79_thread_switch(&r,2)==S79_THREAD_WAITING&&r.current==0);
 puts("PASS authored native per-thread LastError/TLS, uint32 pointer values, exact opaque-byte preservation, unknown-CPU/cache/waiting refusal; no original/SR/Unicorn code");return 0;}
