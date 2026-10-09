"""Mechanism grouping and exact conditional domains, without input pruning."""
import hashlib,json,re,struct,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'samples'))
from auto_i386_analysis import PE
def generate():
 c=json.loads((ROOT/'stage76/evidence/full-dependency-closure-v1.json').read_text());nodes={q['pc']:q for q in c['instructions']};pe=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe');assert hashlib.sha256(pe.data).hexdigest()==c['original_sha256'];prior=json.loads((ROOT/'stage76/evidence/full-audit.json').read_text())['expanded_unresolved_non_IAT_CALL_sites'];new={r['PC']for r in json.loads((ROOT/'stage78/evidence/dynamic-domains.json').read_text())['register_IAT_sources'] if r['all_CFG_paths_have_single_IAT_source']};unknown=[q for q in prior if q['PC']not in new];assert len(unknown)==25
 business=[];crt=[];loader=[]
 crt_slots={'0x7689de':0x88aa00,'0x7703ad':0x1e58524,'0x77158e':0x1e58630,'0x77bb25':0x88a3d4};arrays={'0x76a026':'reverse exit-callback array from [0x1e58f50]/[0x1e58f54]','0x76a097':'forward init/exit array passed by caller'};dynamic={'0x774c9a':('GetActiveWindow',0x1e586ac),'0x774cac':('GetLastActivePopup',0x1e586b0),'0x774cbd':('MessageBoxA',0x1e586a8)}
 for q in unknown:
  pc=q['PC'];row=dict(q,original_instruction_bytes=nodes[int(pc,0)]['bytes'])
  if pc in crt_slots:row.update(mechanism='mutable_CRT_callback_global',slot=hex(crt_slots[pc]),global_target_domain_proved=False,missing_dynamic_evidence='entry slot value, every setter/write during original path, callback address/ABI and exception behavior; saved zero is not dead');crt.append(row)
  elif pc in arrays:row.update(mechanism='mutable_CRT_callback_array',array_source=arrays[pc],global_target_domain_proved=False,missing_dynamic_evidence='actual array begin/end and every nonzero element, registration/write history, traversal order and callback ABI; initial empty array is not universal proof');crt.append(row)
  elif pc in dynamic:name,slot=dynamic[pc];row.update(mechanism='GetProcAddress_result',export_name=name,slot=hex(slot),global_target_domain_proved=False,missing_dynamic_evidence='real LoadLibrary/GetProcAddress module/export-to-guest-trampoline binding and current slot integrity; native User32 implementation unavailable; UI work remains forbidden');loader.append(row)
  else:
   m=re.fullmatch(r'dword ptr \[(eax|edx) \+ (\d+)\]',q['operand']);assert m,row;row.update(mechanism='virtual_slot',vptr_register=m[1].upper(),slot_offset=int(m[2]),global_target_domain_proved=False,missing_dynamic_evidence='actual callsite vptr register/receiver, immutable table membership and target, object lifecycle/factory provenance; active call not observed in available snapshots');business.append(row)
 assert(len(business),len(crt),len(loader))==(16,6,3)
 # Straight-line machine-byte identity at five sites proves the operand form.
 # For each fixed original vtable, the immutable table slot proves the exact
 # singleton target CONDITIONALLY. No claim that every receiver uses a table
 # in this list, and no filtering/removal of receivers from input is permitted.
 cfg=json.loads((ROOT/'stage75/configs/phase8-type53-family.json').read_text());tables=list(map(lambda v:int(v,0),cfg['profile_contract']['root_vtable_whitelist']));cases={0x4aef47:200,0x4aef92:612,0x4aefaa:612,0x5d4a69:716,0x5d4a76:728};certificates=[]
 for pc,offset in cases.items():
  q=nodes[pc];rows=[]
  for vt in tables:
   slot=vt+offset;section=pe.section(slot);assert section and not section['flags']&0x80000000;target=struct.unpack('<I',pe.read(slot,4))[0];assert target==int(cfg['resolved_indirect_edges'][hex(pc)]['target'],0)
   rows.append(dict(vtable=vt,slot=slot,target=target,slot_bytes=pe.read(slot,4).hex(),target_in_full_function_entries=hex(target)in c['function_entries']))
  certificates.append(dict(site=pc,original_instruction_bytes=q['bytes'],operand=q['operand'],vptr_register=re.match(r'dword ptr \[(\w+)',q['operand'])[1].upper(),slot_offset=offset,cases=rows,proof='exact original operand form plus readonly original PE slot bytes; runtime must validate vptr/permissions/current slot/target',universal_receiver_domain_proved=False))
 r=dict(status='25_CALLS_GROUPED_CONDITIONAL_DOMAINS_NOT_UNIVERSAL',counts=dict(virtual_slots=16,CRT_globals=4,CRT_arrays=2,GetProcAddress_results=3),groups=dict(virtual_slots=business,CRT_callbacks=crt,dynamic_exports=loader),conditional_singleton_site_certificates=certificates,conditional_sites=5,conditional_table_slot_cases=20,unconditional_complete_site_domains=0,no_receiver_input_filtering_or_mutation=True,unknown_call_policy='stop entire attempt with UNKNOWN_DYNAMIC_DOMAIN; never skip object/call or return success',game_or_Unicorn_executions=0)
 (ROOT/'stage79/evidence/call-domains.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps({k:r[k]for k in ['status','counts','conditional_sites','conditional_table_slot_cases','unconditional_complete_site_domains']}));return r
def resolve(certificates,site,target,cpu,read,permissions):
 from thread_contract import Boundary
 cert=next((x for x in certificates if x['site']==site),None)
 if not cert:raise Boundary('UNKNOWN_DYNAMIC_DOMAIN',site=hex(site))
 vt=cpu.get(cert['vptr_register']);row=next((x for x in cert['cases'] if x['vtable']==vt),None)
 if not row:raise Boundary('UNPROVED_VPTR_DOMAIN',vptr=vt)
 if permissions(row['slot'],4)!=1:raise Boundary('MUTABLE_OR_UNMAPPED_VTABLE_SLOT')
 actual=int.from_bytes(read(row['slot'],4),'little')
 if actual!=row['target']or target!=actual:raise Boundary('TABLE_OR_TARGET_BINDING_MISMATCH')
 if not row['target_in_full_function_entries']:raise Boundary('TARGET_NOT_MIGRATED')
 return actual
if __name__=='__main__':generate()
