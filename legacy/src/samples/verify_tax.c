/* State/ABI comparison scaffold. No Swift code and no gameplay implementation. */
#include <stdio.h>
#include <stdlib.h>
#include "native_cpu.h"
extern uint32_t tax_rates[7],tax_matrix[35],tax_descriptor[4];
extern uint32_t tax_current,tax_difficulty,tax_metric,tax_coverage;
extern uint8_t tax_guard,rates_segment[],diff_segment[],taxdata_segment[],city_segment[];
extern void c_tax_contribution(CPU *),c_nearest_rate(CPU *);
extern void (*ptr_initialize_pointers)(uint64_t);
static const int32_t shipping_matrix[]={3,2,1,0,0,2,1,0,0,-1,1,0,0,-1,-2,0,0,-1,-2,-3,0,-1,-2,-3,-4,-1,-2,-3,-4,-5,-2,-3,-4,-5,-6};
static uint8_t before[7822];
static void snapshot(uint8_t *dest) {
    memcpy(dest,rates_segment,3240);memcpy(dest+3240,diff_segment,740);
    memcpy(dest+3980,taxdata_segment,2444);memcpy(dest+6424,city_segment,1398);
}
static uint32_t initial_reg(uint32_t seed,unsigned i) {return (seed*0x9e3779b9u)^(i*0x13579bdfu)^0xa1b2c3d4u;}
static uint32_t *reg(CPU *c,unsigned i) {
    switch(i) {case 0:return &c->eax;case 1:return &c->ecx;case 2:return &c->edx;case 3:return &c->ebx;case 4:return &c->esp;case 5:return &c->ebp;case 6:return &c->esi;default:return &c->edi;}
}
int main(int argc,char **argv) {
    if(argc!=2)return 2;FILE *f=fopen(argv[1],"rb");if(!f)return 3;
    uint32_t header[4],r[36];if(fread(header,4,4,f)!=4||header[0]!=0x54415832||header[2]!=36)return 4;
    CPU c;if(!initialize_cpu(&c,&tax_current)||!ptr_initialize_pointers)return 5;
    ptr_initialize_pointers(c.pointer_offset);
    if(tax_descriptor[0]!=pointer32(&c,tax_rates)||tax_descriptor[3]!=pointer32(&c,&tax_current))return 6;
    unsigned regfail=0,flagfail=0,stackfail=0,memfail=0,examples=0;uint32_t digest=2166136261u;
    for(unsigned n=0;n<header[1];n++) {
        if(fread(r,4,36,f)!=36)return 7;
        uint32_t variant=r[8];
        for(unsigned i=0;i<7;i++)tax_rates[i]=variant==1?(uint32_t[]){0,4,4,8,12,16,20}[i]:variant==2?(uint32_t[]){0x80000000,0xffffffff,0,1,4,0x7ffffffe,0x7fffffff}[i]:i*3;
        for(unsigned i=0;i<35;i++)tax_matrix[i]=variant==2?(i%3==0?0x80000000:i%3==1?0x7fffffff:0xffffffff):(uint32_t)shipping_matrix[i];
        tax_current=r[2];tax_difficulty=r[3];tax_coverage=r[4];tax_metric=r[5];tax_guard=(uint8_t)r[6];tax_descriptor[1]=r[7];
        snapshot(before);
        memset(native_guest_stack,0xa5,sizeof(native_guest_stack));
        initialize_cpu(&c,&tax_current);for(unsigned i=0;i<8;i++)if(i!=4)*reg(&c,i)=initial_reg(r[1],i);
        c.eflags=r[9];uint32_t esp=c.esp;
        for(unsigned round=0;round<2;round++) {
            if(r[0]) {c.ecx=pointer32(&c,tax_descriptor);c_nearest_rate(&c);}else c_tax_contribution(&c);
            unsigned badregs=0,badstack=0,badmem=0,badflags=0;
            for(unsigned i=0;i<8;i++)if(*reg(&c,i)!=(i==4?esp:r[10+i]))badregs|=1u<<i;
            badflags=((c.eflags^r[18])&r[19])!=0 || ((c.eflags^r[9])&~0x8d5u)!=0;
            uint32_t actual_stack[16];memcpy(actual_stack,(const void *)(uintptr_t)(c.pointer_offset+esp-64),64);
            for(unsigned i=0;i<16;i++) {
                /* SR represents return continuations by native code pointers. */
                if(i==15||(!r[0]&&i==14)) {if(actual_stack[i]==0||actual_stack[i]==0xa5a5a5a5)badstack++;continue;}
                if(actual_stack[i]!=r[20+i])badstack++;
            }
            uint8_t after[sizeof(before)];snapshot(after);badmem=memcmp(before,after,sizeof(before))!=0;
            regfail+=badregs!=0;flagfail+=badflags;stackfail+=badstack!=0;memfail+=badmem;
            digest=(digest^c.eax)*16777619u;
            if((badregs||badflags||badstack||badmem)&&examples++<8)
                fprintf(stderr,"case=%u round=%u kind=%u tax=%d diff=%u coverage=%d metric=%d guard=%u variant=%u regs_mask=%x flags=%x expected_flags=%x defined_mask=%x stack_bad=%u memory_bad=%u eax=%x expected_eax=%x ecx=%x expected_ecx=%x edx=%x expected_edx=%x\n",n,round,r[0],(int32_t)r[2],r[3],(int32_t)r[4],(int32_t)r[5],r[6],variant,badregs,c.eflags,r[18],r[19],badstack,badmem,c.eax,r[10],c.ecx,r[11],c.edx,r[12]);
        }
    }
    if(fgetc(f)!=EOF)return 8;fclose(f);
    printf("{\"kind\":\"Emperor_tax_152bytes_arm64\",\"cases\":%u,\"calls\":%u,\"register_failures\":%u,\"defined_flag_failures\":%u,\"stack_failures\":%u,\"memory_failures\":%u,\"data_bytes_compared_per_call\":7822,\"stack_bytes_compared\":64,\"opaque_return_slots_per_tax_call\":2,\"repeat_without_state_reset\":true,\"digest\":%u,\"passed\":%s}\n",header[1],header[1]*2,regfail,flagfail,stackfail,memfail,digest,(regfail||flagfail||stackfail||memfail)?"false":"true");
    return (regfail||flagfail||stackfail||memfail)?1:0;
}
