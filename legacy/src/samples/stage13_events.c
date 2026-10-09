/* Task-authored C ABI event lifecycle. No game source or original code here.
 * LastError is supplied by each caller's thread domain. Handles are opaque
 * 32-bit IDs, never host pointers. Core waits use actual pthread conditions.
 * Research adapter refuses unsupported guest scheduling before such waits.
 */
#include "stage13_events.h"
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {uint32_t handle,live,manual,signaled;} event_record;
struct s13_events {
    pthread_mutex_t lock;
    pthread_cond_t condition[S13_EVENT_CAPACITY];
    event_record records[S13_EVENT_CAPACITY];
    uint32_t waiters[S13_EVENT_CAPACITY],active_waiters,next_serial;
};
enum {MAGIC=0x45563133,VERSION=1,HEADER_WORDS=4};
static const uint32_t HANDLE_BASE=0x51000000;
static int lookup(s13_events *s,uint32_t h) {
    for(unsigned i=0;i<S13_EVENT_CAPACITY;i++)if(s->records[i].live&&s->records[i].handle==h)return (int)i;
    return -1;
}
static uint64_t now_ns(void) {struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))abort();return (uint64_t)t.tv_sec*1000000000+(uint64_t)t.tv_nsec;}
s13_events *s13_events_new(void) {
    s13_events *s=calloc(1,sizeof(*s));if(!s)return NULL;
    if(pthread_mutex_init(&s->lock,NULL)) {free(s);return NULL;}
    for(unsigned i=0;i<S13_EVENT_CAPACITY;i++)if(pthread_cond_init(&s->condition[i],NULL))abort();
    s->next_serial=1;return s;
}
uint32_t s13_events_delete(s13_events *s) {
    if(!s)return 0;pthread_mutex_lock(&s->lock);
    if(s->active_waiters) {pthread_mutex_unlock(&s->lock);return S13_REFUSED;}
    pthread_mutex_unlock(&s->lock);
    for(unsigned i=0;i<S13_EVENT_CAPACITY;i++)pthread_cond_destroy(&s->condition[i]);
    pthread_mutex_destroy(&s->lock);free(s);return 1;
}
uint32_t s13_event_create(s13_events *s,uint32_t manual,uint32_t initial,uint32_t *error) {
    pthread_mutex_lock(&s->lock);unsigned slot=0;
    while(slot<S13_EVENT_CAPACITY&&s->records[slot].live)slot++;
    if(slot==S13_EVENT_CAPACITY||s->next_serial>=0xffffff) {pthread_mutex_unlock(&s->lock);return S13_REFUSED;}
    uint32_t h=HANDLE_BASE|s->next_serial++;s->records[slot]=(event_record){h,1,manual!=0,initial!=0};
    *error=0;pthread_mutex_unlock(&s->lock);return h;
}
static uint32_t mutate(s13_events *s,uint32_t h,uint32_t *error,int signaled) {
    pthread_mutex_lock(&s->lock);int i=lookup(s,h);
    if(i<0) {*error=6;pthread_mutex_unlock(&s->lock);return 0;}
    event_record *e=&s->records[i];int changed=!e->signaled&&signaled;e->signaled=(uint32_t)signaled;
    if(changed) {if(e->manual)pthread_cond_broadcast(&s->condition[i]);else pthread_cond_signal(&s->condition[i]);}
    pthread_mutex_unlock(&s->lock);return 1;
}
uint32_t s13_event_set(s13_events *s,uint32_t h,uint32_t *error) {return mutate(s,h,error,1);}
uint32_t s13_event_reset(s13_events *s,uint32_t h,uint32_t *error) {return mutate(s,h,error,0);}
uint32_t s13_event_close(s13_events *s,uint32_t h,uint32_t *error) {
    pthread_mutex_lock(&s->lock);int i=lookup(s,h);
    if(i<0) {*error=6;pthread_mutex_unlock(&s->lock);return 0;}
    if(s->waiters[i]) {pthread_mutex_unlock(&s->lock);return S13_REFUSED;} // SDK undefined when closed during a pending wait.
    s->records[i].live=0;pthread_mutex_unlock(&s->lock);return 1;
}
uint32_t s13_event_query(s13_events *s,uint32_t h,uint32_t *manual,uint32_t *signaled,uint32_t *waiters) {
    pthread_mutex_lock(&s->lock);int i=lookup(s,h);
    if(i<0) {pthread_mutex_unlock(&s->lock);return 0;}
    *manual=s->records[i].manual;*signaled=s->records[i].signaled;*waiters=s->waiters[i];pthread_mutex_unlock(&s->lock);return 1;
}
uint32_t s13_event_wait(s13_events *s,uint32_t h,uint32_t milliseconds,uint32_t *error) {
    pthread_mutex_lock(&s->lock);int i=lookup(s,h);
    if(i<0) {*error=6;pthread_mutex_unlock(&s->lock);return UINT32_MAX;}
    event_record *e=&s->records[i];uint32_t answer=0;
    if(!e->signaled&&!milliseconds) {pthread_mutex_unlock(&s->lock);return 258;}
    uint64_t deadline=milliseconds==UINT32_MAX?UINT64_MAX:now_ns()+(uint64_t)milliseconds*1000000;
    s->active_waiters++;s->waiters[i]++;
    while(!e->signaled) {
        int status;
        if(milliseconds==UINT32_MAX)status=pthread_cond_wait(&s->condition[i],&s->lock);
        else {
            uint64_t now=now_ns();if(now>=deadline) {answer=258;break;}
            uint64_t remaining=deadline-now;struct timespec relative={(time_t)(remaining/1000000000),(long)(remaining%1000000000)};
            status=pthread_cond_timedwait_relative_np(&s->condition[i],&s->lock,&relative);
        }
        if(status&&status!=ETIMEDOUT) {answer=S13_REFUSED;break;}
    }
    if(answer==0&&!e->manual)e->signaled=0;
    s->waiters[i]--;s->active_waiters--;pthread_mutex_unlock(&s->lock);return answer;
}
size_t s13_events_snapshot_size(void) {return (HEADER_WORDS+S13_EVENT_CAPACITY*4u)*sizeof(uint32_t);}
uint32_t s13_events_export(s13_events *s,void *buffer,size_t capacity) {
    if(capacity!=s13_events_snapshot_size())return 0;pthread_mutex_lock(&s->lock);
    if(s->active_waiters) {pthread_mutex_unlock(&s->lock);return S13_REFUSED;}
    uint32_t header[HEADER_WORDS]={MAGIC,VERSION,S13_EVENT_CAPACITY,s->next_serial};
    memcpy(buffer,header,sizeof(header));memcpy((char*)buffer+sizeof(header),s->records,sizeof(s->records));pthread_mutex_unlock(&s->lock);return 1;
}
uint32_t s13_events_import(s13_events *s,const void *buffer,size_t size) {
    if(size!=s13_events_snapshot_size())return 0;uint32_t header[HEADER_WORDS];event_record records[S13_EVENT_CAPACITY];
    memcpy(header,buffer,sizeof(header));memcpy(records,(const char*)buffer+sizeof(header),sizeof(records));
    if(header[0]!=MAGIC||header[1]!=VERSION||header[2]!=S13_EVENT_CAPACITY||!header[3]||header[3]>=0xffffff)return 0;
    for(unsigned i=0;i<S13_EVENT_CAPACITY;i++) {
        event_record e=records[i];if(e.live>1||e.manual>1||e.signaled>1)return 0;
        if(e.handle&&((e.handle&0xff000000)!=HANDLE_BASE||(e.handle&0xffffff)==0||(e.handle&0xffffff)>=header[3]))return 0;
        if(e.live&&!e.handle)return 0;
        for(unsigned j=0;j<i;j++)if(e.handle&&e.handle==records[j].handle)return 0;
    }
    pthread_mutex_lock(&s->lock);if(s->active_waiters) {pthread_mutex_unlock(&s->lock);return S13_REFUSED;}
    memcpy(s->records,records,sizeof(records));s->next_serial=header[3];memset(s->waiters,0,sizeof(s->waiters));pthread_mutex_unlock(&s->lock);return 1;
}
