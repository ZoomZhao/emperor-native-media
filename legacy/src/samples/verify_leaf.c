#include <stdio.h>
#include "native_cpu.h"
extern uint8_t emperor_flag;
extern void c_emperor_leaf(CPU *);
int main(void) {
    unsigned failures=0,flag_failures=0;
    for(unsigned b=0;b<256;b++) {
        CPU c;
        if(!initialize_cpu(&c,&emperor_flag)) return 2;
        emperor_flag=(uint8_t)b;
        c.eax=0xabcdefed;c.ecx=0x23456788;c.edx=0x76543210;
        c.ebx=0x11223344;c.ebp=0x55667788;c.esi=0x99aabbcc;c.edi=0xddeeff00;
        uint32_t esp=c.esp;
        c_emperor_leaf(&c);
        unsigned r=(b&1)?1:(b|1),ones=0;
        for(unsigned v=r;v;v>>=1)ones+=v&1;
        uint32_t expected_flags=((ones%2==0)?4:0)|((r&128)?128:0);
        if((c.eflags&0x8c5)!=expected_flags)flag_failures++;
        int ok=emperor_flag==(b|1) && c.eax==0xabcdef01 && c.ecx==(0x23456700|(b|1))
          && c.edx==0x76543210 && c.ebx==0x11223344 && c.ebp==0x55667788
          && c.esi==0x99aabbcc && c.edi==0xddeeff00 && c.esp==esp;
        if(!ok) { failures++;fprintf(stderr,"initial=%u flag=%u eax=%x ecx=%x esp=%x expected_esp=%x\n",b,emperor_flag,c.eax,c.ecx,c.esp,esp); }
    }
    printf("{\"kind\":\"translated_Emperor_21byte_leaf_arm64\",\"cases\":256,\"memory_register_failures\":%u,\"defined_flag_failures\":%u,\"passed\":%s,\"flags_checked\":true,\"undefined_AF_ignored\":true,\"gameplay_proven\":false}\n",failures,flag_failures,(failures||flag_failures)?"false":"true");
    return (failures||flag_failures)?1:0;
}
