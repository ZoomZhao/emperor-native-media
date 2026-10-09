#ifndef STAGE14_MESSAGES_H
#define STAGE14_MESSAGES_H
#include <stddef.h>
#include <stdint.h>
typedef struct s14_messages s14_messages;
s14_messages *s14_messages_new(void);
void s14_messages_delete(s14_messages *);
uint32_t s14_message_register(s14_messages *,const char *,uint32_t *);
size_t s14_messages_size(void);
uint32_t s14_messages_export(s14_messages *,void *,size_t);
uint32_t s14_messages_import(s14_messages *,const void *,size_t);
#endif
