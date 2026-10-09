#include <stdio.h>
#include <stdlib.h>
#include "native_cpu.h"
extern uint32_t tax_rates[7],tax_matrix[35],tax_descriptor[4],tax_current,tax_difficulty,tax_coverage,tax_metric;
extern uint8_t tax_guard,rates_segment[],diff_segment[],taxdata_segment[],city_segment[];
extern void c_tax_raise(CPU *);
extern void (*ptr_initialize_pointers)(uint64_t);
static uint8_t before[7822];
static void snapshot(uint8_t *d) {memcpy(d,rates_segment,3240);memcpy(d+3240,diff_segment,740);memcpy(d+3980,taxdata_segment,2444);memcpy(d+6424,city_segment,1398);memset(d+3980+0x96c,0,4);}
static uint32_t *reg(CPU *c,unsigned i) {switch(i){case 0:return &c->eax;case 1:return &c->ecx;case 2:return &c->edx;case 3:return &c->ebx;case 4:return &c->esp;case 5:return &c->ebp;case 6:return &c->esi;default:return &c->edi;}}
int main(int argc,char **argv) {
    if(argc!=2)return 2;FILE *f=fopen(argv[1],"rb");if(!f)return 3;
    uint32_t h[4],r[33];if(fread(h,4,4,f)!=4||h[0]!=0x57525433||h[2]!=33)return 4;
    CPU c;if(!initialize_cpu(&c,&tax_current)||!ptr_initialize_pointers)return 5;ptr_initialize_pointers(c.pointer_offset);
    if(tax_descriptor[0]!=pointer32(&c,tax_rates)||tax_descriptor[3]!=pointer32(&c,&tax_current))return 6;
    uint32_t shipping_matrix[35];memcpy(shipping_matrix,tax_matrix,sizeof(shipping_matrix));
    unsigned regs=0,flags=0,stack=0,memory=0,writes=0,changed=0,examples=0;uint32_t esp=0;
    for(unsigned n=0;n<h[1];n++) {
        if(fread(r,4,33,f)!=33)return 7;
        if(r[0]) {
            initialize_cpu(&c,&tax_current);esp=c.esp;memset(native_guest_stack,0xa5,sizeof(native_guest_stack));
            for(unsigned i=0;i<8;i++)if(i!=4)*reg(&c,i)=(r[1]*0x9e3779b9u)^(i*0x13579bdfu)^0xa1b2c3d4u;
            c.eflags=r[4];tax_current=r[3];tax_descriptor[1]=7;
            for(unsigned i=0;i<7;i++)tax_rates[i]=r[2]==1?(uint32_t[]){0,4,4,8,12,16,20}[i]:r[2]==2?(uint32_t[]){0x80000000,0xffffffff,0,1,4,0x7ffffffe,0x7fffffff}[i]:i*3;
            /* Same complete mapped inputs as ISA/Unicorn; writer does not read these fields. */
            for(unsigned i=0;i<35;i++)tax_matrix[i]=r[2]==2?(i%3==0?0x80000000:i%3==1?0x7fffffff:0xffffffff):shipping_matrix[i];
            tax_difficulty=0;tax_coverage=11;tax_metric=350;tax_guard=1;
        }
        if(tax_current!=r[3])return 8;snapshot(before);c_tax_raise(&c);
        unsigned br=0,bf=0,bs=0,bm=0;
        for(unsigned i=0;i<8;i++) {
            uint32_t expected=i==4?esp:r[5+i];
            if(r[16]&&i==1)expected=pointer32(&c,tax_rates);
            if(r[16]&&i==2)expected=pointer32(&c,&tax_current);
            if(*reg(&c,i)!=expected)br|=1u<<i;
        }
        bf=((c.eflags^r[13])&r[14])!=0||((c.eflags^r[4])&~0x8d5u)!=0;
        uint32_t actual[16];memcpy(actual,(const void *)(uintptr_t)(c.pointer_offset+esp-64),64);
        for(unsigned i=0;i<16;i++) {
            if(i==13||i==15){if(!actual[i]||actual[i]==0xa5a5a5a5)bs++;continue;}
            uint32_t expected=i==9?pointer32(&c,tax_descriptor):r[17+i];if(actual[i]!=expected)bs++;
        }
        uint8_t after[sizeof(before)];snapshot(after);bm=memcmp(before,after,sizeof(before))!=0||tax_current!=r[15];writes+=r[16];changed+=tax_current!=r[3];
        regs+=br!=0;flags+=bf;stack+=bs!=0;memory+=bm;
        if((br||bf||bs||bm)&&examples++<6)fprintf(stderr,"call=%u regs=%x flags=%x expected=%x mask=%x stack=%u memory=%u current=%x expected_current=%x\n",n,br,c.eflags,r[13],r[14],bs,bm,tax_current,r[15]);
    }
    fclose(f);printf("{\"calls\":%u,\"state_sequences\":%u,\"consecutive_calls\":8,\"expected_write_paths\":%u,\"observed_state_changes\":%u,\"register_failures\":%u,\"defined_flag_failures\":%u,\"stack_failures\":%u,\"memory_failures\":%u,\"passed\":%s}\n",h[1],h[1]/8,writes,changed,regs,flags,stack,memory,(regs||flags||stack||memory)?"false":"true");return (regs||flags||stack||memory)?1:0;
}
