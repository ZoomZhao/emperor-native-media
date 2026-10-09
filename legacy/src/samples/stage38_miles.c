/* Own observed startup/preference state. Not a provider or mixer replacement. */
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef struct {pthread_mutex_t mutex;uint32_t defaults[45],prefs[45],started;uint64_t calls,lock_calls;uint32_t startups;} State;
void*s38_new(const uint32_t*d,size_t n){if(!d||n!=45||d[42]!=8||d[44]!=1)return NULL;State*s=calloc(1,sizeof(*s));if(!s)return NULL;if(pthread_mutex_init(&s->mutex,NULL)){free(s);return NULL;}memcpy(s->defaults,d,180);memcpy(s->prefs,d,180);return s;}
void s38_delete(void*p){State*s=p;if(s){pthread_mutex_destroy(&s->mutex);free(s);}}
int s38_startup(void*p,uint32_t*out){State*s=p;if(!s||!out||pthread_mutex_lock(&s->mutex))return 0;*out=s->started?0:1;if(!s->started){memcpy(s->prefs,s->defaults,180);s->started=1;s->startups++;}s->calls++;s->lock_calls++;return pthread_mutex_unlock(&s->mutex)==0;}
int s38_preference(void*p,uint32_t i,uint32_t v,uint32_t*out){State*s=p;if(!s||!out||(i!=42&&i!=40&&i!=15)||pthread_mutex_lock(&s->mutex))return 0;*out=s->prefs[i];s->prefs[i]=v;s->calls++;s->lock_calls++;return pthread_mutex_unlock(&s->mutex)==0;}
int s38_get(void*p,uint32_t i,uint32_t*out){State*s=p;if(!s||!out||i!=42||pthread_mutex_lock(&s->mutex))return 0;*out=s->prefs[i];return pthread_mutex_unlock(&s->mutex)==0;}
int s38_export(void*p,void*out,size_t n){State*s=p;if(!s||!out||n!=256||pthread_mutex_lock(&s->mutex))return 0;uint32_t a[64]={0};a[0]=0x5333384d;a[1]=1;memcpy(a+2,s->prefs,180);a[47]=1;a[48]=s->started;a[51]=(uint32_t)s->calls;a[52]=(uint32_t)(s->calls>>32);a[53]=(uint32_t)s->lock_calls;a[54]=(uint32_t)(s->lock_calls>>32);a[55]=s->startups;memcpy(out,a,256);return pthread_mutex_unlock(&s->mutex)==0;}
int s38_import(void*p,const void*in,size_t n){State*s=p;if(!s||!in||n!=256)return 0;uint32_t a[64];memcpy(a,in,256);if((a[0]!=0x5332354d&&a[0]!=0x5333384d)||a[1]!=1||a[46]!=1||a[47]!=1||a[48]>1||a[49]||a[50])return 0;if(a[0]==0x5332354d&&(a[48]||a[55]))return 0;for(int i=56;i<64;i++)if(a[i])return 0;if(pthread_mutex_lock(&s->mutex))return 0;memcpy(s->prefs,a+2,180);s->started=a[48];s->calls=a[51]|((uint64_t)a[52]<<32);s->lock_calls=a[53]|((uint64_t)a[54]<<32);s->startups=a[55];return pthread_mutex_unlock(&s->mutex)==0;}
