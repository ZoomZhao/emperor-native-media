#include "guest_memory.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#define SPACE (UINT64_C(1)<<32)
static int compare64(const void *a,const void *b){uint64_t x=*(const uint64_t *)a,y=*(const uint64_t *)b;return(x>y)-(x<y);}
int s79_guard(const S79Memory *m,uint32_t a,uint64_t n,unsigned permission){
 if(m->poisoned||!m->base)return S79_UNMAPPED;
 if(!n)return S79_OK;
 if(!permission||permission&~3u||n>SPACE-(uint64_t)a)return S79_OVERFLOW;
 uint64_t cursor=a,end=cursor+n;
 for(size_t i=0;i<m->count&&cursor<end;i++){
  const S79Region *r=m->regions+i;
  if(cursor<r->low)return S79_UNMAPPED;
  if(cursor>=r->end)continue;
  if((r->permissions&permission)!=permission)return S79_PERMISSION;
  cursor=r->end<end?r->end:end;
 }
 return cursor==end?S79_OK:S79_UNMAPPED;
}
int s79_host_to_guest(const S79Memory *m,const void *p,uint32_t *a){
 uintptr_t raw=(uintptr_t)p,base=(uintptr_t)m->base;
 if(!m->base||raw<base||(uint64_t)(raw-base)>=SPACE)return S79_OVERFLOW;
 *a=(uint32_t)(raw-base);return S79_OK;
}
int s79_read(void *v,uint32_t a,size_t n,uint8_t *out){
 S79Memory *m=v;if(s79_guard(m,a,n,1))return 0;
 if(n)memcpy(out,m->base+a,n);return 1;
}
int s79_write(void *v,uint32_t a,size_t n,const uint8_t *in){
 S79Memory *m=v;if(s79_guard(m,a,n,2))return 0;
 if(n){memcpy(m->base+a,in,n);m->writes++;}return 1;
}
void s79_memory_close(S79Memory *m){
 if(m->base)munmap(m->base,(size_t)SPACE);
 free(m->regions);memset(m,0,sizeof(*m));
}
int s79_memory_open(S79Memory *m,const S79Region *regions,size_t count,const char *path){
 memset(m,0,sizeof(*m));if(!count||count>64)return S79_INVALID_PLAN;
 uint64_t last=0;
 for(size_t i=0;i<count;i++){
  const S79Region *r=regions+i;
  if(r->low%4096||r->end%4096||r->end<=r->low||r->end>SPACE||r->low<last||!r->permissions||r->permissions&~7u)return S79_INVALID_PLAN;
  last=r->end;
 }
 m->regions=malloc(count*sizeof(*regions));if(!m->regions)return S79_IO;
 memcpy(m->regions,regions,count*sizeof(*regions));m->count=count;
 m->base=mmap(NULL,(size_t)SPACE,PROT_NONE,MAP_PRIVATE|MAP_ANON,-1,0);
 if(m->base==MAP_FAILED){m->base=NULL;s79_memory_close(m);return S79_IO;}
 FILE *f=path?fopen(path,"rb"):NULL;if(path&&!f){s79_memory_close(m);return S79_IO;}
 long native_page=sysconf(_SC_PAGESIZE);if(native_page<4096||(native_page&(native_page-1)))goto fail;
 uint64_t boundaries[128];size_t used=0;
 for(size_t i=0;i<count;i++){
  const S79Region *r=regions+i;size_t n=(size_t)(r->end-r->low);uint8_t *p=m->base+r->low;
  uint64_t low=r->low&~((uint64_t)native_page-1),end=(r->end+(uint64_t)native_page-1)&~((uint64_t)native_page-1);
  boundaries[used++]=low;boundaries[used++]=end;
  if(mprotect(m->base+low,(size_t)(end-low),PROT_READ|PROT_WRITE))goto fail;
  if(f){if(fseeko(f,(off_t)r->offset,SEEK_SET)||fread(p,1,n,f)!=n)goto fail;}
 }
 /* macOS arm64 uses 16KiB host pages. Exact 4KiB guest permissions/holes are
  * enforced by s79_guard. Host backing gets the UNION of overlapping guest
  * fragments' R/W permissions, with X always absent, rather than incorrectly
  * mprotect'ing 4KiB guest pages or making one fragment revoke its neighbor. */
 qsort(boundaries,used,sizeof(*boundaries),compare64);
 for(size_t k=1;k<used;k++){
  uint64_t low=boundaries[k-1],end=boundaries[k];if(low==end)continue;unsigned permissions=0;
  for(size_t i=0;i<count;i++){
   uint64_t first=regions[i].low&~((uint64_t)native_page-1),last=(regions[i].end+(uint64_t)native_page-1)&~((uint64_t)native_page-1);
   if(first<end&&low<last)permissions|=regions[i].permissions&3u;
  }
  int flags=(permissions&1?PROT_READ:0)|(permissions&2?PROT_WRITE:0);
  if(mprotect(m->base+low,(size_t)(end-low),flags))goto fail;
 }
 if(f)fclose(f);return S79_OK;
 fail:if(f)fclose(f);s79_memory_close(m);return S79_IO;
}
