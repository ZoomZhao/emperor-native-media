#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {uint32_t address,depth,owner;} Lock;
static Lock locks[4096];static size_t lock_count;
typedef struct {uint32_t site,slot,thunk,reg;} ApiSite;
static ApiSite api_sites[1024];static size_t api_site_count;
int s82_import_api_sites(const char *path){FILE*f=fopen(path,"rb");char magic[4];uint32_t n;if(!f)return 1;int bad=fread(magic,1,4,f)!=4||memcmp(magic,"S82I",4)||fread(&n,4,1,f)!=1||n>1024;if(!bad){api_site_count=n;bad=fread(api_sites,sizeof(ApiSite),n,f)!=n||fgetc(f)!=EOF;}fclose(f);return bad;}
void s82_named_api_guard(uint32_t pc){S80Runtime*r=s80_active_runtime();for(size_t i=0;i<api_site_count;i++){ApiSite*q=api_sites+i;if(q->site==pc){uint32_t v;if(!s80_read(r,q->slot,4,(uint8_t*)&v)||v!=q->thunk||(q->reg<8&&r->cpu.r[q->reg]!=v))s82_stop(S80_BINDING,q->slot);}}}
int s82_import_locks(const char *path){FILE*f=fopen(path,"rb");char magic[4];uint32_t n;if(!f)return 1;int bad=fread(magic,1,4,f)!=4||memcmp(magic,"S82L",4)||fread(&n,4,1,f)!=1||n>4096;if(!bad){lock_count=n;bad=fread(locks,sizeof(Lock),n,f)!=n||fgetc(f)!=EOF;}fclose(f);return bad;}
int s82_export_locks(const char *path){FILE*f=fopen(path,"wbx");if(!f)return 1;uint32_t n=(uint32_t)lock_count;int bad=fwrite("S82L",1,4,f)!=4||fwrite(&n,4,1,f)!=1||fwrite(locks,sizeof(Lock),n,f)!=n;if(fclose(f))bad=1;return bad;}
static void require(S80Runtime*r,uint32_t a,uint32_t n,unsigned access){if(s79_guard(&r->memory->view,a,n,access))s82_stop(S80_MEMORY,a);}
static void platform_write(S80Runtime*r,uint32_t a,uint32_t n,const uint8_t*b){require(r,a,n,2);if(r->pending||(uint64_t)(n+UINT64_C(3))/4>r->write_cap-r->writes)s82_stop(S80_BUDGET,a);for(uint32_t i=0;i<n;){size_t k=n-i<4?n-i:4;if(!s80_write(r,a+i,k,b+i))s82_stop(S80_MEMORY,a+i);i+=(uint32_t)k;}}
void *s82_api_dispatch(void *cpu,uint32_t id){
 S80Runtime*r=s80_active_runtime();if(!r||cpu!=&r->cpu||id<1||id>15)s82_stop(400,id);
 static const unsigned argc[16]={0,3,4,3,3,0,0,1,1,2,1,1,1,3,3,3};unsigned n=argc[id];uint32_t sp=r->cpu.r[4],a[4]={0},ret,value=r->cpu.r[0];require(r,sp,4+4*n,1);memcpy(&ret,(void*)(uintptr_t)(r->memory->offset+sp),4);if(n)memcpy(a,(void*)(uintptr_t)(r->memory->offset+sp+4),4*n);
 if(r->event_count>=r->event_cap)s82_stop(S80_BUDGET,id);r->api_id=id;int status=0;
 switch(id){
 case 1:status=s77_alloc(&r->heap,a[0],a[1],a[2],&value);break;
 case 2:status=s77_realloc(&r->heap,a[0],a[1],a[2],a[3],&value);break;
 case 3:status=s77_free(&r->heap,a[0],a[1],a[2]);value=1;break;
 case 4:status=s77_size(&r->heap,a[0],a[1],a[2],&value);break;
 case 5:value=s79_thread_id(&r->thread_set);break;
 case 6:value=s79_last_error(&r->thread_set);break;
 case 7:s79_set_error(&r->thread_set,a[0]);value=0;break;
 case 8:status=s79_tls_get(&r->thread_set,a[0],&value);break;
 case 9:status=s79_tls_set(&r->thread_set,a[0],a[1]);value=1;break;
 case 10:case 11:case 12:{
  size_t i=0;while(i<lock_count&&locks[i].address!=a[0])i++;uint32_t tid=s79_thread_id(&r->thread_set);
  if(id==10){if(i!=lock_count||lock_count==4096)s82_stop(410,a[0]);require(r,a[0],24,3);locks[lock_count++]=(Lock){a[0],0,0};}
  else {if(i==lock_count)s82_stop(411,a[0]);Lock*l=locks+i;if(id==11){if(l->owner&&l->owner!=tid)s82_stop(412,a[0]);if(l->depth==UINT32_MAX)s82_stop(413,a[0]);l->owner=tid;l->depth++;}else{if(l->owner!=tid||!l->depth)s82_stop(414,a[0]);if(!--l->depth)l->owner=0;}}
  break;}
 case 13:case 14:case 15:{
  uint32_t dst=a[0],src=a[1],len=a[2];if(len>64u*1024u*1024u)s82_stop(415,len);require(r,dst,len,2);
  if(id==13&&len&&(uint64_t)dst<src+(uint64_t)len&&(uint64_t)src<dst+(uint64_t)len)s82_stop(416,dst);
  if(id!=15)require(r,src,len,1);uint8_t*b=malloc(len?len:1);if(!b)s82_stop(417,len);if(id==15)memset(b,src&255,len);else if(len)memcpy(b,(void*)(uintptr_t)(r->memory->offset+src),len);platform_write(r,dst,len,b);free(b);value=dst;break;}
 default:s82_stop(400,id);
 }
 if(status)s82_stop(420+(uint32_t)status,a[0]);r->api_id=0;uint32_t *event=r->events[r->event_count++];event[0]=id;event[1]=r->pc;event[2]=n;memcpy(event+3,a,16);event[7]=value;event[8]=s79_last_error(&r->thread_set);event[9]=r->heap.next;event[10]=(uint32_t)s77_export_live(&r->heap,NULL,0);
 r->cpu.r[0]=value;r->cpu.r[4]=sp+4+(id>=13?0:4*n);return stage72_lookup_return(ret);
}
void s82_unimplemented(uint32_t id){s82_stop(500,id);}
