/* Authored LLVM/SR-llasm code only. Never links the game object. */
#include "runtime.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern void stage81_authored_run(S78CPU *);
extern void stage81_authored_heap_alloc(S78CPU *),stage81_authored_tls_get(S78CPU *);
extern void stage81_authored_table_probe1(S78CPU *),stage81_authored_table_probe2(S78CPU *);
extern S80Binding stage81_authored_bindings[3];
extern const S81DomainCase stage81_domain_cases[];
extern const size_t stage81_domain_case_count;
extern S80Runtime *s80_active_runtime(void);
extern void *s80_bound_target(uint32_t);
extern void s80_domain_stop(void) __attribute__((noreturn));
extern void x86_rep_movsb(S78CPU *);
extern uint32_t stage77_HeapAlloc_c(uint32_t,uint32_t,uint32_t),stage77_HeapReAlloc_c(uint32_t,uint32_t,uint32_t,uint32_t),stage77_HeapSize_c(uint32_t,uint32_t,uint32_t),stage79_GetCurrentThreadId_c(void),stage79_GetLastError_c(void),stage79_SetLastError_c(uint32_t),stage79_TlsGetValue_c(uint32_t),stage79_TlsSetValue_c(uint32_t,uint32_t);
static unsigned callbacks;
void *stage81_lookup_authored_call(uint32_t pc,uint32_t target){S80Runtime *r=s80_active_runtime();if(!r||pc!=r->pc||pc!=0xa0000100||target!=0xa0000200)s80_domain_stop();callbacks++;return s80_bound_target(target);}
static uint32_t word(S80Runtime *r,uint32_t p){uint32_t v;assert(s80_read(r,p,4,(uint8_t *)&v));return v;}
static void put(S80Runtime *r,uint32_t p,uint32_t v){assert(s80_write(r,p,4,(uint8_t *)&v));}
static unsigned run_code(S80Runtime *r,uint32_t value,uint32_t slot){
 r->authored_execution=1;unsigned outside=0;assert(!s80_set_bindings(r,stage81_authored_bindings,3,stage81_authored_bindings,3));
 for(unsigned i=0;i<3;i++){uintptr_t p=(uintptr_t)stage81_authored_bindings[i].target;if(p<r->cpu.pointer_offset||p-r->cpu.pointer_offset>UINT32_MAX)outside++;}
 assert(outside==3);/* A full data arena cannot also contain unrelated code. */
 r->pc=0xa0000000;put(r,value,0xfffffff9);put(r,slot,0xa0000200);uint32_t sp=0x70080000;put(r,sp,0xa0000004);r->cpu.r[S78_EAX]=value;r->cpu.r[S78_EDX]=slot;r->cpu.r[S78_ECX]=3;r->cpu.r[S78_ESP]=sp;uint32_t flags=r->cpu.r[S78_FLAGS];size_t start=r->writes;callbacks=0;
 stage81_authored_run(&r->cpu);
 assert(callbacks==3&&word(r,value)==23&&word(r,slot)==0xa0000200&&r->cpu.r[S78_ESP]==sp+4&&r->cpu.r[S78_ECX]==0&&r->cpu.r[S78_FLAGS]==flags&&r->pc==0xa0000004&&r->writes==start+9);
 const uint32_t values[]={0,3,10,13,20,23};unsigned k=0;
 for(size_t i=start;i<r->writes;i++){S80Write *w=r->trace+i;if((i-start)%3==0)assert(w->address==sp-4&&w->value==0xa0000105&&w->pc==0xa0000100);else assert(w->address==value&&w->value==values[k++]);}
 printf("authored CALL/RET: arena=%llx native_callback=%llx stable_callback_token=a0000200 return=a0000105 value=23 outside-old-offset-domain=%u\n",(unsigned long long)r->cpu.pointer_offset,(unsigned long long)(uintptr_t)stage81_authored_bindings[1].target,outside);return outside;
}
static void failure(S80Runtime *r,int budget,uint32_t address){r->armed=1;int jumped=setjmp(r->escape);if(!jumped){if(budget==2)stage77_lookup_target(r->pc,address);else if(budget)stage72_guard((void *)(uintptr_t)(r->cpu.pointer_offset+address),4,2);else stage72_lookup_return(0xffffdead);assert(0);}r->armed=0;}
int main(int argc,char **argv){
 assert(argc==5);FILE *f=fopen(argv[2],"rb");assert(f);char magic[4];uint32_t count;assert(fread(magic,1,4,f)==4&&!memcmp(magic,"S79M",4)&&fread(&count,4,1,f)==1&&count<=64);S79Region regions[64];for(unsigned i=0;i<count;i++)assert(fread(regions+i,24,1,f)==1);assert(fgetc(f)==EOF);fclose(f);
 S80Sparse arena;memset(&arena,0,sizeof(arena));assert(!s79_memory_open(&arena.view,regions,count,argv[3]));arena.offset=(uintptr_t)arena.view.base;f=fopen(argv[3],"rb");assert(f);uint8_t *buf=malloc(1<<20);assert(buf);uint64_t bytes=0;
 for(unsigned i=0;i<count;i++){assert(!fseeko(f,(off_t)regions[i].offset,SEEK_SET));for(uint64_t at=regions[i].low;at<regions[i].end;){size_t n=regions[i].end-at>(1<<20)?(1<<20):(size_t)(regions[i].end-at);assert(fread(buf,1,n,f)==n&&!memcmp(buf,(void *)(uintptr_t)(arena.offset+at),n));bytes+=n;at+=n;}}free(buf);fclose(f);assert(bytes==297521152&&count==28);
 S80Runtime r;assert(!s80_import_threads(&r,argv[1],arena.offset));S80Write *log=calloc(5000000,sizeof(*log));assert(log);assert(!s80_connect(&r,&arena,log,5000000,128));s80_activate(&r);assert(!s81_import_heap(&r,argv[4]));assert(r.heap.heap_count==2&&r.heap.count==18131&&r.heap.next==491927584&&r.heap.mapped_end==0x1e000000);assert(s80_switch_thread(&r,2)==S79_THREAD_WAITING);
 r.pc=0xa0000000;uint32_t initial_next=r.heap.next;uint32_t p=stage77_HeapAlloc_c(0x3001,8,8),slot=stage77_HeapAlloc_c(0x3001,8,8);assert(p==initial_next&&slot==p+8&&stage77_HeapSize_c(0x3001,0,p)==8);run_code(&r,p,slot);
 uint32_t moved=stage77_HeapReAlloc_c(0x3001,8,p,16);assert(moved!=p&&word(&r,moved)==23&&stage77_HeapSize_c(0x3001,0,moved)==16);uint32_t n;assert(s77_size(&r.heap,0x3001,0,p,&n)==S77_BOUNDARY);
 uint32_t b=stage77_HeapAlloc_c(0x3001,8,64);put(&r,b,0x04030201);put(&r,b+4,0x08070605);r.cpu.r[S78_FLAGS]&=~0x400u;r.cpu.r[S78_ESI]=b;r.cpu.r[S78_EDI]=b+16;r.cpu.r[S78_ECX]=8;x86_rep_movsb(&r.cpu);assert(word(&r,b+16)==0x04030201&&word(&r,b+20)==0x08070605);
 r.cpu.r[S78_FLAGS]|=0x400;r.cpu.r[S78_ESI]=b+7;r.cpu.r[S78_EDI]=b+9;r.cpu.r[S78_ECX]=8;x86_rep_movsb(&r.cpu);assert(word(&r,b+2)==0x04030201&&word(&r,b+6)==0x08070605);
 uint32_t fs;assert(!s78_fs_read(&r.helpers,0,&fs)&&s78_fs_read(&r.helpers,4,&n)==S78_ADDRESS_BOUNDARY);assert(!s78_fs_write(&r.helpers,0,0xffffffff)&&!s78_fs_read(&r.helpers,0,&n)&&n==0xffffffff);assert(!s78_fs_write(&r.helpers,0,fs));
 S80Binding *conditional=calloc(stage81_domain_case_count,sizeof(*conditional));assert(conditional);
 for(size_t i=0;i<stage81_domain_case_count;i++)conditional[i]=(S80Binding){stage81_domain_cases[i].target,stage81_authored_bindings[1].target};assert(!s80_set_bindings(&r,stage81_authored_bindings,3,conditional,stage81_domain_case_count));
 for(size_t i=0;i<stage81_domain_case_count;i++){const S81DomainCase *c=stage81_domain_cases+i;r.pc=c->site;r.cpu.r[c->reg]=c->value;assert(stage77_lookup_target(c->site,c->target)==stage81_authored_bindings[1].target);}
 const S81DomainCase *unknown=stage81_domain_cases+21;r.pc=unknown->site;r.cpu.r[unknown->reg]=0x80000000;failure(&r,2,unknown->target);assert(r.status==S80_DYNAMIC_DOMAIN);r.cpu.r[unknown->reg]=unknown->value;failure(&r,2,unknown->target^4);assert(r.status==S80_DYNAMIC_DOMAIN);assert(!s80_set_bindings(&r,stage81_authored_bindings,3,stage81_authored_bindings,3));free(conditional);r.pc=0xa0000300;
 assert(stage79_GetCurrentThreadId_c()==1);stage79_SetLastError_c(99);assert(stage79_TlsSetValue_c(0,0xf1234567)==1&&stage79_GetLastError_c()==99&&stage79_TlsGetValue_c(0)==0xf1234567&&stage79_GetLastError_c()==0);
 stage81_authored_table_probe1(&r.cpu);assert(r.cpu.r[S78_EAX]==0x765358);stage81_authored_table_probe2(&r.cpu);assert(r.cpu.r[S78_EAX]==0x76ac88);
 uint32_t api_sp=0x70090000;put(&r,api_sp,0xa0000004);put(&r,api_sp+4,0x3001);put(&r,api_sp+8,8);put(&r,api_sp+12,3);uint32_t api_next=r.heap.next;r.cpu.r[S78_ESP]=api_sp;stage81_authored_heap_alloc(&r.cpu);assert(r.cpu.r[S78_EAX]==api_next&&r.cpu.r[S78_ESP]==api_sp+16&&word(&r,api_next)==0);
 put(&r,api_sp,0xa0000004);put(&r,api_sp+4,0);r.cpu.r[S78_ESP]=api_sp;stage79_SetLastError_c(44);stage81_authored_tls_get(&r.cpu);assert(r.cpu.r[S78_EAX]==0xf1234567&&r.cpu.r[S78_ESP]==api_sp+8&&stage79_GetLastError_c()==0);
 r.cpu.r[S78_ESP]=0x700ffffe;r.armed=1;int jumped=setjmp(r.escape);if(!jumped){stage81_authored_tls_get(&r.cpu);assert(0);}r.armed=0;assert(r.status==S80_MEMORY&&r.fault==0x70100002&&r.cpu.r[S78_ESP]==0x700ffffe);
 size_t oldcap=r.write_cap;r.write_cap=r.writes;uint32_t before=word(&r,b);failure(&r,1,b);assert(r.status==S80_BUDGET&&word(&r,b)==before);r.write_cap=oldcap;failure(&r,0,0);assert(r.status==S80_BINDING);
 uint32_t next=r.heap.next;uint32_t grown=stage77_HeapAlloc_c(0x3001,0,0x1000000);assert(grown==next&&r.heap.mapped_end==0x1f000000&&arena.view.count==29&&word(&r,grown)==0xcdcdcdcd&&word(&r,grown+0xfffffc)==0xcdcdcdcd);
 /* Same exact guest tokens, code pointers and code on a second data arena. */
 S80Sparse second;memset(&second,0,sizeof(second));S79Region toy[]={{0x10000000,3,0x10004000,0},{0x70000000,3,0x70100000,0}};assert(!s79_memory_open(&second.view,toy,2,NULL));second.offset=(uintptr_t)second.view.base;assert(second.offset!=arena.offset);S80Runtime r2;assert(!s80_import_threads(&r2,argv[1],second.offset));S80Write log2[64];assert(!s80_connect(&r2,&second,log2,64,128));s80_activate(&r2);run_code(&r2,0x10000000,0x10000008);
 s79_memory_close(&second.view);s77_dispose(&r.heap);s79_memory_close(&arena.view);free(log);printf("PASS joint own-SR-llasm-to-arm64 indirect CALL/RET/callback/stack/uint32 wrap, two arbitrary arenas, real memory %llu bytes, 18131 heap records plus 16MiB growth, REP directions/overlap, FS0 scope, TLS, ordered writes and pre-store budget; original game instructions=0\n",(unsigned long long)bytes);return 0;
}
