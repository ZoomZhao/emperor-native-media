"""Runnable preflight and authored capture; original instruction path is closed."""
import sys,argparse,json,struct,time
from pathlib import Path
sys.dont_write_bytecode=True
from core import ROOT,V,SOURCE,PINS,LIB_SHA,sha,put,verify,HostStore,Boundary,fixed
def preflight():
 s=json.loads((V/'configs/source.json').read_text());verify({str(SOURCE/n):h for n,h in PINS.items()});h=HostStore(ROOT/s['host_manifest']);cpu=json.loads((V/'evidence/cpu-restore.json').read_text());own=json.loads((V/'evidence/component-tests-v2.json').read_text());history=json.loads((V/'evidence/historical-memory-and-input-calls.json').read_text());static=json.loads((V/'evidence/static-prefix.json').read_text());full=json.loads((V/'evidence/full-size-memory-v2.json').read_text());calls=[r for r in history['calls']if r['name']=='GetAsyncKeyState']
 blockers=[{'kind':'ORIGINAL_EXECUTION_NOT_AUTHORIZED_THIS_DEVELOPMENT','remaining_rounds':9,'new_attempt_claim_allowed':False},
 {'kind':'CONTROLLED_KEYBOARD_PREFIX_UNIMPLEMENTED_AND_OUTSIDE_CURRENT_SCOPE','return_PCs':[r['return_pc']for r in calls],'keys':[r['args'][0]for r in calls],'source_domain':calls[0]['domain'],'historical_backend':calls[0]['backend'],'physical_keyboard_required':False,'constant_zero_substitution_allowed':False},
 {'kind':'WORKER_MUTABLE_CALLBACK_DOMAIN_NOT_CLOSED','PC':0x5fc139,'operand':'[EDX]','earlier_current_queue_count':static['worker4']['captured_callback_count'],'current_empty_queue_is_not_all_future_paths_certificate':True},
 {'kind':'LIVE_CLOCK_PREFIX_AND_PHASE7_WATCH_HOOKS_NOT_YET_INTEGRATED_OR_QUALIFIED','GetTickCount_calls_in_historical_next_snapshot':static['historical_new_API_kinds'].get('KERNEL32.dll!GetTickCount'),'source_Watch_must_be_preserved':True,'cross_cycle_allowed':False},
 {'kind':'A_CAPS_TOO_SMALL_FOR_OBSERVED_FRAME_CONTRACT','approved_steps':6000000,'approved_writes':2000000,'approved_guest_seconds':8,'role_total_seconds':60,'conditional_pixel_instruction_lower_bound':full['conditional_guest_pixel_instruction_lower_bound'],'conditional_guest_plus_platform_records':full['conditional_frame_guest_plus_platform_writes'],'whole_prefix_wall_or_step_bound_proven':False},
 {'kind':'ORIGINAL_MULTI_STORE_PRECOMMIT_PLAN_NOT_QUALIFIED','own_fixture_single_store_and_whole_API_precommit_proven':True,'original_REP_and_cross_region_fault_semantics_proven':False}]
 return {'status':'ORIGINAL_CAPTURE_CLOSED_EXPLICIT_PRECHECK_BOUNDARIES','source_pins':PINS,'fixed_unicorn_sha256':LIB_SHA,'four_CPUs_restored':cpu['status'],'source_current_tid':h['current_tid'],'source_thread_statuses':{str(k):v['status']for k,v in h['guest_threads20'].items()},'next_scheduling_preserves_existing_kernel':{'last_at_invocation':1,'quantum':h['thread_quantum'],'worker_first':h['worker_first'],'thread_prefer':h['thread_prefer'],'wake_before_choose':True,'no_force_main':True},'proposed_original_instruction_threads':[1,2,4],'thread3_if_naturally_runnable':'STOP_WITHOUT_REWRITING_WAIT_STATE','source_Watch':h['watch_state54'],'original_goal':static['original_caller_to_phase8'],'self_cases':own['case_count'],'full_size_platform':full,'blockers':blockers,'runtime_imported':False,'emu_start_calls':0,'original_execution':0,'remaining_rounds':9}
def authored(out):
 out=out.resolve()
 if not out.resolve().is_relative_to(V/'runs')or out.is_symlink():raise Boundary('OWN_OUTPUT_SCOPE')
 out.mkdir(parents=True,exist_ok=False);put(out/'authored.claim.json',{'kind':'AUTHORED_ONLY','original_authority':False,'original_rounds_consumed':0,'maximum_own_capture_executions':1})
 u=fixed();from fixture import make
 from stage73_cpu import public_state
 from unicorn import x86_const as x
 r,meta=make(u);start=time.monotonic()
 try:
  q=r.thread_run(3,100)
  if q['status']!='natural_authored_call':raise Boundary('AUTHORED_CAPTURE_FAILED '+str(q))
  r.thread_save();threads={};cur=r.current_tid;saved=r.uc.context_save();regions=[];offset=0
  with(out/'memory.bin').open('xb')as f:
   for lo,hi,p in r.m.regions:b=r.m.read(lo,hi-lo);f.write(b);regions.append({'low':lo,'high':hi,'permissions':p,'offset':offset,'sha256':__import__('hashlib').sha256(b).hexdigest()});offset+=len(b)
  for tid,t in sorted(r.threads.items()):
   r.uc.context_restore(t['context']);body=t['context'].__getstate__()[0];(out/f'context-{tid}.bin').write_bytes(body);threads[str(tid)]={'public_CPU':public_state(r.uc,u),'status':t['status'],'wait':t['wait'],'tls':t['tls'],'last_error':t['last_error'],'teb':t['teb'],'context_payload_sha256':__import__('hashlib').sha256(body[40:]).hexdigest()}
  r.uc.context_restore(saved);(out/'events.bin').write_bytes(r.events.export());(out/'surfaces.bin').write_bytes(r.bridge.host['dd_native_blob'])
  with(out/'writes.bin').open('xb')as f:
   for tid,pc,a,n,v,kind in r.m.trace:f.write(struct.pack('<5I',tid,pc,a,n,v))
  put(out/'snapshot.json',{'kind':'AUTHORED_FOUR_THREAD_NATURAL_CALL','original_input':False,'current_tid':cur,'regions':regions,'memory_bytes':offset,'thread_records':threads,'goal_contract':r.goal_contract,'natural_caller_SP':r.caller_sp,'api_log':r.api_log,'budget':{'shared_steps':r.m.budget.steps,'shared_writes':r.m.budget.writes,'steps_by_thread':r.m.budget.by_thread},'original_game_functions':0,'no_force_main':True,'scheduled':r.scheduled})
  put(out/'frozen.json',{'hashes':{str(p.relative_to(ROOT)):sha(p)for p in out.iterdir()if p.is_file()},'original_instruction_execution_authorized':False})
  print(json.dumps({'status':'AUTHORED_CAPTURE_AND_FREEZE_PASS','scheduled':r.scheduled,'steps':r.m.budget.steps,'writes':r.m.budget.writes,'memory_bytes':offset,'seconds':time.monotonic()-start,'original_execution':0}))
 finally:r.events.close()
def main():
 p=argparse.ArgumentParser();g=p.add_mutually_exclusive_group(required=True);g.add_argument('--preflight',action='store_true');g.add_argument('--self-capture',action='store_true');g.add_argument('--original-run',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
 if a.original_run:raise Boundary('ORIGINAL_EXECUTION_CLOSED_BEFORE_ENGINE_IMPORT_NO_CLAIM')
 if a.preflight:
  r=preflight()
  if a.output:
   if not a.output.resolve().is_relative_to(V):raise Boundary('PREFLIGHT_OUTPUT_SCOPE')
   put(a.output,r)
  print(json.dumps(r,default=str));return
 if a.output is None:raise Boundary('AUTHORED_OUTPUT_REQUIRED')
 authored(a.output)
if __name__=='__main__':main()
