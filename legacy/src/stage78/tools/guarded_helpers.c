#include "guarded_helpers.h"
#include <string.h>
#define DF 0x400u
#define ARITH 0x8d5u
int s78_span(uint32_t p,uint32_t n,unsigned w,int back,uint64_t *lo,uint64_t *hi){
 if(w!=1&&w!=4)return S78_ADDRESS_BOUNDARY;
 if(!n){*lo=*hi=p;return S78_OK;}
 uint64_t distance=(uint64_t)(n-1)*w;
 if(back){if(distance>p)return S78_ADDRESS_BOUNDARY;*lo=p-distance;*hi=(uint64_t)p+w;}
 else{*lo=p;*hi=(uint64_t)p+(uint64_t)n*w;}
 return *hi>(UINT64_C(1)<<32)?S78_ADDRESS_BOUNDARY:S78_OK;
}
static uint32_t cmp8(uint32_t flags,uint8_t a,uint8_t b){
 uint8_t v=(uint8_t)(a-b);unsigned parity=0;
 for(unsigned i=0;i<8;i++)parity^=(v>>i)&1;
 return (flags&~ARITH)|(a<b?1:0)|(!parity?4:0)|((a^b^v)&16)|
  (!v?64:0)|(v&128)|(((a^b)&(a^v)&128)?0x800:0);
}
S78Result s78_rep(S78CPU *cpu,S78Context *ctx,int op,unsigned w){
 S78Result out={S78_OK,0,0};uint32_t initial=cpu->r[S78_FLAGS];
 if(!cpu->r[S78_ECX])return out;
 if(!ctx->flat_DS_ES_proved){out.status=S78_SEGMENT_UNKNOWN;return out;}
 if((op!=0&&op!=1&&op!=2)||(w!=1&&w!=4)||(op==2&&w!=1)){
  out.status=S78_ADDRESS_BOUNDARY;return out;}
 /* SCAS may terminate before an inaccessible later byte. Never reject it
  * because the hypothetical whole-count span would cross a boundary. */
 while(cpu->r[S78_ECX]){
  if(out.completed==ctx->budget){out.status=S78_BUDGET_BOUNDARY;break;}
  uint32_t src=cpu->r[S78_ESI],dst=cpu->r[S78_EDI];uint8_t bytes[4];
  if((uint64_t)dst+w>(UINT64_C(1)<<32)||op==0&&(uint64_t)src+w>(UINT64_C(1)<<32)){
   out.status=S78_ADDRESS_BOUNDARY;break;}
  if(op==0){if(!ctx->read(ctx->opaque,src,w,bytes)){out.status=S78_READ_BOUNDARY;break;}}
  else if(op==1){uint32_t v=cpu->r[S78_EAX];for(unsigned i=0;i<w;i++)bytes[i]=(uint8_t)(v>>(8*i));}
  else if(!ctx->read(ctx->opaque,dst,1,bytes)){out.status=S78_READ_BOUNDARY;break;}
  if(op!=2&&!ctx->write(ctx->opaque,dst,w,bytes)){out.status=S78_WRITE_BOUNDARY;break;}
  uint32_t delta=(initial&DF)?(uint32_t)(0u-w):w;
  if(op==0)cpu->r[S78_ESI]+=delta;
  cpu->r[S78_EDI]+=delta;cpu->r[S78_ECX]--;out.completed++;
  if(op==2){out.last_byte=bytes[0];cpu->r[S78_FLAGS]=cmp8(initial,(uint8_t)cpu->r[S78_EAX],bytes[0]);
   if((uint8_t)cpu->r[S78_EAX]==bytes[0])break;}
 }
 /* Fault boundary semantics from Intel REP/SCAS. A resource-budget stop is
  * an explicit tool stop rather than an emulated architectural exception. */
 if(op==2&&out.status!=S78_OK&&out.status!=S78_BUDGET_BOUNDARY)cpu->r[S78_FLAGS]=initial;
 return out;
}
static int fs_address(S78Context *c,uint32_t offset,uint32_t *a){
 if(!c->fs_cache_proved||c->active_teb!=c->fs_base)return S78_SEGMENT_UNKNOWN;
 if((uint64_t)offset+3>c->fs_limit||(uint64_t)c->fs_base+offset+4>(UINT64_C(1)<<32))return S78_ADDRESS_BOUNDARY;
 *a=c->fs_base+offset;return S78_OK;
}
int s78_fs_read(S78Context *c,uint32_t o,uint32_t *v){
 uint32_t a;int s=fs_address(c,o,&a);if(s)return s;uint8_t b[4];
 if(!c->read(c->opaque,a,4,b))return S78_READ_BOUNDARY;
 *v=(uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;return S78_OK;
}
int s78_fs_write(S78Context *c,uint32_t o,uint32_t v){
 uint32_t a;int s=fs_address(c,o,&a);if(s)return s;if(!c->fs_writable)return S78_WRITE_BOUNDARY;
 uint8_t b[4];for(unsigned i=0;i<4;i++)b[i]=(uint8_t)(v>>(8*i));
 return c->write(c->opaque,a,4,b)?S78_OK:S78_WRITE_BOUNDARY;
}
int s78_target(const S78Domain *d,size_t n,uint32_t site,uint32_t target){
 for(size_t i=0;i<n;i++)if(d[i].site==site){
  if(!d[i].proved)return S78_DOMAIN_UNKNOWN;
  for(size_t j=0;j<d[i].count;j++)if(d[i].targets[j]==target)return S78_OK;
  return S78_TARGET_DENIED;
 }
 return S78_DOMAIN_UNKNOWN;
}
