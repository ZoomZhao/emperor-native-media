#include "runtime.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
extern int own_a(void);
extern int own_b(void);
extern uint32_t stage79_GetCurrentThreadId_c(void),stage79_GetLastError_c(void),stage79_SetLastError_c(uint32_t),stage79_TlsGetValue_c(uint32_t),stage79_TlsSetValue_c(uint32_t,uint32_t);
extern void x86_rep_movsd(S78CPU *);
static void expected_stop(S80Runtime *r,int kind,uint32_t target){
 r->armed=1;int jumped=setjmp(r->escape);if(!jumped){if(kind==0)stage72_guard((void *)(uintptr_t)(r->memory->offset+0x400000),4,2);else if(kind==1)stage77_lookup_target(r->pc,target);else stage72_lookup_return(target);assert(0);}r->armed=0;
}
int main(int argc,char **argv){assert(argc==5);uint64_t page=(uint64_t)sysconf(_SC_PAGESIZE);uintptr_t anchor=(uintptr_t)own_a<(uintptr_t)own_b?(uintptr_t)own_a:(uintptr_t)own_b;assert(anchor>0x100000);uint64_t offset=0;(void)page;(void)anchor;
 fprintf(stderr,"S80 own text anchor=%llx logical offset=%llx\n",(unsigned long long)anchor,(unsigned long long)offset);
 S79Region toy[]={{0x400000,3,0x401000,0},{0x401000,1,0x402000,0},{0x7b7000,1,0x7b8000,0}};S80Sparse m;memset(&m,0,sizeof(m));assert(!s79_memory_open(&m.view,toy,3,NULL));offset=(uintptr_t)m.view.base;m.offset=offset;S80Runtime r;assert(!s80_import_threads(&r,argv[1],offset));assert(r.thread_set.current==0&&r.threads[0].tid==1&&r.pc==0x419035);S79Thread before[4];memcpy(before,r.threads,sizeof(before));
 assert(!s80_export_initial_threads(&r,argv[4]));assert(s80_switch_thread(&r,2)==S79_THREAD_WAITING&&r.thread_set.current==0&&!memcmp(before,r.threads,sizeof(before)));assert(s80_switch_thread(&r,99)==S79_THREAD_UNKNOWN);
 assert(own_a()==17&&own_b()==23);
 S80Write log[8];assert(!s80_connect(&r,&m,log,8,20));s80_activate(&r);S80Binding ret[]={{0x111111,(void *)(uintptr_t)(offset+0x80000000)}},targets[]={{0x5d4770,(void *)(uintptr_t)(offset+0x80000010)}};assert(!s80_set_bindings(&r,ret,1,targets,1));S80Binding bad[]={{1,(void *)(uintptr_t)(offset+UINT64_C(0x100000000))}};assert(s80_set_bindings(&r,bad,1,targets,1)==S80_BINDING);assert(stage72_lookup_return(0x111111)==(void *)(uintptr_t)(offset+0x80000000));
 assert(s79_guard(&m.view,0x400fff,2,1)==0&&s79_guard(&m.view,0x400fff,2,2)==S79_PERMISSION&&s79_guard(&m.view,0x402000,1,1)==S79_UNMAPPED);
 stage72_step(0x12345);void *p=(void *)(uintptr_t)(offset+0x400000);stage72_guard(p,4,2);*(uint32_t *)p=0xf1234567;stage72_note_write(p,4,0xf1234567);assert(r.writes==1&&log[0].pc==0x12345&&log[0].address==0x400000&&log[0].value==0xf1234567);
 r.write_cap=1;expected_stop(&r,0,0);assert(r.status==S80_BUDGET&&*(uint32_t *)p==0xf1234567&&r.writes==1);r.write_cap=8;
 /* Readonly guest table populated only as authored fixture in private backing. */
 /* Host page is R only; use a separate temporary writable toy logical plan. */
 r.cpu.r[S78_EAX]=0x7b72c8;r.pc=0x4aef47;expected_stop(&r,1,0x5d4770);assert(r.status==S80_DYNAMIC_DOMAIN);expected_stop(&r,2,0x222222);assert(r.status==S80_BINDING);
 r.cpu.r[0]=0x87654321;r.cpu.r[8]=0x602;r.pc=0x765432;s80_save_core(&r);assert(r.threads[0].core[0]==0x87654321&&r.threads[0].core[8]==0x765432&&r.threads[0].core[9]==0x602&&!memcmp(before[0].opaque,r.threads[0].opaque,5608));
 s79_memory_close(&m.view);assert(own_a()==17);
 FILE *f=fopen(argv[2],"rb");assert(f);char magic[4];uint32_t count;assert(fread(magic,1,4,f)==4&&!memcmp(magic,"S79M",4)&&fread(&count,4,1,f)==1&&count<=64);S79Region regions[64];for(unsigned i=0;i<count;i++)assert(fread(&regions[i],24,1,f)==1);assert(fgetc(f)==EOF);fclose(f);
 assert(!s79_memory_open(&m.view,regions,count,argv[3]));offset=(uintptr_t)m.view.base;m.offset=offset;r.cpu.pointer_offset=offset;ret[0].target=(void *)(uintptr_t)(offset+0x80000000);targets[0].target=(void *)(uintptr_t)(offset+0x80000010);assert(!s80_set_bindings(&r,ret,1,targets,1));assert(!s80_connect(&r,&m,log,8,20));f=fopen(argv[3],"rb");assert(f);unsigned char *buf=malloc(1<<20);assert(buf);uint64_t bytes=0;
 for(unsigned i=0;i<count;i++){assert(!fseeko(f,(off_t)regions[i].offset,SEEK_SET));for(uint64_t a=regions[i].low;a<regions[i].end;){size_t n=regions[i].end-a>(1<<20)?(1<<20):(size_t)(regions[i].end-a);assert(fread(buf,1,n,f)==n&&!memcmp(buf,(void *)(uintptr_t)(offset+a),n));bytes+=n;a+=n;}}
 assert(count==28&&bytes==297521152&&own_a()==17&&own_b()==23);free(buf);fclose(f);
 r.cpu.r[S78_EAX]=0x7b72c8;r.pc=0x4aef47;assert(stage77_lookup_target(r.pc,0x5d4770)==(void *)(uintptr_t)(offset+0x80000010));r.cpu.r[S78_EAX]=0x7b72cc;expected_stop(&r,1,0x5d4770);assert(r.status==S80_DYNAMIC_DOMAIN);
 uint32_t fs=0;assert(!s78_fs_read(&r.helpers,0,&fs));assert(s78_fs_read(&r.helpers,4,&fs)==S78_ADDRESS_BOUNDARY);
 assert(stage79_GetCurrentThreadId_c()==1&&stage79_GetLastError_c()==2);stage79_SetLastError_c(99);assert(stage79_GetLastError_c()==99&&stage79_TlsGetValue_c(0)==0x10000000&&stage79_GetLastError_c()==0);stage79_SetLastError_c(88);assert(stage79_TlsSetValue_c(0,0xf1234567)==1&&stage79_GetLastError_c()==88&&stage79_TlsGetValue_c(0)==0xf1234567&&stage79_GetLastError_c()==0);
 r.cpu.r[S78_ECX]=1;r.cpu.r[S78_ESI]=0x400000;r.cpu.r[S78_EDI]=0x10000000;size_t prior=r.writes;x86_rep_movsd(&r.cpu);assert(r.writes==prior+1&&log[prior].address==0x10000000&&!memcmp((void *)(uintptr_t)(offset+0x400000),(void *)(uintptr_t)(offset+0x10000000),4));
 assert(!memcmp(before[0].opaque,r.threads[0].opaque,5608));s79_memory_close(&m.view);
 printf("PASS authored runtime: four exact CPU/FP80 sidecars, waiting workers refused, synthetic binding arithmetic (no code-placement claim), 28 private regions / %llu bytes compared, ordered write and pre-store budget, conditional domain, FS:[0]-only guard; no game code linked/executed\n",(unsigned long long)bytes);return 0;
}
