/* Data-driven native comparison against independent Unicorn output. */
#include <stdio.h>
#include "native_cpu.h"
extern uint8_t emperor_flag;
extern void c_emperor_leaf(CPU *);
static uint32_t *reg(CPU *c,unsigned i) {switch(i){case 0:return &c->eax;case 1:return &c->ecx;case 2:return &c->edx;case 3:return &c->ebx;case 4:return &c->esp;case 5:return &c->ebp;case 6:return &c->esi;default:return &c->edi;}}
int main(int argc,char **argv) {
    if(argc!=2)return 2;FILE *f=fopen(argv[1],"rb");if(!f)return 3;uint32_t h[4],r[28];if(fread(h,4,4,f)!=4||h[0]!=0x4c454146||h[2]!=28)return 4;
    unsigned regs=0,flags=0,memory=0,stack=0;
    for(unsigned i=0;i<h[1];i++) {
        if(fread(r,4,28,f)!=28)return 5;CPU c;if(!initialize_cpu(&c,&emperor_flag))return 6;memset(native_guest_stack,0xa5,sizeof(native_guest_stack));
        c.eax=0xabcdefed;c.ecx=0x23456788;c.edx=0x76543210;c.ebx=0x11223344;c.ebp=0x55667788;c.esi=0x99aabbcc;c.edi=0xddeeff00;uint32_t esp=c.esp;emperor_flag=r[0];c_emperor_leaf(&c);
        unsigned bad=0;for(unsigned j=0;j<8;j++)if(*reg(&c,j)!=(j==4?esp:r[j+1]))bad|=1u<<j;
        regs+=bad!=0;flags+=((c.eflags^r[9])&r[10])!=0||((c.eflags^r[9])&~0x8d5u)!=0;memory+=emperor_flag!=r[11];
        uint32_t actual[16];memcpy(actual,(const void *)(uintptr_t)(c.pointer_offset+esp-64),64);unsigned bs=0;
        for(unsigned j=0;j<16;j++)if(j==15){if(actual[j]==0||actual[j]==0xa5a5a5a5)bs++;}else if(actual[j]!=r[12+j])bs++;stack+=bs!=0;
        if(bad)fprintf(stderr,"case=%u badregs=%x\n",i,bad);
    }
    fclose(f);printf("{\"cases\":%u,\"register_failures\":%u,\"defined_flag_failures\":%u,\"memory_failures\":%u,\"stack_failures\":%u,\"opaque_return_slots\":1,\"passed\":%s}\n",h[1],regs,flags,memory,stack,(regs||flags||memory||stack)?"false":"true");return (regs||flags||memory||stack)?1:0;
}
