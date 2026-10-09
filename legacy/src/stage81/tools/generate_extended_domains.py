import hashlib,json,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage81';sys.path.insert(0,str(ROOT/'samples'))
from auto_i386_analysis import PE
pe=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe');assert hashlib.sha256(pe.data).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
nodes={q['pc']:q for q in json.loads((ROOT/'stage79/evidence/known-method-completed-closure.json').read_text())['instructions']};regs={'EAX':0,'ECX':1,'EDX':2,'EBX':3};rows=[]
for c in json.loads((ROOT/'stage79/evidence/call-domains.json').read_text())['conditional_singleton_site_certificates']:
 for q in c['cases']:rows.append(dict(site=c['site'],reg=regs[c['vptr_register']],value=q['vtable'],slot=q['slot'],target=q['target'],kind='vtable'))
crt=json.loads((ROOT/'stage75/configs/crt-jump-domains.json').read_text())['domains']
for site,c in crt.items():
 pc=int(site,0);assert nodes[pc]['bytes']==c['original_instruction_bytes'];reg=c['index_register'].upper();table=int(c['table'],0)
 for q in c['rows']:
  index=int(q['wrapped_index'],0);slot=int(q['slot'],0);target=int(q['target'],0);assert(slot==(table+4*index)&0xffffffff)and pe.read(slot,4)==target.to_bytes(4,'little')and not pe.section(slot)['flags']&0x80000000
  rows.append(dict(site=pc,reg=regs[reg],value=index,slot=slot,target=target,kind='CRT_index'))
body='''/* Generated conditional operand/readonly-slot certificates. Unknown stops. */
#include "runtime.h"
extern S80Runtime *s80_active_runtime(void);
extern void s80_domain_stop(void) __attribute__((noreturn));
extern void *s80_bound_target(uint32_t);
const S81DomainCase stage81_domain_cases[]={%s};
const size_t stage81_domain_case_count=sizeof(stage81_domain_cases)/sizeof(*stage81_domain_cases);
void *stage77_lookup_target(uint32_t site,uint32_t target){
 S80Runtime *r=s80_active_runtime();if(!r||site!=r->pc)s80_domain_stop();
 for(size_t i=0;i<stage81_domain_case_count;i++){
  const S81DomainCase *c=stage81_domain_cases+i;if(c->site!=site||r->cpu.r[c->reg]!=c->value)continue;
  uint32_t actual;
  if(s79_guard(&r->memory->view,c->slot,4,1)==0&&s79_guard(&r->memory->view,c->slot,4,2)==S79_PERMISSION&&s80_read(r,c->slot,4,(uint8_t *)&actual)&&actual==c->target&&target==actual)return s80_bound_target(target);
  s80_domain_stop();
 }
 s80_domain_stop();
}
'''%(',\n'.join('{%du,%du,%du,%du,%du}'%(r['site'],r['reg'],r['value'],r['slot'],r['target'])for r in rows))
(S/'tools/domains_extended.c').write_text(body);e=dict(total_conditional_cases=len(rows),conditional_sites=len(set(r['site']for r in rows)),CRT_conditional_JMP_sites=32,CRT_table_cases=len(rows)-21,vtable_cases=21,universal_complete_dynamic_domains=0,proof='runtime exact index/vptr, readonly current slot bytes, requested target and native binding; CRT reviewed CFG ranges are not promoted to universal input proof',rows=rows)
(S/'evidence/extended-domains.json').write_text(json.dumps(e,indent=2)+'\n');print({k:v for k,v in e.items()if k!='rows'})
