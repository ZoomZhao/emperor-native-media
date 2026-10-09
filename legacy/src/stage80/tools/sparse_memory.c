/* Own guest backing allocator: never overwrites existing host mappings. */
#include "sparse_memory.h"
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
static int cmp64(const void *a,const void *b){uint64_t x=*(const uint64_t *)a,y=*(const uint64_t *)b;return(x>y)-(x<y);}
void s80_sparse_close(S80Sparse *m){for(size_t i=0;i<m->backing_count;i++)mach_vm_deallocate(mach_task_self(),m->offset+m->backing[i].low,m->backing[i].end-m->backing[i].low);free(m->view.regions);memset(m,0,sizeof(*m));}
int s80_sparse_open(S80Sparse *m,const S79Region *r,size_t count,const char *path,uint64_t offset){
 memset(m,0,sizeof(*m));long page=sysconf(_SC_PAGESIZE);if(page<4096||!count||count>64||offset%(uint64_t)page||offset>UINT64_MAX-UINT64_C(0x100000000))return S79_INVALID_PLAN;
 uint64_t ends[128],previous=0;size_t n=0;
 for(size_t i=0;i<count;i++){
  if(r[i].low%4096||r[i].end%4096||r[i].end<=r[i].low||r[i].end>UINT64_C(0x100000000)||r[i].low<previous||!r[i].permissions||r[i].permissions&~7u)return S79_INVALID_PLAN;
  previous=r[i].end;ends[n++]=r[i].low&~((uint64_t)page-1);ends[n++]=(r[i].end+(uint64_t)page-1)&~((uint64_t)page-1);
 }
 m->offset=offset;m->view.base=(uint8_t *)(uintptr_t)offset;m->view.count=count;m->view.regions=malloc(count*sizeof(*r));if(!m->view.regions)goto fail;memcpy(m->view.regions,r,count*sizeof(*r));qsort(ends,n,sizeof(*ends),cmp64);
 /* Build union backing runs. mach_vm_allocate with fixed address and WITHOUT
  * VM_FLAGS_OVERWRITE returns failure on existing mappings; it cannot clobber
  * text, libraries, heap or another region. Never use MAP_FIXED here. */
 for(size_t k=1;k<n;k++){
  uint64_t low=ends[k-1],high=ends[k];if(low==high)continue;int covered=0;
  for(size_t i=0;i<count;i++)if((r[i].low&~((uint64_t)page-1))<high&&low<((r[i].end+(uint64_t)page-1)&~((uint64_t)page-1))){covered=1;break;}
  if(!covered)continue;
  if(m->backing_count&&m->backing[m->backing_count-1].end==low)m->backing[m->backing_count-1].end=high;
  else m->backing[m->backing_count++]=(S80Backing){low,high};
 }
 size_t planned=m->backing_count;m->backing_count=0;
 for(size_t i=0;i<planned;i++){
  S80Backing b=m->backing[i];mach_vm_address_t native=offset+b.low;mach_vm_size_t size=b.end-b.low;
  kern_return_t kr=mach_vm_allocate(mach_task_self(),&native,size,VM_FLAGS_FIXED);
  if(kr!=KERN_SUCCESS||native!=offset+b.low){fprintf(stderr,"S80 fixed allocation refused: guest=[%llx,%llx) native=%llx size=%llx kern=%d\n",(unsigned long long)b.low,(unsigned long long)b.end,(unsigned long long)(offset+b.low),(unsigned long long)size,kr);if(kr==KERN_SUCCESS)mach_vm_deallocate(mach_task_self(),native,size);goto fail;}
  m->backing_count++;
 }
 FILE *f=path?fopen(path,"rb"):NULL;if(path&&!f)goto fail;
 for(size_t i=0;i<count;i++)if(f){if(fseeko(f,(off_t)r[i].offset,SEEK_SET)||fread((void *)(uintptr_t)(offset+r[i].low),1,(size_t)(r[i].end-r[i].low),f)!=(size_t)(r[i].end-r[i].low)){fclose(f);goto fail;}}
 if(f)fclose(f);
 for(size_t k=1;k<n;k++){
  uint64_t low=ends[k-1],high=ends[k];if(low==high)continue;unsigned permissions=0;
  for(size_t i=0;i<count;i++)if((r[i].low&~((uint64_t)page-1))<high&&low<((r[i].end+(uint64_t)page-1)&~((uint64_t)page-1)))permissions|=r[i].permissions&3u;
  if(!permissions)continue;int native=(permissions&1?PROT_READ:0)|(permissions&2?PROT_WRITE:0);
  if(mprotect((void *)(uintptr_t)(offset+low),(size_t)(high-low),native))goto fail;
 }
 return S79_OK;
 fail:s80_sparse_close(m);return S79_IO;
}
