import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];p=ROOT/'stage79/evidence/call-domains.json';d=json.loads(p.read_text());registers={'EAX':0,'ECX':1,'EDX':2,'EBX':3}
rows=[]
for c in d['conditional_singleton_site_certificates']:
 for x in c['cases']:rows.append('{%du,%du,%du,%du,%du}'%(c['site'],registers[c['vptr_register']],x['vtable'],x['slot'],x['target']))
text='''/* Generated from frozen conditional certificates; no universal domain. */
#include "runtime.h"
#include <string.h>
extern S80Runtime *s80_active_runtime(void);
extern void s80_domain_stop(void) __attribute__((noreturn));
extern void *s80_bound_target(uint32_t);
typedef struct {uint32_t site,reg,vtable,slot,target;} Case;
static const Case cases[]={%s};
void *stage77_lookup_target(uint32_t site,uint32_t target){
 S80Runtime *r=s80_active_runtime();if(!r||site!=r->pc)s80_domain_stop();
 for(size_t i=0;i<sizeof(cases)/sizeof(*cases);i++){
  const Case *c=cases+i;if(c->site!=r->pc||r->cpu.r[c->reg]!=c->vtable)continue;
  if(s79_guard(&r->memory->view,c->slot,4,1)==0&&s79_guard(&r->memory->view,c->slot,4,2)==S79_PERMISSION){
   uint32_t actual=0;if(s80_read(r,c->slot,4,(uint8_t *)&actual)&&actual==c->target&&target==actual)return s80_bound_target(target);
  }
  s80_domain_stop();
 }
 s80_domain_stop();
}
'''%(',\n'.join(rows))
(ROOT/'stage80/tools/domains.c').write_text(text)
(ROOT/'stage80/evidence/domain-generation.json').write_text(json.dumps(dict(source_sha256=hashlib.sha256(p.read_bytes()).hexdigest(),cases=len(rows),sites=len(d['conditional_singleton_site_certificates']),universal_domains=0),indent=2)+'\n')
print('21 conditional cases / 6 sites generated; no universal target admission')
