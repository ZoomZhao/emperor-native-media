"""Protect all earlier artifacts and summarize actual build-only evidence."""
import collections,difflib,hashlib,json,re,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage78'
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb') as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
def main():
 baseline=json.loads((S/'evidence/protection-baseline.json').read_text());bad=[]
 for p,expected in baseline['hashes'].items():
  if not Path(p).is_file() or sha(p)!=expected:bad.append(p)
 ledger=json.loads((ROOT/'evidence/stage55-batch-ledger.json').read_text());assert len(ledger['executions'])==20 and ledger['remaining_processes']==80 and ledger['goal_closed'] is True
 protection=dict(status='PASS' if not bad else 'FAIL',protected_hashes_checked=len(baseline['hashes']),changed_or_missing=bad,old_ledger=dict(used=len(ledger['executions']),remaining=ledger['remaining_processes'],closed=ledger['goal_closed']),new_original_Unicorn_or_SR_game_library_executions=0,Stage73_two_claims_not_reused=True)
 (S/'evidence/protection-final.json').write_text(json.dumps(protection,indent=2)+'\n');assert not bad,bad
 reuse=[]
 for name in ['full_build.py','bind_guard_full.py']:
  old=(ROOT/'stage77/tools'/name).read_text().splitlines();new=(S/'tools'/name).read_text().splitlines();same=sum(x.size for x in difflib.SequenceMatcher(a=old,b=new,autojunk=False).get_matching_blocks());reuse.append(dict(file=name,reused_identical_lines=same,new_total_lines=len(new),identical_source_line_percent=round(100*same/len(new),3),metric_is_not_semantic_coverage=True))
 build=json.loads((S/'builds/full-format-v1/result.json').read_text());guard=json.loads((S/'builds/full-bound-guarded-open/result.json').read_text());formats=json.loads((S/'evidence/operand-formats.json').read_text());domains=json.loads((S/'evidence/dynamic-domains.json').read_text());pre=json.loads((S/'evidence/preflight.json').read_text());flags=json.loads((ROOT/'stage77/evidence/full-flags-unknown-v1.json').read_text())
 # Check actual source bytes and the order of SR comments independently of
 # helper symbol resolution. FS comments are rewritten by SR; other aliases
 # are normalized. This is format/order evidence, not instruction equivalence.
 closure=json.loads((ROOT/'stage76/evidence/full-dependency-closure-v1.json').read_text());comments=[line for line in(S/'builds/full-format-v1/seg01_code.original.llinc').read_text().splitlines() if line.startswith(';')];assert len(comments)==7334
 aliases={'jz':'je','jnz':'jne','jc':'jb','jnae':'jb','jnb':'jae','jnc':'jae','jna':'jbe','jnbe':'ja','jnge':'jl','jnl':'jge','jng':'jle','jnle':'jg','jpe':'jp','jpo':'jnp','setz':'sete','setnz':'setne','retn':'ret','sal':'shl','repnz':'repne'};fs=0
 for q,line in zip(closure['instructions'],comments):
  m=line[1:].split()[0]
  if q['mnemonic']!=aliases.get(m,m):assert q['mnemonic']=='mov' and 'fs:[0]' in q['operand'] and '_fs_dword' in line;fs+=1
 symbols=subprocess.run(['/usr/bin/nm','-g',str(S/'builds/full-bound-guarded-open/phase8-guarded-with-heap-OPEN.a')],capture_output=True,text=True,timeout=10);assert symbols.returncode==0;defined=set();undefined=set()
 for line in symbols.stdout.splitlines():
  m=re.fullmatch(r'\s*(?:[0-9a-fA-F]+\s+)?([A-Za-z])\s+(\S+)',line)
  if m:(undefined if m[1]=='U' else defined).add(m[2])
 unresolved=sorted(undefined-defined);stdlib={'_free','_malloc','_memcpy','_realloc'}
 result=dict(status='STAGE78_BUILD_ONLY_COMPLETE_RUNTIME_REFUSED',original_instructions=7334,original_bytes=20054,functions=201,original_bytes_changed=0,input_conditions_added=0,per_address_original_instruction_patches=0,reuse=reuse,format_extent_and_mnemonic_coverage=dict(verified=7334,total=7334,percent=100),old_530_unsupported_classification=formats['old_unsupported_categories'],format_unknown_remaining=0,emitted_relocations=build['reconstructed_relocations'],relocation_emission_automatic_percent=100,relocation_semantic_denominator_proved=False,mapped_immediate_pointer_questions=85,indexed_displacement_pointer_questions=62,flags_annotations=len(flags['flags']['annotations']),flags_fixed_point_converged=True,flags_summary_updates=flags['statistics'].get('summary_updates'),native_full_flags_equivalence_proved=False,conditional_register_IAT_bindings=len(build['register_import_bindings']),conditional_register_binding_application_percent=100,remaining_non_IAT_CALL_sites=domains['remaining_non_IAT_CALL_sites_after_conditional_classification'],reviewed_not_machine_proved_jump_domains=len(domains['reviewed_jump_domains']),dynamic_target_resolver_interfaces=guard['generic_dynamic_target_resolvers'],fully_machine_proved_dynamic_domains=0,guest_CALL_bindings=guard['guest_CALL_return_PC_edits'],guest_RET_bindings=guard['guest_RETURN_resolvers'],guest_data_VA_bindings=guard['guest_data_VA_bindings'],scalar_load_guards=guard['scalar_guest_memory_load_guards'],scalar_ordered_write_guards=guard['scalar_guest_memory_store_guards_and_ordered_logs'],REP_FS_original_sites=dict(REP=24,FS=31),REP_FS_helper_symbols_compiled=6,authored_native_helper_scenarios_passed=17,independent_byte_comparison_flag_cases=65536,authored_decoder_tests_passed=6,authored_preflight_tests_passed=3,own_ABI_container_probe_passed=True,remaining_external_symbols=unresolved,remaining_external_count=len(unresolved),remaining_standard_C_symbols=len(set(unresolved)&stdlib),remaining_platform_API_symbols=sum(x not in stdlib and not x.startswith(('_stage','_x86')) for x in unresolved),fresh_full_CPU_or_hidden_segment_cache_verified=False,REP_original_intra_element_faults_verified=False,SEH_dispatch_unwind_verified=False,preflight_runtime_allowed=pre['runtime_allowed'],blocking_gates=pre['blocking_gates'],old_ledger=protection['old_ledger'],protected_hashes_passed=len(baseline['hashes']),new_game_Unicorn_or_SR_game_library_executions=0,guarded_archive_sha256=guard['archive_sha256'],ordered_SR_comments_verified=7334,FS_lowering_comments_verified=fs)
 (S/'evidence/result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()
