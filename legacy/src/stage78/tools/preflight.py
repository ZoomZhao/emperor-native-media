"""Audit-only mandatory execution gates. False/missing/unknown always refuse.

There is intentionally no game launcher, bypass flag or force-success option.
The output states what was built/tested separately from behavior not verified.
"""
import hashlib,json,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
def accept_required(gates):return bool(gates) and all(type(x)is bool and x is True for x in gates.values())
def main(config_path,output):
 cfg=json.loads(Path(config_path).read_text());artifacts={};loaded={}
 for k,v in cfg.items():
  if isinstance(v,str):
   p=(ROOT/v).resolve();assert p.is_relative_to(ROOT) and p.is_file();b=p.read_bytes();artifacts[k]=dict(path=v,sha256=hashlib.sha256(b).hexdigest());loaded[k]=json.loads(b) if p.suffix=='.json' else b.decode()
 f=loaded['format_audit'];d=loaded['domain_audit'];s=loaded['state_contract'];g=loaded['guarded_build'];b=loaded['full_build'];flags=loaded['flags_audit'];tests=loaded['own_helper_test_result']
 static=dict(original_source_extents_and_mnemonics_verified=f['extent_and_mnemonic_verified']==7334 and not f['remaining_format_failures'],flags_monotone_postfixpoint=flags['statistics']['converged'],SRW_llasm_clang_full_build=all(x['returncode']==0 for x in b['steps']),guest_CALL_RET_and_scalar_guards_compiled=g['instruction_step_markers']==7334 and g['guest_RETURN_resolvers']==288 and not g['native_static_data_references_remaining'],REP_FS_callback_ABIs_compiled=g['REP_and_FS_callback_contracts_compiled'])
 own=dict(helper_test_process_passed=tests['returncode']==0 and not tests['timed_out'],helper_scenarios_and_exhaustive_cmp8='17 authored tests passed' in loaded['own_helper_test_log'])
 mandatory=dict(exact_fresh_phase8_entry_and_return_context=False,all_cached_segments_publicly_verified=s['fresh_FS_cache_base_limit_rights_verified'] and s['DS_ES_flat_cache_verified_at_fresh_entry'],whole_CPU_memory_threads_TLS_LastError_restored=s['whole_memory_thread_TLS_LastError_restore_implemented'],full_platform_callbacks_contracts_and_implementations=False,all_dynamic_target_domains_proved=d['fully_machine_proved_per_site_domains']==123,mutable_IAT_and_jump_table_domain_integrity_verified=False,all_ambiguous_pointer_operands_resolved=not f['candidate_counts'].get('mapped_immediate_not_pointer_proved') and not f['candidate_counts'].get('indexed_displacement_not_pointer_proved'),original_REP_partial_fault_and_ordered_trace_behavior_verified=False,SEH_dispatch_unwind_RaiseException_correctly_supported=s['SEH_dispatch_and_unwind_implemented'],native_full_flags_equivalence_verified=flags['native_flag_equivalence_proved'],meaningful_active_city_phase_input_verified=False,new_separate_approval_and_claims_present=cfg['approved_new_execution'] is not None)
 allowed=accept_required(mandatory) and accept_required(static) and accept_required(own)
 report=dict(status='READY_FOR_APPROVED_SCOPED_DIFFERENTIAL' if allowed else 'REFUSE_GAME_EXECUTION_UNCLOSED_CONTRACTS',runtime_allowed=allowed,artifacts=artifacts,static_evidence=static,self_authored_tests=own,mandatory_behavior_gates=mandatory,blocking_gates=[k for k,v in mandatory.items() if v is not True],missing_or_unknown_policy='reject; never substitute code-table membership, zero IAT/callback values, input clipping or fake API returns',game_or_Unicorn_or_derived_library_executions=0,old_stage73_pass_domain='six functions / 728 original bytes / exact one approved real input and stop boundary only')
 p=Path(output);assert not p.exists();p.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ('status','runtime_allowed','blocking_gates')}))
if __name__=='__main__':main(sys.argv[1],sys.argv[2])
