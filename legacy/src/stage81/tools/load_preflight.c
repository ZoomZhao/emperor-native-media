#define _DARWIN_C_SOURCE
#include "runtime.h"
#include <dlfcn.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
_Static_assert(sizeof(S80Binding)==16,"LLVM/native binding ABI");
int main(int argc,char **argv){
 assert(argc==7);FILE *f=fopen(argv[2],"rb");assert(f);char magic[4];uint32_t nr,nt;uint64_t text,size;assert(fread(magic,1,4,f)==4&&!memcmp(magic,"S81B",4)&&fread(&nr,4,1,f)==1&&fread(&nt,4,1,f)==1&&fread(&text,8,1,f)==1&&fread(&size,8,1,f)==1&&nr==432&&nt==2020);uint32_t *pc=malloc((nr+nt)*4);assert(pc&&fread(pc,4,nr+nt,f)==nr+nt&&fgetc(f)==EOF);fclose(f);
 void *handle=dlopen(argv[1],RTLD_LAZY|RTLD_LOCAL);if(!handle){fprintf(stderr,"load-only failure: %s\n",dlerror());free(pc);return 2;}
 S80Binding *returns=dlsym(handle,"stage72_return_bindings"),*targets=dlsym(handle,"stage77_structural_code_bindings");uint32_t *actual=dlsym(handle,"stage72_return_bindings_count");assert(returns&&targets&&actual&&*actual==nr);Dl_info image;assert(dladdr(targets,&image));uintptr_t low=(uintptr_t)image.dli_fbase+text,high=low+size;
 for(uint32_t i=0;i<nr+nt;i++){S80Binding b=i<nr?returns[i]:targets[i-nr];uintptr_t p=(uintptr_t)b.target;assert(b.pc==pc[i]&&p>=low&&p<high&&(p&3)==0);}
 f=fopen(argv[4],"rb");assert(f);uint32_t n;assert(fread(magic,1,4,f)==4&&!memcmp(magic,"S79M",4)&&fread(&n,4,1,f)==1&&n<=64);S79Region regions[64];for(uint32_t i=0;i<n;i++)assert(fread(regions+i,24,1,f)==1);assert(fgetc(f)==EOF);fclose(f);
 S80Sparse memory;memset(&memory,0,sizeof(memory));assert(!s79_memory_open(&memory.view,regions,n,argv[5]));memory.offset=(uintptr_t)memory.view.base;S80Runtime r;assert(!s80_import_threads(&r,argv[3],memory.offset));S80Write log[1];assert(!s80_connect(&r,&memory,log,1,0));assert(!r.authored_execution);assert(!s81_import_heap(&r,argv[6]));assert(!s80_set_bindings(&r,returns,nr,targets,nt));unsigned outside=0;for(uint32_t i=0;i<nt;i++){uintptr_t p=(uintptr_t)targets[i].target;if(p<memory.offset||p-memory.offset>UINT32_MAX)outside++;}
 void (*activate)(S80Runtime *)=(void (*)(S80Runtime *))dlsym(handle,"s80_activate");S79Threads *(*threads)(void)=(S79Threads *(*)(void))dlsym(handle,"stage79_active_threads");S77State *(*heap)(void)=(S77State *(*)(void))dlsym(handle,"stage77_active_heap_context");S78Context *(*helpers)(void)=(S78Context *(*)(void))dlsym(handle,"stage78_current_context");assert(activate&&threads&&heap&&helpers);
 /* Only these four authored C setup/provider symbols are invoked. No SR
  * fastcc procedure or original-code binding is called, and gate stays closed. */
 activate(&r);S78Context *live=helpers();assert(threads()==&r.thread_set&&heap()==&r.heap&&live==&r.helpers&&live->opaque==&r);activate(NULL);
 assert(outside==nt&&r.steps==0&&r.writes==0&&r.pc==0x419035&&r.heap.count==18131&&r.heap.next==491927584&&!r.authored_execution);printf("PASS audited load-only: original guest return PCs %u/%u, code PCs %u/%u, native text bindings %u/%u, outside-old-offset-domain %u; private regions %u, heap records %zu; authored C setup/provider calls=5; original/SR-derived entry invocations=0, instruction steps=0, writes=0, execution gate closed, lazy platform imports remain unimplemented\n",nr,nr,nt,nt,nt,nt,outside,n,r.heap.count);
 s77_dispose(&r.heap);s79_memory_close(&memory.view);free(pc);assert(!dlclose(handle));return 0;
}
