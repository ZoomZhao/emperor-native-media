#ifndef STAGE13_EVENTS_H
#define STAGE13_EVENTS_H
#include <stddef.h>
#include <stdint.h>
typedef struct s13_events s13_events;
#define S13_REFUSED UINT32_C(0xfffffffe)
#define S13_EVENT_CAPACITY 128u
s13_events *s13_events_new(void);
uint32_t s13_events_delete(s13_events *state);
uint32_t s13_event_create(s13_events *,uint32_t manual,uint32_t initial,uint32_t *last_error);
uint32_t s13_event_set(s13_events *,uint32_t handle,uint32_t *last_error);
uint32_t s13_event_reset(s13_events *,uint32_t handle,uint32_t *last_error);
uint32_t s13_event_close(s13_events *,uint32_t handle,uint32_t *last_error);
uint32_t s13_event_wait(s13_events *,uint32_t handle,uint32_t milliseconds,uint32_t *last_error);
uint32_t s13_event_query(s13_events *,uint32_t handle,uint32_t *manual,uint32_t *signaled,uint32_t *waiters);
size_t s13_events_snapshot_size(void);
uint32_t s13_events_export(s13_events *,void *buffer,size_t capacity);
uint32_t s13_events_import(s13_events *,const void *buffer,size_t size);
#endif
