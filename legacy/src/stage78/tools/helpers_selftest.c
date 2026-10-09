#include "guarded_helpers.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {uint8_t data[128];unsigned reads,writes;int deny_read,deny_write;uint32_t base;
 uint32_t log_addr[128];unsigned log_size[128];uint32_t log_value[128];} Memory;
static int locate(Memory *m,uint32_t a,size_t n,size_t *offset){
 if(a<m->base||(uint64_t)a+n>(uint64_t)m->base+sizeof(m->data))return 0;
 *offset=a-m->base;return 1;
}
static int read_bytes(void *v,uint32_t a,size_t n,uint8_t *b){
 Memory *m=v;size_t o;m->reads++;if(m->deny_read>=0&&(int)m->reads==m->deny_read)return 0;
 if(!locate(m,a,n,&o))return 0;memcpy(b,m->data+o,n);return 1;
}
static int write_bytes(void *v,uint32_t a,size_t n,const uint8_t *b){
 Memory *m=v;size_t o;if(m->deny_write>=0&&(int)m->writes+1==m->deny_write)return 0;
 if(!locate(m,a,n,&o))return 0;memcpy(m->data+o,b,n);
 unsigned k=m->writes++;assert(k<128);m->log_addr[k]=a;m->log_size[k]=(unsigned)n;m->log_value[k]=0;
 for(unsigned i=0;i<n;i++)m->log_value[k]|=(uint32_t)b[i]<<(8*i);return 1;
}
static Memory memory(void){Memory m={0};m.base=0x1000;m.deny_read=m.deny_write=-1;for(unsigned i=0;i<128;i++)m.data[i]=(uint8_t)i;return m;}
static S78CPU cpu(uint32_t n,uint32_t s,uint32_t d,uint32_t f){S78CPU c;memset(&c,0x35,sizeof(c));c.r[S78_ECX]=n;c.r[S78_ESI]=s;c.r[S78_EDI]=d;c.r[S78_FLAGS]=f;return c;}
static S78Context context(Memory *m){S78Context c={read_bytes,write_bytes,m,100,0x1000,127,0x1000,1,1,1};return c;}
static unsigned tests;
static void passed(const char *name){printf("PASS %s\n",name);tests++;}
int main(void){uint64_t lo,hi;Memory m=memory();S78Context x=context(&m);S78CPU c=cpu(0,0xffffffff,0xffffffff,0xa57),before=c;
 assert(s78_rep(&c,&x,0,4).status==0&&!memcmp(&c,&before,sizeof(c))&&!m.reads&&!m.writes);passed("zero-count-no-access-preserves-all-state");
 assert(s78_span(0x1010,3,4,1,&lo,&hi)==0&&lo==0x1008&&hi==0x1014);
 assert(s78_span(0xfffffffc,1,4,0,&lo,&hi)==0&&hi==UINT64_C(0x100000000));
 assert(s78_span(0xfffffffc,2,4,0,&lo,&hi)==S78_ADDRESS_BOUNDARY);
 assert(s78_span(0,2,4,1,&lo,&hi)==S78_ADDRESS_BOUNDARY);passed("full-span-direction-overflow-last-element");
 m=memory();x=context(&m);c=cpu(3,0x1001,0x1041,0x202);before=c;
 assert(s78_rep(&c,&x,0,4).completed==3&&c.r[S78_ECX]==0&&c.r[S78_ESI]==0x100d&&c.r[S78_EDI]==0x104d);
 assert(!memcmp(m.data+1,m.data+65,12)&&m.writes==3&&m.log_addr[1]==0x1045&&m.log_size[1]==4);
 before.r[S78_ECX]=c.r[S78_ECX];before.r[S78_ESI]=c.r[S78_ESI];before.r[S78_EDI]=c.r[S78_EDI];assert(!memcmp(&c,&before,sizeof(c)));passed("unaligned-movsd-ordered-elements-opaque-state");
 m=memory();x=context(&m);c=cpu(3,0x1008,0x1048,0x602);assert(s78_rep(&c,&x,0,4).completed==3&&c.r[S78_ESI]==0xffc&&m.log_addr[2]==0x1040);passed("backward-movsd");
 m=memory();x=context(&m);c=cpu(6,0x1000,0x1001,0x202);assert(!s78_rep(&c,&x,0,1).status);for(unsigned i=0;i<=6;i++)assert(m.data[i]==0);passed("overlap-is-sequential-not-memmove");
 m=memory();x=context(&m);m.deny_read=3;c=cpu(4,0x1000,0x1040,0x202);assert(s78_rep(&c,&x,0,4).status==S78_READ_BOUNDARY&&c.r[S78_ECX]==2&&c.r[S78_ESI]==0x1008&&m.writes==2);passed("read-failure-commits-completed-prefix");
 m=memory();x=context(&m);m.deny_write=2;c=cpu(4,0x1000,0x1040,0x202);assert(s78_rep(&c,&x,0,4).status==S78_WRITE_BOUNDARY&&c.r[S78_ECX]==3&&c.r[S78_EDI]==0x1044&&m.writes==1&&m.reads==2);passed("write-failure-no-progress-for-failed-element");
 m=memory();x=context(&m);c=cpu(2,0,0x1041,0x202);c.r[S78_EAX]=0x89abcdef;assert(!s78_rep(&c,&x,1,4).status&&m.log_value[0]==0x89abcdef&&m.data[65]==0xef);passed("stosd-little-endian-and-write-log");
 m=memory();x=context(&m);c=cpu(500,0,0x107f,0x202);c.r[S78_EAX]=127;assert(s78_rep(&c,&x,2,1).completed==1&&(c.r[S78_FLAGS]&64)&&c.r[S78_ECX]==499);passed("scas-early-match-before-inaccessible-tail");
 m=memory();x=context(&m);c=cpu(3,0,0x1000,0x202);c.r[S78_EAX]=0xff;assert(s78_rep(&c,&x,2,1).completed==3&&!(c.r[S78_FLAGS]&64));passed("scas-exhaustion-updates-arithmetic-flags");
 m=memory();x=context(&m);m.deny_read=3;c=cpu(4,0,0x1000,0xa57);c.r[S78_EAX]=0xff;assert(s78_rep(&c,&x,2,1).status==S78_READ_BOUNDARY&&c.r[S78_FLAGS]==0xa57&&c.r[S78_EDI]==0x1002&&c.r[S78_ECX]==2);passed("scas-fault-restores-entry-flags-keeps-prefix");
 m=memory();x=context(&m);x.budget=2;c=cpu(3,0,0x1040,0x202);assert(s78_rep(&c,&x,1,4).status==S78_BUDGET_BOUNDARY&&c.r[S78_ECX]==1&&m.writes==2);passed("budget-stop-explicit-not-success");
 m=memory();m.base=0xffffff80u;x=context(&m);c=cpu(1,0,0xfffffffcu,0x202);assert(!s78_rep(&c,&x,1,4).status&&c.r[S78_EDI]==0);passed("final-index-wrap-after-valid-last-element");
 m=memory();x=context(&m);uint32_t v;assert(!s78_fs_write(&x,0,0x70001000)&&!s78_fs_read(&x,0,&v)&&v==0x70001000);x.fs_base=0x1040;x.active_teb=0x1040;x.fs_limit=63;assert(!s78_fs_write(&x,0,0xffffffff));x.fs_base=0x1000;x.active_teb=0x1000;x.fs_limit=127;assert(!s78_fs_read(&x,0,&v)&&v==0x70001000);passed("FS-per-thread-SEH-chain-bookkeeping");
 x.fs_cache_proved=0;assert(s78_fs_read(&x,0,&v)==S78_SEGMENT_UNKNOWN);x.fs_cache_proved=1;x.active_teb=0;assert(s78_fs_read(&x,0,&v)==S78_SEGMENT_UNKNOWN);x.active_teb=x.fs_base;assert(s78_fs_read(&x,126,&v)==S78_ADDRESS_BOUNDARY);passed("FS-unknown-cache-thread-or-limit-refuses");
 const uint32_t t[]={0x765100,0x765200};const S78Domain domains[]={{0x765000,t,2,1},{0x765010,t,2,0}};
 assert(s78_target(domains,2,0x765000,0x765100)==0);assert(s78_target(domains,2,0x765000,0x765101)==S78_TARGET_DENIED);assert(s78_target(domains,2,0x765010,0x765100)==S78_DOMAIN_UNKNOWN);assert(s78_target(domains,2,0x765011,0x765100)==S78_DOMAIN_UNKNOWN);passed("dynamic-unknown-and-middle-instruction-target-denied");
 for(unsigned a=0;a<256;a++)for(unsigned b=0;b<256;b++){
  m=memory();x=context(&m);m.data[0]=(uint8_t)b;c=cpu(1,0,0x1000,0xa02);c.r[S78_EAX]=a;
  assert(!s78_rep(&c,&x,2,1).status);unsigned value=(a+256-b)&255,ones=0;
  for(unsigned z=value;z;z>>=1)ones+=z&1;
  int signed_a=a<128?(int)a:(int)a-256,signed_b=b<128?(int)b:(int)b-256,difference=signed_a-signed_b;
  unsigned expected=(0xa02u&~0x8d5u)|(a<b?1:0)|(ones%2==0?4:0)|((a%16)<(b%16)?16:0)|(!value?64:0)|(value>=128?128:0)|(difference>127||difference< -128?0x800:0);
  assert(c.r[S78_FLAGS]==expected);
 }
 passed("all-65536-byte-comparisons-independent-flag-formula");
 printf("%u authored tests passed; zero game/Unicorn/SR-library execution\n",tests);return 0;
}
