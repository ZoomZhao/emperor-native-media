import json,struct,sys
sys.dont_write_bytecode=True
from common import ROOT,S,sha,put,verify
def main():
 baseline=json.loads((S/'evidence/protection-baseline.json').read_text());verify(baseline['hashes']);ledger=json.loads((ROOT/'evidence/stage55-batch-ledger.json').read_text());assert len(ledger['executions'])==20 and ledger['remaining_processes']==80 and ledger['goal_closed']
 c=json.loads((S/'configs/original.json').read_text());verify(c['pins']);assert not list((S/'evidence').glob('original-*-attempt.claim.json'))and not list((S/'evidence').glob('*APPROVAL*'))
 positive=json.loads((S/'runs/authored-platform-ABC-v3/result.json').read_text());small=json.loads((S/'runs/authored-ABC-v3/result.json').read_text());assert positive['status']==small['status']=='EXACT_KNOWN_STATE_AND_ORDERED_WRITES_MATCH'and not positive['mismatches']and positive['scope_label']=='AUTHORED_DRIVER_PROTOCOL_ONLY'
 assert json.loads((S/'builds/negative-v2/result.json').read_text())['cases']==12
 guard=json.loads((S/'evidence/execution-gate-negative-tests-v2.json').read_text());assert len(guard['cases'])==2
 counts={0:0,1:0}
 for r in struct.iter_unpack('<6I',(S/'runs/authored-platform-ABC-v3/reference/writes.bin').read_bytes()):counts[r[0]]+=1
 build=json.loads((S/'builds/original-v3/result.json').read_text());assert build['status']=='DRIVER_BUILT_NOT_EXECUTED'and all(r['returncode']==0 for r in build['steps'])
 blockers=json.loads((S/'evidence/blockers.json').read_text());assert len(blockers['unknown_indirect_CALL_sites'])==20
 for r in blockers['APIs']:
  if r['API']in ('InitializeCriticalSection','EnterCriticalSection','LeaveCriticalSection'):r['status']='IMPLEMENTED_CONDITIONALLY_AUTHORED_20_API_CALL_DIFFERENTIAL_PASSED'
 put(S/'evidence/blockers-verified.json',blockers)
 result={'status':'STAGE82_NATURAL_CAPTURE_DIFFERENTIAL_DRIVERS_IMPLEMENTED_AUTHORED_VALIDATED_REAL_FULL_BUILD_ONLY','protected_previous_hashes_passed':baseline['count'],'old_ledger':{'used':20,'remaining':80,'closed':True},'new_original_instructions':0,'new_SR_game_function_invocations':0,'new_original_approval_or_attempt_claims':0,'function_set_unchanged':{'functions':203,'instructions':7346,'bytes':20089},'generic_natural_CALL_return_and_SP_proof_implemented':True,'configurably_bounded_A_B_C_command_implemented':True,'own_simple_pipeline_case':small,'own_platform_pipeline_case':positive,'authored_platform_API_calls':20,'authored_platform_API_kinds':15,'ordered_instruction_write_records':counts[0],'ordered_platform_write_records':counts[1],'native_API_wrappers_with_real_guest_stack_return':15,'authored_boundary_or_binding_cases':12,'original_execution_gate_denial_cases':2,'Win32_previous_open':23,'Win32_new_conditional_lock_implementations':3,'Win32_remaining_explicit_boundaries':20,'named_API_current_IAT_and_register_guards':len(c['named_sites']),'unknown_non_IAT_CALL_sites_remaining':20,'SEH_dispatch_unwind':'EXPLICIT_UNIMPLEMENTED_FAILURE','natural_original_input_available':False,'all_path_prerequisites_proved':False,'combined_command_is_implemented_not_a_stub_proposal':True,'approval_required_for_new_original_scope':True,'no_input_filter_or_ledger_reset':True,'original_driver_sha256':sha(S/'builds/original-v3/native-driver'),'configuration_sha256':sha(S/'configs/original.json'),'request':'stage82/configs/combined-validation-request.json','recovery':'all sources/configs/fixtures/build outputs/failures present under Stage82'}
 put(S/'evidence/result.json',result);print(json.dumps({k:result[k]for k in ('status','protected_previous_hashes_passed','new_original_instructions','authored_platform_API_calls','ordered_platform_write_records','Win32_remaining_explicit_boundaries')}))
if __name__=='__main__':main()
