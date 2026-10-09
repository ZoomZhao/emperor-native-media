/* Real host pthread tests of the self-authored event core, no game code. */
#include "stage13_events.h"
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
typedef struct {s13_events *s;uint32_t handle,timeout,result,error;} job;
static void *waiter(void *p) {job *j=p;j->error=0x13579;j->result=s13_event_wait(j->s,j->handle,j->timeout,&j->error);return NULL;}
static uint64_t now(void) {struct timespec t;assert(!clock_gettime(CLOCK_MONOTONIC,&t));return (uint64_t)t.tv_sec*1000000000+t.tv_nsec;}
static void until_waiters(s13_events *s,uint32_t h,unsigned expected) {
    uint64_t end=now()+1000000000;uint32_t manual,signaled,count;
    while(1) {assert(s13_event_query(s,h,&manual,&signaled,&count));if(count==expected)return;assert(now()<end);struct timespec pause={0,1000000};nanosleep(&pause,NULL);}
}
int main(void) {
    s13_events *s=s13_events_new();assert(s);uint32_t error=77;unsigned groups=0;
    for(unsigned manual=0;manual<2;manual++) {
        uint32_t h=s13_event_create(s,manual,0,&error);assert(error==0);pthread_t t[2];job jobs[2]={{s,h,150,0,0},{s,h,150,0,0}};
        for(unsigned i=0;i<2;i++)assert(!pthread_create(&t[i],NULL,waiter,&jobs[i]));until_waiters(s,h,2);
        size_t n=s13_events_snapshot_size();unsigned char *blob=malloc(n);assert(blob);
        assert(s13_events_export(s,blob,n)==S13_REFUSED);error=77;assert(s13_event_close(s,h,&error)==S13_REFUSED&&error==77);
        assert(s13_event_set(s,h,&error)==1&&error==77);
        for(unsigned i=0;i<2;i++)assert(!pthread_join(t[i],NULL));
        unsigned success=(jobs[0].result==0)+(jobs[1].result==0);assert(success==(manual?2u:1u));
        for(unsigned i=0;i<2;i++)assert((jobs[i].result==0||jobs[i].result==258)&&jobs[i].error==0x13579);
        assert(s13_events_export(s,blob,n)==1);free(blob);assert(s13_event_close(s,h,&error)==1);groups++;
    }
    // Infinite wait actually blocks until another native thread sets the event.
    uint32_t h=s13_event_create(s,0,0,&error);job infinite={s,h,UINT32_MAX,0,0};pthread_t thread;
    assert(!pthread_create(&thread,NULL,waiter,&infinite));until_waiters(s,h,1);assert(s13_event_set(s,h,&error)==1);assert(!pthread_join(thread,NULL));
    assert(infinite.result==0&&infinite.error==0x13579);assert(s13_event_close(s,h,&error)==1);groups++;
    // Different events must not wake each other's waiters.
    uint32_t a=s13_event_create(s,0,0,&error),b=s13_event_create(s,0,0,&error);job ja={s,a,100,0,0},jb={s,b,100,0,0};pthread_t ta,tb;
    assert(!pthread_create(&ta,NULL,waiter,&ja));assert(!pthread_create(&tb,NULL,waiter,&jb));until_waiters(s,a,1);until_waiters(s,b,1);
    assert(s13_event_set(s,b,&error)==1);assert(!pthread_join(ta,NULL));assert(!pthread_join(tb,NULL));assert(ja.result==258&&jb.result==0);groups++;
    assert(s13_event_close(s,a,&error)==1&&s13_event_close(s,b,&error)==1);assert(s13_events_delete(s)==1);
    printf("{\"all_passed\":true,\"native_pthread_groups\":%u,\"pending_wait_snapshot_refused\":true,\"pending_wait_close_refused\":true,\"infinite_wait_released_by_actual_signal\":true,\"guest_threads_implemented\":false}\n",groups);return 0;
}
