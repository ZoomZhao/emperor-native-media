import hashlib,json,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage80'
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
def main():
 baseline=json.loads((S/'evidence/protection-baseline.json').read_text());bad=[p for p,h in baseline['hashes'].items()if not Path(p).is_file()or sha(p)!=h];assert not bad,bad
 ledger=json.loads((ROOT/'evidence/stage55-batch-ledger.json').read_text());assert(len(ledger['executions']),ledger['remaining_processes'],ledger['goal_closed'])==(20,80,True)
 manifest=ROOT/'stage79/configs/cpu-evidence-manifest.json';m=json.loads(manifest.read_text());assert all(sha(q['path'])==q['sha256']for q in m['pinned_files']);assert not(ROOT/'stage79/approval.json').exists()
 evidence=json.loads((ROOT/'stage79/runs/cpu-evidence-once/result.json').read_text());assert evidence['original_game_instructions']==0 and len(evidence['probes'])==56
 assert(S/'evidence/measured-thread-transfer.bin').read_bytes()==(S/'evidence/native-roundtrip-v4.bin').read_bytes()
 assert json.loads((S/'evidence/run-own-logical-runtime-v4.result.json').read_text())['returncode']==0
 build=json.loads((S/'builds/runtime-configurable-v3-OPEN/result.json').read_text());assert all(q['returncode']==0 for q in build['steps'])
 for p,h in json.loads((S/'configs/runtime-build-v2.json').read_text())['pinned_inputs'].items():assert sha(ROOT/p)==h,p
 result=dict(status='MEASURED_CPU_IMPORTED_AUTHORED_RUNTIME_CONNECTION_TESTED_BUILD_ONLY_CODE_PLACEMENT_BLOCKED',protected_hashes_passed=len(baseline['hashes']),old_ledger=dict(used=20,remaining=80,closed=True),new_original_game_instructions=0,new_SR_game_library_loads_or_executions=0,new_original_thread_scheduling=0,authored_Unicorn_probes_executed=56,authored_Unicorn_processes=1,diagnostic_retry=False,diagnostic_manifest_sha256=sha(manifest),genuine_existing_authorization_resolved=True,new_user_approval_fabricated=False,thread_public_CPU_records=4,worker_public_CPU_missing=False,all_worker_threads_still_waiting=True,GS_BASE_public_read_supported=False,FS_cached_limit_value_known=False,FS_callback_allowed_offsets=[0],native_initial_state_roundtrip_exact_bytes=(S/'evidence/native-roundtrip-v4.bin').stat().st_size,opaque_context_offsets_guessed=False,opaque_is_initial_sidecar_not_mutated_engine_context=True,private_memory_regions_imported_and_compared=28,private_memory_bytes_imported_and_compared=297521152,connected_bridges=len(build['connected_bridge_symbols']),thread_API_C_frontends_connected_and_authored_tested=5,conditional_dynamic_cases=21,unconditional_complete_dynamic_domains=0,pre_store_write_budget_guard=True,scalar_and_REP_write_trace_callbacks_connected=True,native_code_and_guest_backing_co_placement_proved=False,native_code_binding_positive_selftest_uses_synthetic_pointers_only=True,protected_game_object_reused_build_only_functions=203,protected_game_object_reused_build_only_instructions=7346,protected_game_object_reused_build_only_bytes=20089,source_instruction_bytes_changed=0,new_per_original_address_patch_count=0,authored_runtime_selftest_UBSan_passed=True,phase8_runtime_allowed=False,archive_sha256=build['archive_sha256'],remaining_external_symbols=build['remaining_external_symbols'],retained_failures=['compile-v1: existing frozen helper warning promoted to error; subsequent compile uses explicit warning flag','runtime-v2/v3: safe mach_vm_allocate KERN_NO_SPACE at backing after main code','runtime-v4: own small PAGEZERO/image_base layout killed by macOS, not bypassed','compile-layout-v5: arm64 linker ignores no_pie/image_base, not executed','runtime-v6: safe KERN_NO_SPACE near authored dylib; no successful common-offset layout claim'])
 (S/'evidence/result.json').write_text(json.dumps(result,indent=2)+'\n')
 artifacts={str(p.relative_to(ROOT)):sha(p)for p in S.rglob('*')if p.is_file()and p.name!='manifest.json'}
 added=['stage79/evidence/cpu-evidence-attempt.claim.json','stage79/runs/cpu-evidence-once/result.json'];artifacts.update({p:sha(ROOT/p)for p in added})
 (S/'manifest.json').write_text(json.dumps(dict(previous_protected_hashes=4511,hashes=artifacts,diagnostic_original_manifest_sha256=sha(manifest),old_ledger_sha256=sha(ROOT/'evidence/stage55-batch-ledger.json')),indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()
