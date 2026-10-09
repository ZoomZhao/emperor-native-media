#define _DARWIN_C_SOURCE
#include "runtime.h"
#include "build_scope.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
extern S80Binding stage72_return_bindings[],stage77_structural_code_bindings[];
extern uint32_t stage72_return_bindings_count;
extern void s82_entry(S78CPU*),s82_completion(void);
extern int s82_import_locks(const char*),s82_export_locks(const char*),s82_import_api_sites(const char*);
static void path(char*out,const char*dir,const char*name){assert(snprintf(out,4096,"%s/%s",dir,name)<4096);}
static void dump(const char*p,const void*b,size_t n){FILE*f=fopen(p,"wbx");assert(f&&fwrite(b,1,n,f)==n&&!fclose(f));}
int main(int argc,char**argv){
 if(argc!=5){fprintf(stderr,"usage: native-driver INPUT_DIR OUTPUT_DIR LIMITS_BIN EXECUTION_TICKET\n");return 2;}
 char p[4096],magic[4];FILE*f=fopen(argv[4],"rb");uint32_t mode;
 if(!f||fread(magic,1,4,f)!=4||memcmp(magic,"S82T",4)||fread(&mode,4,1,f)!=1||mode!=S82_MODE){if(f)fclose(f);fprintf(stderr,"EXECUTION_GATE_CLOSED\n");return 3;}
 uint8_t receipt[64];if(fread(receipt,1,64,f)!=64||fgetc(f)!=EOF){fclose(f);return 3;}fclose(f);/* manifest and input hashes are checked by the orchestrator before its exclusive claim/ticket */
 uint64_t cap[3];f=fopen(argv[3],"rb");assert(f&&fread(cap,8,3,f)==3&&fgetc(f)==EOF&&!fclose(f));assert(cap[0]&&cap[0]<=6000000&&cap[1]&&cap[1]<=2000000&&cap[2]&&cap[2]<=8000000000ULL);
 path(p,argv[1],"regions.bin");f=fopen(p,"rb");uint32_t n;assert(f&&fread(magic,1,4,f)==4&&!memcmp(magic,"S79M",4)&&fread(&n,4,1,f)==1&&n<=64);S79Region region[64];assert(fread(region,24,n,f)==n&&fgetc(f)==EOF&&!fclose(f));
 S80Sparse memory={0};path(p,argv[1],"memory.bin");assert(!s79_memory_open(&memory.view,region,n,p));memory.offset=(uintptr_t)memory.view.base;
 S80Runtime r;path(p,argv[1],"threads.bin");assert(!s80_import_threads(&r,p,memory.offset));assert(r.pc==S82_ENTRY);S80Write*trace=calloc((size_t)cap[1],sizeof(*trace));assert(trace&&!s80_connect(&r,&memory,trace,(size_t)cap[1],cap[0]));path(p,argv[1],"heap.bin");assert(!s81_import_heap(&r,p));path(p,argv[1],"locks.bin");assert(!s82_import_locks(p));assert(!s80_set_bindings(&r,stage72_return_bindings,stage72_return_bindings_count,stage77_structural_code_bindings,S82_TARGET_COUNT));
 path(p,argv[1],"api-sites.bin");assert(!s82_import_api_sites(p));uint32_t stack_return;assert(s80_read(&r,r.cpu.r[4],4,(uint8_t*)&stack_return)&&stack_return==S82_RETURN);r.mode=mode;r.complete_pc=S82_RETURN;r.complete_sp=r.cpu.r[4]+4;r.complete_target=(void*)&s82_completion;r.allowed=s82_allowed;r.allowed_count=sizeof(s82_allowed)/sizeof(*s82_allowed);r.events=calloc(4096,sizeof(*r.events));assert(r.events);r.event_cap=4096;struct timespec now;clock_gettime(CLOCK_MONOTONIC,&now);r.deadline=now.tv_sec+now.tv_nsec/1e9+cap[2]/1e9;s80_activate(&r);r.armed=1;
 if(!setjmp(r.escape))s82_entry(&r.cpu);
 r.armed=0;s80_activate(NULL);path(p,argv[2],"native-result.bin");uint64_t result[6]={r.status,r.completed,r.pc,r.fault,r.steps,r.writes};dump(p,result,sizeof(result));
 path(p,argv[2],"writes.bin");dump(p,trace,r.writes*sizeof(*trace));path(p,argv[2],"api-events.bin");dump(p,r.events,r.event_count*sizeof(*r.events));path(p,argv[2],"core.bin");uint32_t core[10];memcpy(core,r.cpu.r,32);core[8]=r.pc;core[9]=r.cpu.r[8];dump(p,core,sizeof(core));
 path(p,argv[2],"locks.bin");assert(!s82_export_locks(p));path(p,argv[2],"tls.bin");S79Thread*t=r.threads+r.thread_set.current;uint8_t tls[268];memcpy(tls,&t->last_error,4);memcpy(tls+4,&t->allocated,8);memcpy(tls+12,t->tls,256);dump(p,tls,sizeof(tls));
 size_t live=s77_export_live(&r.heap,NULL,0);S77Allocation*rows=calloc(live?live:1,sizeof(*rows));assert(rows);s77_export_live(&r.heap,rows,live);path(p,argv[2],"heap-state.bin");f=fopen(p,"wbx");uint32_t hh[4]={r.heap.next,r.heap.mapped_end,(uint32_t)live,(uint32_t)r.heap.heap_count};assert(f&&fwrite(hh,4,4,f)==4);for(size_t i=0;i<live;i++)assert(fwrite(rows+i,4,3,f)==3);assert(!fclose(f));
 path(p,argv[2],"regions.bin");f=fopen(p,"wbx");uint32_t nr=(uint32_t)memory.view.count;assert(f&&fwrite("S79M",1,4,f)==4&&fwrite(&nr,4,1,f)==1);uint64_t offset=0;path(p,argv[2],"memory.bin");FILE*mem=fopen(p,"wbx");assert(mem);for(size_t i=0;i<memory.view.count;i++){S79Region q=memory.view.regions[i];q.offset=offset;assert(fwrite(&q,24,1,f)==1);uint64_t size=q.end-q.low;assert(fwrite((void*)(uintptr_t)(memory.offset+q.low),1,size,mem)==size);offset+=size;}assert(!fclose(f)&&!fclose(mem));
 printf("native driver: mode=%u completed=%u status=%u PC=%08x steps=%llu writes=%zu APIs=%zu; original execution only if mode=1 ticket\n",mode,r.completed,r.status,r.pc,(unsigned long long)r.steps,r.writes,r.event_count);free(rows);free(r.events);free(trace);s77_dispose(&r.heap);s79_memory_close(&memory.view);return r.completed&&!r.status?0:4;
}
