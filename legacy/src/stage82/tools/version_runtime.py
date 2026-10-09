"""Create new Stage82 runtime source versions; never mutate Stage81."""
import sys
sys.dont_write_bytecode=True
from common import ROOT,S
def main():
 h=(ROOT/'stage81/tools/runtime.h').read_text().replace('uint32_t pc,address,size,value;','uint32_t kind,pc,address,size,value,api;')
 h=h.replace('jmp_buf escape;','uint32_t mode,api_id,complete_pc,complete_sp,completed;void *complete_target;const uint32_t *allowed;size_t allowed_count;double deadline;uint32_t (*events)[11];size_t event_count,event_cap;jmp_buf escape;')
 h=h.replace('#endif','S80Runtime *s80_active_runtime(void);\nvoid s82_stop(uint32_t,uint32_t) __attribute__((noreturn));\nvoid s82_complete(void) __attribute__((noreturn));\nvoid *s82_api_dispatch(void *,uint32_t);\nvoid s82_named_api_guard(uint32_t);\n#endif')
 (S/'tools/runtime.h').write_text(h)
 c=(ROOT/'stage81/tools/runtime.c').read_text().replace('#include <string.h>','#include <string.h>\n#include <time.h>')
 c=c.replace('(S80Write){r->pc,a,(uint32_t)n,v}','(S80Write){r->api_id?1u:0u,r->pc,a,(uint32_t)n,v,r->api_id}')
 c=c.replace('(S80Write){active->pc,a,n,actual}','(S80Write){0,active->pc,a,n,actual,0}')
 old='if(!active||!active->authored_execution||pc<0xa0000000||pc>=0xa0001000)abort();'
 new='if(!active||!active->armed)abort();if(active->mode==1){size_t lo=0,hi=active->allowed_count;while(lo<hi){size_t mid=lo+(hi-lo)/2;if(active->allowed[mid]<pc)lo=mid+1;else hi=mid;}if(lo==active->allowed_count||active->allowed[lo]!=pc)stop(S80_NO_DRIVER,pc);}else if(active->mode!=2||pc<0xa0000000||pc>=0xa0001000)stop(S80_NO_DRIVER,pc);struct timespec now;clock_gettime(CLOCK_MONOTONIC,&now);if(now.tv_sec+now.tv_nsec/1e9>active->deadline)stop(S80_BUDGET,pc);'
 assert old in c;c=c.replace(old,new);c=c.replace('active->pc=pc;active->steps++;','if(active->mode==1)s82_named_api_guard(pc);active->pc=pc;active->steps++;')
 c=c.replace('void *stage72_lookup_return(uint32_t pc){if(!active)abort();','void *stage72_lookup_return(uint32_t pc){if(!active)abort();if(pc==active->complete_pc)return active->complete_target;')
 c+='\nvoid s82_stop(uint32_t code,uint32_t address){stop(code,address);}\nvoid s82_complete(void){if(!active||active->pending||active->cpu.r[4]!=active->complete_sp)stop(S80_BINDING,active?active->cpu.r[4]:0);active->pc=active->complete_pc;active->completed=1;stop(0,0);}\n'
 (S/'tools/runtime.c').write_text(c)
 (S/'tools/heap_connection.c').write_text((ROOT/'stage81/tools/heap_connection.c').read_text())
 (S/'tools/domains.c').write_text((ROOT/'stage81/tools/domains_extended.c').read_text())
 print('New version only: trace kind/API attribution, closed-default modes, allowlist/deadline, genuine-return completion')
if __name__=='__main__':main()
