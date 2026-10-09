/* Authored Miles pre-startup preference state, no original DLL code. */
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef struct{pthread_mutex_t mutex;uint32_t prefs[45];uint64_t calls,lock_calls;uint32_t attached,started,debug,dispatcher;} State;
void*s25_new(const uint32_t*defaults,size_t n){if(!defaults||n!=45||defaults[42]!=8||defaults[44]!=1)return NULL;State*s=calloc(1,sizeof(*s));if(!s)return NULL;pthread_mutexattr_t a;if(pthread_mutexattr_init(&a)){free(s);return NULL;}int e=pthread_mutexattr_settype(&a,PTHREAD_MUTEX_RECURSIVE);if(!e)e=pthread_mutex_init(&s->mutex,&a);pthread_mutexattr_destroy(&a);if(e){free(s);return NULL;}memcpy(s->prefs,defaults,sizeof(s->prefs));s->attached=1;return s;}
void s25_delete(void*p){State*s=p;if(s){pthread_mutex_destroy(&s->mutex);free(s);}}
int s25_preference(void*p,uint32_t index,uint32_t value,uint32_t*old){State*s=p;if(!s||!old||index!=42)return 0;int e=pthread_mutex_lock(&s->mutex);if(e)return 0;if(!s->attached||s->started||s->debug||s->dispatcher||s->prefs[44]!=1){pthread_mutex_unlock(&s->mutex);return 0;}*old=s->prefs[index];s->prefs[index]=value;s->calls++;s->lock_calls++;return pthread_mutex_unlock(&s->mutex)==0;}
size_t s25_size(void){return 256;}
int s25_export(void*p,void*out,size_t n){State*s=p;if(!s||!out||n!=256||pthread_mutex_lock(&s->mutex))return 0;uint32_t a[64]={0};a[0]=0x5332354d;a[1]=1;memcpy(a+2,s->prefs,sizeof(s->prefs));a[47]=s->attached;a[48]=s->started;a[49]=s->debug;a[50]=s->dispatcher;a[51]=(uint32_t)s->calls;a[52]=(uint32_t)(s->calls>>32);a[53]=(uint32_t)s->lock_calls;a[54]=(uint32_t)(s->lock_calls>>32);memcpy(out,a,256);return pthread_mutex_unlock(&s->mutex)==0;}
int s25_import(void*p,const void*in,size_t n){State*s=p;if(!s||!in||n!=256)return 0;uint32_t a[64];memcpy(a,in,256);if(a[0]!=0x5332354d||a[1]!=1||a[46]!=1||a[47]!=1||a[48]||a[49]||a[50])return 0;for(int i=55;i<64;i++)if(a[i])return 0;if(pthread_mutex_lock(&s->mutex))return 0;memcpy(s->prefs,a+2,sizeof(s->prefs));s->attached=a[47];s->started=a[48];s->debug=a[49];s->dispatcher=a[50];s->calls=a[51]|((uint64_t)a[52]<<32);s->lock_calls=a[53]|((uint64_t)a[54]<<32);return pthread_mutex_unlock(&s->mutex)==0;}
