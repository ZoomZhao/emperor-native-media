#ifndef STAGE72_RUNTIME_H
#define STAGE72_RUNTIME_H
#include <stdint.h>
#include <stddef.h>
#include "llasm_cpu.h"
typedef struct {uint32_t low,size,permissions;} S72Region;
typedef struct {uint32_t pc,address,size,value;} S72Write;
typedef struct {uint32_t pc;void *target;} S72Return;
typedef struct {uint64_t steps,writes;uint32_t pc,status,fault_address;} S72Result;
extern S72Return stage72_return_bindings[];
extern uint32_t stage72_return_bindings_count;
int stage72_check_regions(const S72Region*,size_t);
int stage72_check_access(const S72Region*,size_t,uint32_t,unsigned,unsigned);
int stage72_check_returns(const S72Return*,size_t,uint64_t);
int stage72_configure(_cpu*,const S72Region*,size_t,uint64_t,uint64_t,uint64_t);
void stage72_step(uint32_t);
void stage72_guard(void*,uint32_t,uint32_t);
void stage72_note_write(void*,uint32_t,uint64_t);
void stage72_validate_rep(_cpu*,uint32_t);
void *stage72_lookup_return(uint32_t);
void stage72_stop_c(_cpu*);
int stage72_run(void);
const S72Result *stage72_result(void);
const S72Write *stage72_writes(void);
void stage72_dispose(void);
#endif
