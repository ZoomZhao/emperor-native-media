/* Runs authored adapter/data tests only. No game or SR/Unicorn linkage. */
#include "heap_native.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define LOW UINT32_C(0x10000000)
typedef struct {unsigned char*bytes;uint32_t capacity,mapped;unsigned fills,copies,maps,fail_fill;} Arena;
static int check(void*v,uint32_t p,uint32_t n,unsigned permission){Arena*a=v;return !(permission&~3u)&&p>=LOW&&(uint64_t)p+n<=a->mapped&&(uint64_t)p+n<=LOW+a->capacity;}
static int map(void*v,uint32_t old,uint32_t end){Arena*a=v;if(old!=a->mapped||end<old||end>LOW+a->capacity)return 0;memset(a->bytes+old-LOW,0,end-old);a->mapped=end;a->maps++;return 1;}
static int fill(void*v,uint32_t p,uint32_t n,unsigned char value){Arena*a=v;if(a->fail_fill||!check(v,p,n,2))return 0;memset(a->bytes+p-LOW,value,n);a->fills++;return 1;}
static int copy(void*v,uint32_t dst,uint32_t src,uint32_t n){Arena*a=v;if(!check(v,src,n,1)||!check(v,dst,n,2))return 0;memmove(a->bytes+dst-LOW,a->bytes+src-LOW,n);a->copies++;return 1;}
static S77Memory memory(Arena*a){return(S77Memory){a,check,map,fill,copy};}
static void own_tests(void){
 Arena a={0};a.capacity=0x2000000;a.mapped=LOW+0x1000000;a.bytes=calloc(a.capacity,1);assert(a.bytes);
 S77Heap heaps[]={{0x3001,0},{0x3f00,0}};S77State s={0};assert(s77_import(&s,heaps,2,NULL,0,LOW,a.mapped,memory(&a))==S77_OK);
 uint32_t p,q,n,oldnext;assert(s77_alloc(&s,0x3001,8,3,&p)==0&&p==LOW);for(unsigned i=0;i<8;i++)assert(a.bytes[i]==0);assert(s77_size(&s,0x3001,0,p,&n)==0&&n==3);
 assert(s77_alloc(&s,0x3f00,0,17,&q)==0&&q==LOW+8);for(unsigned i=0;i<24;i++)assert(a.bytes[q-LOW+i]==0xcd);assert(s77_size(&s,0x3001,0,q,&n)==S77_BOUNDARY);
 assert(s77_free(&s,0x3001,0,p)==0);assert(s77_free(&s,0x3001,0,p)==S77_BOUNDARY);assert(s77_free(&s,0x3001,0,0)==S77_BOUNDARY);assert(s77_size(&s,0x3001,0,p,&n)==S77_BOUNDARY);
 memset(a.bytes+q-LOW,0xab,17);uint32_t moved;assert(s77_realloc(&s,0x3f00,8,q,24,&moved)==0&&moved!=q);for(unsigned i=0;i<17;i++)assert(a.bytes[moved-LOW+i]==0xab);for(unsigned i=17;i<24;i++)assert(a.bytes[moved-LOW+i]==0);assert(s77_size(&s,0x3f00,0,q,&n)==S77_BOUNDARY);
 oldnext=s.next;uint32_t oom=7;assert(s77_realloc(&s,0x3f00,8,moved,UINT32_MAX,&oom)==0&&oom==0&&s.next==oldnext);assert(s77_size(&s,0x3f00,0,moved,&n)==0&&n==24);for(unsigned i=0;i<17;i++)assert(a.bytes[moved-LOW+i]==0xab);
 for(unsigned i=0;i<150;i++){uint32_t x;assert(s77_alloc(&s,0x3001,0,8,&x)==0);assert(x>=oldnext);}assert(s.count>128);
 assert(s77_realloc(&s,0x3f00,0,moved,16,&q)==0);for(unsigned i=0;i<16;i++)assert(a.bytes[q-LOW+i]==0xab);
 size_t live=s77_export_live(&s,NULL,0);S77Allocation*rows=calloc(live,sizeof(*rows));assert(rows&&s77_export_live(&s,rows,live)==live);unsigned writes=a.fills+a.copies;
 S77State restored={0};assert(s77_import(&restored,heaps,2,rows,live,s.next,s.mapped_end,memory(&a))==0);assert(restored.next==s.next&&restored.mapped_end==s.mapped_end&&a.fills+a.copies==writes);assert(s77_size(&restored,0x3f00,0,q,&n)==0&&n==16);
 S77State rejected={0};uint32_t saved=rows[0].heap;rows[0].heap=0xbad;assert(s77_import(&rejected,heaps,2,rows,live,s.next,s.mapped_end,memory(&a))==S77_METADATA&&!rejected.heaps);rows[0].heap=saved;
 uint32_t savedp=rows[1].pointer;rows[1].pointer=rows[0].pointer;assert(s77_import(&rejected,heaps,2,rows,live,s.next,s.mapped_end,memory(&a))==S77_METADATA);rows[1].pointer=savedp;
 oldnext=s.next;assert(s77_alloc(&s,0x3001,4,8,&p)==S77_BOUNDARY&&s.next==oldnext);assert(s77_destroy_heap(&s,0x3001)==S77_BOUNDARY);
 /* A future allocation crosses a chunk boundary; guest VA remains unchanged. */
 assert(s77_alloc(&s,0x3001,0,0x1000000,&p)==0&&p==oldnext&&a.maps==1&&s.mapped_end==LOW+0x2000000);
 a.fail_fill=1;assert(s77_alloc(&s,0x3001,0,8,&p)==S77_MEMORY&&s.poisoned);assert(s77_alloc(&s,0x3001,0,8,&p)==S77_BOUNDARY);
 s77_dispose(&restored);s77_dispose(&s);free(rows);free(a.bytes);
 printf("{\"authored_heap_scenarios_passed\":12,\"original_or_SR_or_Unicorn_linked\":false}\n");
}
typedef struct {S77Region*regions;size_t count;} MetadataMemory;
static int metadata_check(void*v,uint32_t p,uint32_t n,unsigned permission){
 MetadataMemory*m=v;uint64_t end=(uint64_t)p+n,cursor=p;if(end>UINT64_C(0x100000000))return 0;
 for(size_t i=0;i<m->count;i++){S77Region r=m->regions[i];if(r.low<=cursor&&cursor<r.end&&(r.permissions&permission)==permission)cursor=end<r.end?end:r.end;if(cursor==end)return 1;}return 0;
}
static int forbidden_map(void*v,uint32_t a,uint32_t b){(void)v;(void)a;(void)b;assert(!"real checkpoint must not map memory");return 0;}
static int forbidden_fill(void*v,uint32_t a,uint32_t b,unsigned char c){(void)v;(void)a;(void)b;(void)c;assert(!"real checkpoint must not write bytes");return 0;}
static int forbidden_copy(void*v,uint32_t a,uint32_t b,uint32_t c){(void)v;(void)a;(void)b;(void)c;assert(!"real checkpoint must not copy bytes");return 0;}
static uint32_t u32(FILE*f){unsigned char b[4];assert(fread(b,1,4,f)==4);return(uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;}
static Arena copy_real_heap(const char*memory_path,const char*plan_path){
 FILE*plan=fopen(plan_path,"rb"),*source=fopen(memory_path,"rb");assert(plan&&source);
 char magic[4];assert(fread(magic,1,4,plan)==4&&!memcmp(magic,"S77M",4));assert(u32(plan)==1);uint32_t count=u32(plan),end=u32(plan);assert(count<=128&&end>LOW&&end<=0x20000000);
 Arena a={0};a.capacity=end-LOW;a.mapped=end;a.bytes=malloc(a.capacity);assert(a.bytes);uint32_t cursor=LOW;
 for(uint32_t i=0;i<count;i++){
  uint32_t low=u32(plan),high=u32(plan),permission=u32(plan),lo=u32(plan),hi=u32(plan);uint64_t offset=(uint64_t)lo|((uint64_t)hi<<32);
  assert(low==cursor&&high>low&&high<=end&&permission==3);assert(fseeko(source,(off_t)offset,SEEK_SET)==0);assert(fread(a.bytes+low-LOW,1,high-low,source)==high-low);
  /* Independently reread and compare every copied byte, including freed holes. */
  assert(fseeko(source,(off_t)offset,SEEK_SET)==0);unsigned char chunk[8192];uint32_t done=0;
  while(done<high-low){uint32_t n=high-low-done;if(n>sizeof(chunk))n=sizeof(chunk);assert(fread(chunk,1,n,source)==n&&!memcmp(chunk,a.bytes+low-LOW+done,n));done+=n;}
  cursor=high;
 }
 assert(cursor==end&&fgetc(plan)==EOF);fclose(plan);fclose(source);return a;
}
static void real_metadata(const char*path,const char*memory_path,const char*plan_path){
 FILE*f=fopen(path,"rb");assert(f);char magic[4];assert(fread(magic,1,4,f)==4&&!memcmp(magic,"S77H",4));assert(u32(f)==1);
 uint32_t nh=u32(f),nr=u32(f),na=u32(f),next=u32(f),mapped=u32(f);assert(nh<=4096&&nr<=128&&na<=1000000);
 S77Heap*heaps=calloc(nh,sizeof(*heaps));S77Region*regions=calloc(nr,sizeof(*regions));S77Allocation*rows=calloc(na,sizeof(*rows));assert(heaps&&regions&&rows);
 for(uint32_t i=0;i<nh;i++){heaps[i].handle=u32(f);heaps[i].options=u32(f);}
 for(uint32_t i=0;i<nr;i++){regions[i].low=u32(f);regions[i].end=u32(f);regions[i].permissions=u32(f);}
 for(uint32_t i=0;i<na;i++){rows[i].pointer=u32(f);rows[i].heap=u32(f);rows[i].size=u32(f);rows[i].live=1;}assert(fgetc(f)==EOF);fclose(f);
 MetadataMemory m={regions,nr};S77Memory callbacks={&m,metadata_check,forbidden_map,forbidden_fill,forbidden_copy};Arena real={0};
 if(memory_path){real=copy_real_heap(memory_path,plan_path);assert(real.mapped==mapped);callbacks=memory(&real);}
 S77State s={0};assert(s77_import(&s,heaps,nh,rows,na,next,mapped,callbacks)==S77_OK);
 S77Allocation*exported=calloc(na,sizeof(*exported));assert(exported&&s77_export_live(&s,exported,na)==na);for(uint32_t i=0;i<na;i++){uint32_t size;assert(s77_size(&s,rows[i].heap,0,rows[i].pointer,&size)==0&&size==rows[i].size);assert(exported[i].pointer==rows[i].pointer&&exported[i].heap==rows[i].heap&&exported[i].size==rows[i].size);}
 printf("{\"real_metadata_imported_and_roundtrip_checked\":%u,\"heap_handles_preserved\":%u,\"heap_next\":%u,\"mapped_end\":%u,\"source_heap_bytes_modified\":false,\"native_full_runtime_restored\":false}\n",na,nh,s.next,s.mapped_end);
 if(memory_path){
  /* Execute our authored heap adapter on its private COPY, never game code. */
  uint32_t pointer,size;assert(s77_alloc(&s,heaps[0].handle,8,3,&pointer)==0&&pointer==next);assert(s77_size(&s,heaps[0].handle,0,pointer,&size)==0&&size==3);for(unsigned i=0;i<8;i++)assert(real.bytes[pointer-LOW+i]==0);
  uint32_t old=rows[0].pointer,newp;uint32_t keep=rows[0].size<16?rows[0].size:16;unsigned char prior[16];memcpy(prior,real.bytes+old-LOW,keep);
  assert(s77_realloc(&s,rows[0].heap,8,old,16,&newp)==0&&!memcmp(prior,real.bytes+newp-LOW,keep));assert(s77_size(&s,rows[0].heap,0,old,&size)==S77_BOUNDARY);
  printf("{\"native_private_heap_bytes_restored_and_compared\":%u,\"all_original_guest_addresses_preserved\":true,\"authored_adapter_alloc_and_realloc_on_private_copy_passed\":true,\"source_checkpoint_modified\":false,\"game_instruction_executed\":false,\"full_guest_CPU_runtime_restored\":false}\n",real.capacity);
  free(real.bytes);
 }
 s77_dispose(&s);free(exported);free(rows);free(regions);free(heaps);
}
int main(int argc,char**argv){own_tests();if(argc==2)real_metadata(argv[1],NULL,NULL);else if(argc==4)real_metadata(argv[1],argv[2],argv[3]);else assert(argc==1);return 0;}
