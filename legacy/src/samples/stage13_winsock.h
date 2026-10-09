#ifndef STAGE13_WINSOCK_H
#define STAGE13_WINSOCK_H
#include <stddef.h>
#include <stdint.h>
uint32_t s13_wsa_startup(uint32_t *references,uint32_t version,void *output,size_t size,uint32_t *last_error);
uint32_t s13_wsa_cleanup(uint32_t *references,uint32_t *last_error);
#endif
