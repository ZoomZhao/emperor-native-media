#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
static int check(void *p,uint32_t a,uint32_t n,unsigned access){S80Runtime *r=p;return s79_guard(&r->memory->view,a,n,access)==S79_OK;}
static int map(void *p,uint32_t start,uint32_t end){
 S80Runtime *r=p;S79Memory *m=&r->memory->view;
 if(start!=r->heap.mapped_end||start<0x10000000||end>0x20000000||end<=start||(start&0xffffff)||(end&0xffffff)||m->count>=64)return 0;
 for(size_t i=0;i<m->count;i++)if(start<m->regions[i].end&&m->regions[i].low<end)return 0;
 S79Region *regions=malloc((m->count+1)*sizeof(*regions));if(!regions)return 0;size_t at=0;while(at<m->count&&m->regions[at].low<start)at++;
 memcpy(regions,m->regions,at*sizeof(*regions));regions[at]=(S79Region){start,3,end,UINT64_MAX};memcpy(regions+at+1,m->regions+at,(m->count-at)*sizeof(*regions));
 /* Whole private arena already reserved: new heap chunk gets R/W, never X.
  * Caller imports only after successful allocation; no guest VA is moved. */
 if(mprotect((void *)(uintptr_t)(r->memory->offset+start),(size_t)(end-start),PROT_READ|PROT_WRITE)){free(regions);return 0;}
 free(m->regions);m->regions=regions;m->count++;return 1;
}
static int reserve(S80Runtime *r,uint32_t a,uint32_t n){return !r->pending&&r->writes<=r->write_cap&&check(r,a,n,2)&&((uint64_t)n+3)/4<=r->write_cap-r->writes;}
static int fill(void *p,uint32_t a,uint32_t n,unsigned char v){
 S80Runtime *r=p;if(!reserve(r,a,n))return 0;uint8_t b[4]={v,v,v,v};
 for(uint32_t i=0;i<n;){size_t k=n-i<4?n-i:4;if(!s80_write(r,a+i,k,b))abort();i+=(uint32_t)k;}return 1;
}
static int copy(void *p,uint32_t a,uint32_t b,uint32_t n){
 S80Runtime *r=p;if(!reserve(r,a,n)||!check(r,b,n,1))return 0;
 /* Bump-policy fresh target never overlaps old live allocation. Refuse an
  * unexpected overlapping copy rather than guessing intra-copy semantics. */
 if(n&&a<(uint64_t)b+n&&b<(uint64_t)a+n)return 0;
 for(uint32_t i=0;i<n;){uint8_t x[4];size_t k=n-i<4?n-i:4;if(!s80_read(r,b+i,k,x)||!s80_write(r,a+i,k,x))abort();i+=(uint32_t)k;}return 1;
}
static int get(FILE *f,uint32_t *p){return fread(p,4,1,f)==1;}
int s81_import_heap(S80Runtime *r,const char *path){
 FILE *f=fopen(path,"rb");if(!f)return S77_METADATA;char magic[4];uint32_t version,nh,nr,na,next,mapped;S77Heap *heaps=NULL;S77Allocation *rows=NULL;int q=S77_METADATA;
 if(fread(magic,1,4,f)!=4||memcmp(magic,"S77H",4)||!get(f,&version)||version!=1||!get(f,&nh)||nh>4096||!get(f,&nr)||nr>64||!get(f,&na)||na>1000000||!get(f,&next)||!get(f,&mapped))goto done;
 heaps=calloc(nh,sizeof(*heaps));rows=calloc(na,sizeof(*rows));if(!heaps||!rows)goto done;
 for(unsigned i=0;i<nh;i++)if(!get(f,&heaps[i].handle)||!get(f,&heaps[i].options))goto done;
 for(unsigned i=0;i<nr;i++){uint32_t low,end,permission;if(!get(f,&low)||!get(f,&end)||!get(f,&permission)||end<low||!check(r,low,end-low,permission&3))goto done;}
 for(unsigned i=0;i<na;i++){if(!get(f,&rows[i].pointer)||!get(f,&rows[i].heap)||!get(f,&rows[i].size))goto done;rows[i].live=1;}
 if(fgetc(f)!=EOF)goto done;q=s77_import(&r->heap,heaps,nh,rows,na,next,mapped,(S77Memory){r,check,map,fill,copy});
 done:free(heaps);free(rows);fclose(f);return q;
}
