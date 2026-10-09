/* Native SR CPU-stack / C API integration; independent Wine byte oracle is
 * evaluated by verify_stage10_native.py, not by this implementation. */
#include "native_cpu.h"
#include "stage10_native_platform.h"
#include <stdio.h>
#include <stdlib.h>

extern void c_stage10_GetACP(CPU *),c_stage10_GetVersion(CPU *);
extern void c_stage10_GetCPInfo(CPU *),c_stage10_GetVersionExA(CPU *);
extern void c_stage10_GetLastError(CPU *),c_stage10_SetLastError(CPU *);
static _Alignas(16) unsigned char output[168];
static unsigned abi_calls;
static void check(int condition) { if(!condition)exit(4); }
static uint32_t invoke(CPU *cpu,void (*fn)(CPU *),const uint32_t *args,size_t count) {
    uint32_t before=cpu->esp;
    cpu->ebx=0x13579bdf;cpu->esi=0x2468ace0;cpu->edi=0xaabbccdd;cpu->ebp=0xdeadbeef;
    for(size_t i=count;i>0;--i) {
        cpu->esp-=4;memcpy((void*)(uintptr_t)(cpu->pointer_offset+cpu->esp),&args[i-1],4);
    }
    fn(cpu);
    check(cpu->esp==before&&cpu->ebx==0x13579bdf&&cpu->esi==0x2468ace0&&
          cpu->edi==0xaabbccdd&&cpu->ebp==0xdeadbeef);
    ++abi_calls;return cpu->eax;
}
static void seed(CPU *cpu) { uint32_t a[]={0x13579};invoke(cpu,c_stage10_SetLastError,a,1); }
static void word(FILE *f,uint32_t value) { check(fwrite(&value,4,1,f)==1); }
int main(int argc,char **argv) {
    CPU cpu;check(initialize_cpu(&cpu,output));
    check(pointer32(&cpu,output)&&pointer32(&cpu,c_stage10_GetCPInfo));
    stage10_bind_buffer(output,sizeof(output),1);
    if(argc==2) {
        uint32_t a[]={932,pointer32(&cpu,output+4)};
        if(!strcmp(argv[1],"null-cp")) { a[0]=936;a[1]=0; }
        else if(!strcmp(argv[1],"read-only-cp")) { a[0]=936;stage10_bind_buffer(output,sizeof(output),0); }
        else if(!strcmp(argv[1],"truncated-version")) {
            uint32_t size=156;memcpy(output+4,&size,4);stage10_bind_buffer(output,12,1);
            uint32_t v[]={pointer32(&cpu,output+4)};invoke(&cpu,c_stage10_GetVersionExA,v,1);return 5;
        } else check(!strcmp(argv[1],"unknown-cp"));
        invoke(&cpu,c_stage10_GetCPInfo,a,2);return 5;
    }
    check(argc==3&&!strcmp(argv[1],"capture"));
    FILE *f=fopen(argv[2],"wb");check(f!=NULL);
    seed(&cpu);word(f,invoke(&cpu,c_stage10_GetACP,NULL,0));
    check(invoke(&cpu,c_stage10_GetLastError,NULL,0)==0x13579);
    word(f,invoke(&cpu,c_stage10_GetVersion,NULL,0));
    check(invoke(&cpu,c_stage10_GetLastError,NULL,0)==0x13579);
    const uint32_t cps[]={936,0,1,2,3,65001,1252,UINT32_MAX,UINT32_MAX};
    for(size_t i=0;i<9;++i) {
        memset(output,0xa5,sizeof(output));seed(&cpu);
        uint32_t a[]={cps[i],i==8?0:pointer32(&cpu,output+4)};
        word(f,invoke(&cpu,c_stage10_GetCPInfo,a,2));
        word(f,invoke(&cpu,c_stage10_GetLastError,NULL,0));
        check(fwrite(output,32,1,f)==1);
    }
    const uint32_t sizes[]={0,4,147,148,149,156,157,UINT32_MAX};
    for(size_t i=0;i<8;++i) {
        memset(output,0xa5,sizeof(output));memcpy(output+4,&sizes[i],4);seed(&cpu);
        uint32_t a[]={pointer32(&cpu,output+4)};
        word(f,invoke(&cpu,c_stage10_GetVersionExA,a,1));
        word(f,invoke(&cpu,c_stage10_GetLastError,NULL,0));
        check(fwrite(output,sizeof(output),1,f)==1);
    }
    check(fclose(f)==0);
    printf("{\"execution_backend\":\"LLasm LLVM arm64 wrappers + native C\",\"abi_calls\":%u,\"stdcall_stack_and_nonvolatile_preserved\":true,\"guest_pointer_bits\":32,\"pointer_offset\":%llu,\"GetACP\":936,\"GetVersion\":\"0x1db10106\",\"original_Emperor_instructions_executed\":false}\n",abi_calls,(unsigned long long)cpu.pointer_offset);
    return 0;
}
