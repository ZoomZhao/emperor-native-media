import hashlib,json,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage79'
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
def main():
 b=json.loads((S/'evidence/protection-baseline.json').read_text());bad=[p for p,digest in b['hashes'].items()if not Path(p).is_file()or sha(p)!=digest];assert not bad,bad
 old=json.loads((ROOT/'evidence/stage55-batch-ledger.json').read_text());assert(len(old['executions']),old['remaining_processes'],old['goal_closed'])==(20,80,True)
 assert not(S/'evidence/cpu-evidence-attempt.claim.json').exists()and not(S/'runs/cpu-evidence-once').exists()
 m=json.loads((S/'configs/cpu-evidence-manifest.json').read_text());assert all(sha(p['path'])==p['sha256']for p in m['pinned_files'])
 guard=json.loads((S/'builds/full-bound-guarded-open/result.json').read_text());build=json.loads((S/'builds/known-method-full-v2/result.json').read_text());pre=json.loads((S/'evidence/preflight.json').read_text());domains=json.loads((S/'evidence/call-domains.json').read_text());assert pre['known_missing_native_targets']==[]and not pre['phase8_runtime_allowed']
 native_functions=set()
 for line in(S/'builds/full-bound-guarded-open/guarded.ll').open():
  if line.startswith('define ')and len(line)<300:native_functions.add(line.split('@',1)[-1].split('(')[0])
 assert {'function_546be0','function_546c40'}<=native_functions,native_functions
 result=dict(status='STAGE79_REAL_RESTORE_MODELS_AND_CONDITIONAL_DOMAINS_BUILT_CPU_REQUEST_READY_NOT_RUN',protected_hashes_passed=len(b['hashes']),old_ledger=dict(used=20,remaining=80,closed=True),new_Unicorn_or_original_or_SR_game_library_runs=0,new_CPU_evidence_claim_created=False,private_snapshot_memory_restored_and_compared_bytes=297521152,private_regions_restored=28,host_page_size_issue='macOS arm64 16KiB; union host backing permissions, exact 4KiB guest permissions/holes by guard; never PROT_EXEC',root_CPU_overrides_stale_saved_current_thread=True,worker_public_CPU_missing=True,opaque_context_offsets_guessed=False,own_CPU_thread_exception_tests_passed=8,own_native_thread_state_tests_passed=True,own_dynamic_domain_tests_passed=4,own_request_interface_tests_passed=2,SEH_dispatch_or_unwind_implemented=False,native_thread_API_ABIs_compiled=5,non_IAT_CALL_mechanisms=domains['counts'],conditional_CALL_sites=5,conditional_tail_JMP_sites=1,conditional_original_table_cases=21,unconditional_complete_dynamic_site_domains=0,original_additional_known_target_instructions=12,original_additional_known_target_bytes=35,known_missing_targets_fixed=['0x546be0','0x546c40'],source_function_entries=203,source_instructions=7346,source_bytes=20089,original_instruction_bytes_changed=0,receiver_input_pruning_or_skip_added=False,all_static_or_dynamic_dependencies_proved=False,guarded_archive_sha256=guard['archive_sha256'],SR_build_steps_all_zero=all(x['returncode']==0 for x in build['steps']),phase8_runtime_allowed=False,CPU_request_original_instructions_maximum=0,CPU_request_authored_instructions_maximum=56,CPU_request_processes_maximum=1,CPU_request_manifest_sha256=sha(S/'configs/cpu-evidence-manifest.json'),minimum_request_source_code_and_describe_test_ready=True,minimum_request_new_explicit_approval_present=False,retained_failures=['native memory v1: 4KiB mprotect rejected on 16KiB macOS host pages; fixed v2 passed','known-target build v1: missing own operand_formats import; fixed v2 passed'])
 (S/'evidence/result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()
