/* Own finite guest-state contract. Original-derived generated code is build-only. */
#ifndef STAGE70_RUNTIME_H
#define STAGE70_RUNTIME_H
#include <stdint.h>
#include <stddef.h>
typedef struct {uint32_t low,size,permissions;uint8_t *bytes;} S70Region;
typedef struct {S70Region *regions;size_t count;uint64_t writes;uint32_t fault_address;} S70Memory;
typedef struct {uint32_t r[8],pc,flags,defined_flags,es_base,ds_base,ss_base;uint64_t budget;} S70CPU;
enum {S70_OK=0,S70_FAULT=1,S70_BUDGET=2,S70_TARGET=3,S70_SEGMENT=4};
uint32_t s70_mask(unsigned bits);
uint32_t s70_reg(S70CPU *c,unsigned reg,unsigned bits,unsigned shift);
void s70_setreg(S70CPU *c,unsigned reg,unsigned bits,unsigned shift,uint32_t value);
int s70_read(S70Memory *m,uint32_t address,unsigned bits,uint32_t *value);
int s70_write(S70Memory *m,uint32_t address,unsigned bits,uint32_t value);
void s70_flags(S70CPU *c,uint32_t a,uint32_t b,uint32_t result,unsigned bits,unsigned op);
int s70_stos(S70CPU *c,S70Memory *m,unsigned bits,int repeat);
int s70_condition(uint32_t flags,unsigned condition);
int s70_city_run(S70CPU *c,S70Memory *m,uint32_t stop_pc,uint32_t stop_sp);
#endif
