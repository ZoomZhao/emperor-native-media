"""Reviewable static/build delivery manifest; no approval or execution claim."""
import collections,hashlib,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage75';sys.path.insert(0,str(ROOT/'samples'))
from auto_i386_analysis import PE
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
def load(p):return json.loads((ROOT/p).read_text())
def main():
 pe=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe');at=struct.unpack_from('<I',pe.data,60)[0]+24;imports_RVA=struct.unpack_from('<I',pe.data,at+104)[0];imports={}
 def string(address):
  b=bytearray()
  for k in range(512):
   c=pe.read(address+k,1)[0]
   if not c:return b.decode('ascii')
   b.append(c)
  raise ValueError('unterminated PE import')
 for k in range(100):
  oft,time,forward,name,iat=struct.unpack('<5I',pe.read(pe.base+imports_RVA+20*k,20))
  if not any((oft,time,forward,name,iat)):break
  dll=string(pe.base+name)
  for j in range(4096):
   thunk=struct.unpack('<I',pe.read(pe.base+(oft or iat)+4*j,4))[0]
   if not thunk:break
   imports[pe.base+iat+4*j]=dll+'.'+('ordinal:'+str(thunk&0xffff)if thunk&0x80000000 else string(pe.base+thunk+2))
 def metrics(path):
  a=load(path);edges={e['pc']:e for e in a['indirect_edges']};categorized=[]
  for e in sorted(edges.values(),key=lambda x:x['pc']):
   r=dict(e);r['PC']=hex(r.pop('pc'));m=__import__('re').fullmatch(r'dword ptr \[(\d+)\]',e['operand']);slot=int(m[1])if m else None
   r['category']='reviewed_static_jump_domain'if e['kind']=='indirect_jump'and e['resolved']else 'IAT_import'if slot in imports else'unresolved_dynamic_call';r['import_name']=imports.get(slot);categorized.append(r)
  return dict(evidence=path,function_entries=len(a['function_entries']),instructions=a['instruction_count'],bytes=a['code_bytes'],direct_call_sites=len({e['pc']for e in a['edges']if e['kind']=='direct_call'}),unique_indirect_sites=len(edges),categories=dict(collections.Counter(r['category']for r in categorized)),edges=categorized,decode_gaps=len(a['gaps']),pending=len(a['pending_PC']),index_proofs_machine_checked=False,original_game_executed=False)
 build=load('stage75/builds/type53-family-v5/result.json');audit=load('stage75/evidence/implementation-audit.json');protect=load('stage75/evidence/protection-postflight.json');cfg=load('stage75/configs/phase8-type53-family.json');vt=load('stage75/evidence/vtable-inventory.json');snap=load('stage75/evidence/snapshot-targets.json');edits=collections.Counter(q['kind']for q in build['translation_binding_edits'])
 snapshots=[]
 for s in snap['snapshots']:
  snapshots.append(dict(tag=s['tag'],path=s['path'],sha256=s['sha256'],vector_count=s['helper_413a50']['count'],active_records=sum(bool(r['active_byte'])for r in s['root_vector_records']),eligible_root_receivers=len(s['eligible_root_virtual_receivers'])if 'eligible_root_virtual_receivers'in s else len(s.get('selected',[])),observed_container_vtables=s['vtable_counts']))
 report=dict(status='GUARDED_PHASE8_BUSINESS_CLOSURE_LINKED_ARM64_BUILD_ONLY',directory=str(ROOT),preserved_Stage73_success=True,preserved_Stage74=True,original_game_or_Unicorn_or_derived_library_executions=0,execution_approval_or_claim_created=False,
  build=dict(result='stage75/builds/type53-family-v5/result.json',artifact=build['artifact'],sha256=build['sha256'],architecture='Mach-O arm64',original_functions=18,source_bytes=759,source_instructions=278,guarded_reachable_instructions=276,guarded_reachable_bytes=752,link_complete_under_explicit_profile=True,unresolved_original_symbol_count=0,behavior_validated=False,runtime_eligible=False,stop_SP=None),
  automation=dict(relocations_emitted=5,automatic_emitted=5,automatic_emitted_percent=100,image_address_candidates=9,classified_candidates=5,classified_percent=round(500/9,3),unclassified_original_immediates=4,manual_pointer_metadata=0,automatic_flags_annotations=36,flags_metadata_generation_percent=100,flags_contract_scope='static CFG under checked type/branch domain, NOT behavior validation',incoming_flags_leaf='4e1d00 six incoming arithmetic flags left unchanged',data_operand_bindings=edits['data_label_to_original_guest_VA'],CALL_guest_return_push_bindings=edits['guest_CALL_return_binding'],RET_resolver_sites=edits['guest_RET_resolver'],native_return_PC_bindings=23,indirect_generated_bindings=6,automatic_binding_generation_percent=100,manually_reviewed_indirect_site_metadata=6,unique_indirect_target_functions=5,manually_reviewed_branch_domain_proofs=1,new_original_instruction_bytes_changed=0,new_per_address_instruction_replacements=0,guarded_loads=90,guarded_stores_with_ordered_log=75,guarded_guest_access_percent=100,REP_helper_actual_store_log_sites=4,source_line_reuse=audit['source_line_reuse']),
  profile=dict(root='0x4aeeb0',helper413a50_original_included=True,root_virtual_sites=['0x4aef47','0x4aef92','0x4aefaa'],nested_indirect_sites=['0x5d4a69','0x5d4a76','0x546bec'],vtable_whitelist=cfg['profile_contract']['root_vtable_whitelist'],only_first_vtable_saved_observed=True,component_vtable='0x7b75c0',original_count8_leaf='0x4e1d00',vector_count_at_least2=True,original413a80_throw_SEH_CRT_boundary_declined_without_fake_return=True,original_body_source_bytes_preserved=True,all_input_or_all_type_equivalence_claimed=False,input_alias_fault_concurrency_contract_not_proved=True),
  historical_snapshots=snapshots,structural_vtable_inventory=dict(candidate_tables=vt['structural_candidate_count'],distinct_target200=len(vt['candidate_target200']),distinct_target612=len(vt['candidate_target612']),not_all_candidates_proved_phase8_receivers=True),
  full_phase8_static=metrics('stage75/evidence/exact-whole-phase8-closure-v3.json'),all_candidate_methods_static_superset=metrics('stage75/evidence/exact-candidate-targets-closure-v3.json'),
  jump_tables=dict(reviewed_relative_dispatch_specs=16,copies=2,generated_original_sites=32,original_pointer_slots=sum(len(x['rows'])for x in load('stage75/configs/crt-jump-domains.json')['domains'].values()),negative_and_wrapped_index_domains_explicit=True,all32_static_target_sets_enumerated=True,these_CRT_routines_native_migrated=False,proof='human-reviewed assembly bounds plus original-byte/slot checks; no formal abstract interpretation'),
  audit='stage75/evidence/implementation-audit.json',protection=protect,
  failures=['build-v1: llasm79 missing comma for two-argument helper; corrected','build-v2/v3: duplicate hidden LLVM external declaration; corrected with exact declaration removal','exact-scan-v1: symbol suffix on direct targets incorrectly classified; corrected and rerun v2/v3','static-audit-v1: LLVM private return labels absent from nm; decoded on-disk Mach-O format6 chained fixups using installed Apple SDK header'],
  remaining=['No saved input has an active eligible receiver; existing history proves phase8 entry/return only, not any of the three root virtual calls','No exact phase8 entry CPU/stopSP/input manifest; no approved Stage75 differential','Full-input413a80 throw path requires FS/SEH/CRT/import semantics; current profile fails outside count>=2 domain','Full phase8 static graph retains IAT imports and register/global/virtual callbacks; 76 structural tables are not a type/factory reachability theorem','New native binding/preflight/flags logic remains unexecuted and needs real differential under a frozen valid input; arbitrary alias/fault/thread domains unproved'],
  minimal_future_reference_scope=dict(status='proposal_only_requires_new_explicit_authorization_and_real_input',start_PC='0x4aeeb0',stop_before_PC='0x4ac33d',stop_SP='must derive from exact real entry snapshot; do not guess',reference_processes=1,native_processes=1,profile='one real input with active type53 receiver and required nested methods; no caller continuation, window/audio/input/import/whole-city execution',obtain_input='Prefer existing authentic entry snapshot/trace with active receiver. If absent, acquiring one itself needs an explicitly bounded new original execution. Do not toggle activity bytes or force saved phase7 CPU to phase8.',old_Stage73_approval_reusable=False,closed_20_80_ledger_reusable=False),
  observed_cost=dict(successful_family_build_seconds=load('stage75/evidence/guarded-business-family-build-v5.result.json')['duration_seconds'],phase8_aligned_scan_seconds=load('stage75/evidence/exact-whole-phase8-closure-v3.json')['duration_seconds'],candidate_aligned_scan_seconds=load('stage75/evidence/exact-candidate-targets-closure-v3.json')['duration_seconds'],preservation_seconds=load('stage75/evidence/preservation-postflight-v1.result.json')['duration_seconds'],remaining_engineering_time='Not measured. Remaining scale is quantified in open call sites/semantic contracts; no day estimate invented.'))
 paths=[]
 for p in S.rglob('*'):
  if p.is_file()and '__pycache__'not in p.parts and p.name not in ('delivery-manifest.json','result.json')and not p.name.startswith('delivery-manifest-build-'):paths.append(p)
 for p in [ROOT/'samples/stage72_runtime.c',ROOT/'samples/stage72_runtime.h',ROOT/'samples/build_pe.py',ROOT/'samples/auto_i386_analysis.py',ROOT/'samples/stage60_controlled_input_plan.py',ROOT/'stage74/tools/analysis_v2.py',ROOT/'build/bin/SRW-llasm',ROOT/'build/bin/llasm',ROOT/'vendor-src/SR/SR/llasm-support/llasm_stos.c',Path(cfg['original_EXE'])]:paths.append(p)
 hashes={str(p.relative_to(ROOT))if p.is_relative_to(ROOT)else str(p):sha(p)for p in sorted(set(paths))}
 report_path=S/'evidence/result.json';assert not report_path.exists();report_path.write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n')
 hashes['stage75/evidence/result.json']=sha(report_path)
 manifest=S/'evidence/delivery-manifest.json';assert not manifest.exists();manifest.write_text(json.dumps(dict(kind='STATIC_BUILD_DELIVERY_NOT_EXECUTION_AUTHORIZATION',pinned_files=hashes,report_sha256=sha(report_path),runtime_eligible=False,old_ledger_state=[20,80,'closed'],no_attempt_claim=True),indent=2)+'\n')
 print(json.dumps(dict(status=report['status'],full_phase8_categories=report['full_phase8_static']['categories'],candidate_categories=report['all_candidate_methods_static_superset']['categories'],pinned_delivery_files=len(hashes),artifact_SHA256=build['sha256'])))
if __name__=='__main__':main()
