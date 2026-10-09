#ifndef S79_MEMORY_H
#define S79_MEMORY_H
#include <stdint.h>
#include <stddef.h>
typedef struct {uint32_t low,permissions;uint64_t end,offset;} S79Region;
typedef struct {uint8_t *base;S79Region *regions;size_t count;
 uint64_t writes;int poisoned;} S79Memory;
enum {S79_OK,S79_UNMAPPED,S79_PERMISSION,S79_OVERFLOW,S79_INVALID_PLAN,S79_IO};
int s79_memory_open(S79Memory *,const S79Region *,size_t,const char *);
void s79_memory_close(S79Memory *);
int s79_guard(const S79Memory *,uint32_t,uint64_t,unsigned);
int s79_host_to_guest(const S79Memory *,const void *,uint32_t *);
int s79_read(void *,uint32_t,size_t,uint8_t *);
int s79_write(void *,uint32_t,size_t,const uint8_t *);
#endif
