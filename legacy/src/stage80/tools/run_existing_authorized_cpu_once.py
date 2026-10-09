"""Correct prior overbroad approval gate using real existing authorization.

Read fixed original Stage79 manifest and its unchanged evidence() implementation;
no Stage79 source, approval or manifest is edited. Only its already planned new
claim/output paths are created. This is an authored diagnostic, not reopening
the original-game ledger or consuming another Stage73 differential claim.
"""
import hashlib,importlib.util,json,signal,sys,time
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage80';S79=ROOT/'stage79'
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
def write(p,value):
 with p.open('x')as f:json.dump(value,f,indent=2);f.write('\n')
def main():
 started=time.monotonic()
 def timeout(a,b):raise RuntimeError('60-second process boundary')
 signal.signal(signal.SIGALRM,timeout);signal.alarm(60)
 resolution=S/'evidence/existing-authorization-resolution.json';r=json.loads(resolution.read_text());assert r['status']=='EXISTING_AUTHORIZATION_SCOPE_CLARIFIED_NOT_NEW_USER_APPROVAL'
 approval=ROOT/r['existing_evidence'];assert sha(approval)==r['existing_evidence_sha256'];text=approval.read_text();assert 'User verbatim: 允许，开始吧' in text and 'same-scope diagnostics/retries' in text and 'official Unicorn 2.1.4' in text
 manifest=S79/'configs/cpu-evidence-manifest.json';assert sha(manifest)==r['manifest_sha256']=='7f05462335e4bcbfa705d372758e579f0d6d53ad4cad6692cc4cda74dde7eb7a';m=json.loads(manifest.read_text());scope=m['execution_scope'];assert scope==dict(id='stage79_cpu_evidence_only_v1',fixed_snapshot='cycle62p19',threads=[1,2,3,4],maximum_processes=1,original_game_instructions=0,authored_probe_instructions_maximum=56,SR_or_game_library_loads=0,thread_execution_or_scheduling=False)
 assert m['limits']==dict(hard_process_seconds=60,each_probe_microseconds=100000,instructions_each_probe=1)
 assert not(S79/'approval.json').exists(),'No new user approval file may be fabricated'
 for row in m['pinned_files']:assert sha(row['path'])==row['sha256'],row['path']
 old=json.loads((ROOT/'evidence/stage55-batch-ledger.json').read_text());assert len(old['executions'])==20 and old['remaining_processes']==80 and old['goal_closed']is True
 claim=S79/'evidence/cpu-evidence-attempt.claim.json';out=S79/'runs/cpu-evidence-once'
 write(claim,dict(manifest_sha256=sha(manifest),authorization_resolution_sha256=sha(resolution),existing_authorization_source=str(approval),authorization_is_new_user_approval=False,charged_authored_diagnostic_attempt=True,maximum_processes=1,retry_allowed=False,original_game_instructions_maximum=0,old_ledger_reopened=False,start_UTC=time.time()));out.mkdir(parents=True,exist_ok=False)
 try:
  source=S79/'tools/cpu_evidence_once.py';spec=importlib.util.spec_from_file_location('fixed_stage79_authored_evidence',source);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
  result=module.evidence(m);result.update(manifest_sha256=sha(manifest),authorization_resolution_sha256=sha(resolution),authorization_from_existing_scope=True,new_user_approval_fabricated=False,old_ledger_modified=False,duration_seconds=round(time.monotonic()-started,3));write(out/'result.json',result);print(json.dumps({k:result[k]for k in ['status','authored_instruction_maximum','original_game_instructions','snapshot_threads_executed_or_scheduled','old_ledger_modified','duration_seconds']}))
 except BaseException as e:
  partial={};tb=e.__traceback__
  while tb:
   if tb.tb_frame.f_code.co_name=='evidence':
    values=tb.tb_frame.f_locals;partial={k:values[k]for k in ['outputs','probes','record','segment','target','opcode','events','code','error']if k in values}
   tb=tb.tb_next
  write(out/'failure.json',dict(status='AUTHORED_CPU_EVIDENCE_ATTEMPT_FAILED',error=repr(e),partial_evidence=partial,charged_authored_diagnostic_attempt=True,retry_allowed=False,original_game_instructions=0,old_ledger_modified=False,duration_seconds=round(time.monotonic()-started,3)));raise
if __name__=='__main__':main()
