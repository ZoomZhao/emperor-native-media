/* Independently check interpreter arithmetic on installed Apple x86_64/Rosetta.
   Original i386 routines are NOT executed here. */
#include <stdint.h>
#include <stdio.h>
#define DO32(ins) __asm__ volatile("pushq %[inflags]; popfq; " ins " %[b],%[value]; pushfq; popq %[outflags]" :[value]"+r"(value),[outflags]"=&r"(flags):[b]"r"(b),[inflags]"r"(input_flags):"cc")
int main(int argc,char **argv) {
    if(argc!=2)return 2;FILE *f=fopen(argv[1],"rb");if(!f)return 3;
    uint32_t r[7];unsigned cases=0,failures=0;
    while(fread(r,4,7,f)==7) {
        uint32_t value=r[1],b=r[2];uint64_t input_flags=r[3],flags=0;
        switch(r[0]) {
            case 0:DO32("addl");break;case 1:DO32("subl");break;case 2:DO32("xorl");break;case 3:DO32("orl");break;
            case 4:DO32("testl");value=r[1]&r[2];break;
            case 5:__asm__ volatile("pushq %[inflags]; popfq; incl %[value]; pushfq; popq %[outflags]" :[value]"+r"(value),[outflags]"=&r"(flags):[inflags]"r"(input_flags):"cc");break;
            case 6: {uint8_t a=(uint8_t)value,d=(uint8_t)b;__asm__ volatile("pushq %[inflags]; popfq; testb %[b],%[a]; pushfq; popq %[outflags]" :[outflags]"=&r"(flags):[a]"q"(a),[b]"q"(d),[inflags]"r"(input_flags):"cc");value=a&d;break;}
            default:return 4;
        }
        cases++;if(value!=r[4]||((uint32_t)flags^r[5])&r[6]) {
            if(failures++<5)fprintf(stderr,"case=%u opcode=%u a=%x b=%x result=%x expected=%x flags=%llx expected=%x mask=%x\n",cases,r[0],r[1],r[2],value,r[4],(unsigned long long)flags,r[5],r[6]);
        }
    }
    fclose(f);printf("{\"cases\":%u,\"failures\":%u,\"oracle\":\"Apple x86_64 execution via installed Rosetta; 32-bit ALU operands\",\"original_module_executed\":false}\n",cases,failures);return failures?1:0;
}
