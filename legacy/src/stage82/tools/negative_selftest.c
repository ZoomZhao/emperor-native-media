#include "runtime.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern void s82_unimplemented(uint32_t);
extern int s82_import_locks(const char*),s82_import_api_sites(const char*);
static void own_return(void){}
int main(int argc,char**argv){
 assert(argc==8);char magic[4];uint32_t n;FILE*f=fopen(argv[2],"rb");assert(f&&fread(magic,1,4,f)==4&&!memcmp(magic,"S79M",4)&&fread(&n,4,1,f)==1&&n<=64);S79Region regions[64];assert(fread(regions,24,n,f)==n&&!fclose(f));S80Sparse m={0};assert(!s79_memory_open(&m.view,regions,n,argv[3]));m.offset=(uintptr_t)m.view.base;S80Runtime r;assert(!s80_import_threads(&r,argv[1],m.offset));S80Write log[64];assert(!s80_connect(&r,&m,log,64,100));assert(!s81_import_heap(&r,argv[4]));assert(!s82_import_locks(argv[5]));assert(!s82_import_api_sites(argv[7]));uint32_t events[32][11];r.events=events;r.event_cap=32;r.complete_pc=0xa0000005;r.complete_target=(void*)&own_return;s80_activate(&r);
 uint32_t ids[]={16,0,3,8,11,13,15,1,5};uint32_t args[][4]={{0},{0},{0x3001,0,0x10000000},{63},{0xa1000400},{0xa1000001,0xa1000000,8},{0xa1000000,0x55,8},{0x3001,4,8},{0}};unsigned expected[]={400,500,0,0,412,416,S80_BUDGET,0,S80_MEMORY};
 for(unsigned i=0;i<9;i++){
  r.status=0;r.fault=0;r.pending=0;r.api_id=0;r.cpu.r[4]=i==8?0x70001000:0x70000f00;r.pc=0xa0000300+i;r.write_cap=i==6?1:64;uint8_t before[16];memcpy(before,(void*)(uintptr_t)(m.offset+0xa1000000),16);uint32_t frame[5]={0xa0000005,args[i][0],args[i][1],args[i][2],args[i][3]};if(i!=8)memcpy((void*)(uintptr_t)(m.offset+r.cpu.r[4]),frame,sizeof(frame));r.armed=1;
  if(!setjmp(r.escape)){if(!ids[i])s82_unimplemented(123);else(void)s82_api_dispatch(&r.cpu,ids[i]);assert(!"negative operation returned success");}
  r.armed=0;assert(r.status&&(!expected[i]||r.status==expected[i])&&r.writes==0&&r.event_count==0&&r.steps==0&&!memcmp(before,(void*)(uintptr_t)(m.offset+0xa1000000),16));printf("negative %u explicit stop status=%u writes=0 scheduling=0\n",i,r.status);
 }
 for(unsigned i=0;i<3;i++){uint32_t target=i==1?0xa0000861:0xa0000860;memcpy((void*)(uintptr_t)(m.offset+0xa1000010),&target,4);r.cpu.r[2]=i==2?target+1:target;r.status=0;r.armed=1;if(!setjmp(r.escape)){s82_named_api_guard(0xa0000380);assert(i==0);}else assert(i!=0&&r.status==S80_BINDING);r.armed=0;assert(r.writes==0&&r.event_count==0);}
 f=fopen(argv[6],"wbx");assert(f&&fprintf(f,"{\"status\":\"TWELVE_AUTHORED_BOUNDARY_BINDING_CASES_PASSED\",\"cases\":12,\"original_game_instructions\":0,\"SEH_explicit_failure\":true,\"worker_scheduling\":0}\n")>0&&!fclose(f));s80_activate(NULL);s77_dispose(&r.heap);s79_memory_close(&m.view);return 0;
}
