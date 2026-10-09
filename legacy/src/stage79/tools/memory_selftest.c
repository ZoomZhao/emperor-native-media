#include "guest_memory.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv){
 S79Region authored[]={{0x1000,3,0x2000,0},{0x2000,1,0x3000,0},{0xfffff000,3,UINT64_C(0x100000000),0}};S79Memory m;assert(!s79_memory_open(&m,authored,3,NULL));uint8_t b[4]={1,2,3,4},out[4];uint32_t address;
 assert(!s79_guard(&m,0x1fff,2,1));assert(s79_guard(&m,0x1fff,2,2)==S79_PERMISSION);
 assert(s79_guard(&m,0x3000,1,1)==S79_UNMAPPED);assert(s79_guard(&m,0,1,1)==S79_UNMAPPED);assert(s79_guard(&m,0xffffffff,2,1)==S79_OVERFLOW);
 assert(!s79_guard(&m,0xffffffff,1,3));assert(!s79_guard(&m,0,0,1));
 assert(s79_write(&m,0xfffffffc,4,b)&&s79_read(&m,0xfffffffc,4,out)&&!memcmp(b,out,4));
 assert(!s79_host_to_guest(&m,m.base+0xfffffffc,&address)&&address==0xfffffffc);
 assert(s79_host_to_guest(&m,m.base+UINT64_C(0x100000000),&address)==S79_OVERFLOW);
 assert(!s79_write(&m,0x2ffc,4,b)&&m.writes==1);s79_memory_close(&m);
 puts("PASS authored cross-region permissions, holes, uint32 boundaries, no native pointer truncation, failed-write immutability");
 if(argc==3){
  FILE *plan=fopen(argv[1],"rb");assert(plan);char magic[4];uint32_t count;assert(fread(magic,1,4,plan)==4&&!memcmp(magic,"S79M",4)&&fread(&count,4,1,plan)==1&&count<=64);S79Region *regions=calloc(count,sizeof(*regions));assert(regions);
  for(unsigned i=0;i<count;i++)assert(fread(&regions[i].low,4,1,plan)==1&&fread(&regions[i].permissions,4,1,plan)==1&&fread(&regions[i].end,8,1,plan)==1&&fread(&regions[i].offset,8,1,plan)==1);assert(fgetc(plan)==EOF);fclose(plan);
  assert(!s79_memory_open(&m,regions,count,argv[2]));FILE *source=fopen(argv[2],"rb");assert(source);uint8_t *buffer=malloc(1<<20);assert(buffer);uint64_t bytes=0;
  for(unsigned i=0;i<count;i++){
   S79Region *r=regions+i;uint64_t at=r->low;assert(!fseeko(source,(off_t)r->offset,SEEK_SET));
   while(at<r->end){size_t n=r->end-at>(1<<20)?(1<<20):(size_t)(r->end-at);assert(fread(buffer,1,n,source)==n);/* import bytes compare only: does not execute them */assert(!memcmp(buffer,m.base+at,n));at+=n;bytes+=n;}
   assert(!s79_guard(&m,r->low,r->end-r->low,r->permissions&3u));
   if(!(r->permissions&2))assert(s79_guard(&m,r->low,1,2)==S79_PERMISSION);
  }
  printf("PASS real checkpoint private memory import: %u regions, %llu bytes compared; host execute permission never enabled; no game/Unicorn/SR calls\n",count,(unsigned long long)bytes);
  assert(bytes==297521152&&count==28);free(buffer);fclose(source);s79_memory_close(&m);free(regions);
 }
 return 0;
}
