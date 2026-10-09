"""Own synthetic100-process budget and unchanged scheduler caps; no original tool imports."""
import copy,hashlib,json,tempfile
from pathlib import Path
from stage54_watch import Watch,WatchStop,initial_state
from stage54_batch import draft,claim,finish,GateError,SOURCE_SHA,R,sha
old_real=R/'evidence/stage51-batch-ledger.json';old_sha=sha(old_real);cases=[]
def refuses(name,fn,error=WatchStop,kind=None):
 try:fn()
 except error as e:
  if kind:assert e.details['kind']==kind
  cases.append(dict(name=name,passed=True))
 else:raise AssertionError(name)
def caller():
 w=Watch('caller');w.caller_branch(1,0x401adc)
 refuses('real callback return pause',lambda:w.caller_return(1,0x44a7e2,0x700ffaa4,1,1),kind='Caller_return_goal_stop54')
 w.mode='cycle';return w
def step(w,advance=True):
 p=w.state['phase'];w.scheduler_entry(1,0x1000,0x5371c4,p,0)
 if advance and p==50:w.wrap_jump(1,0x1000,0);w.tail_entry(1,0x1000,0x5371c4,0)
 w.returns(1,0x5371c4,0x1004,(p+1)%51 if advance else p)
w=caller()
for i in range(45):step(w)
refuses('first tail cap immediately stops after46 advances',lambda:step(w),kind='Finite_scheduler_cap_stop54')
assert len(w.state['advances'])==46 and w.state['tail_returns']==1 and w.state['phase']==0 and not w.state['full_phase_cycle']
refuses('no further scheduler after first tail despite100 processes',lambda:step(w),kind='Scheduler_cap_before_entry54')
refuses('second tail forbidden',lambda:w.tail_entry(1,0x1000,0x5371c4,0),kind='Second_cycle_tail_forbidden54')
again=Watch('cycle',w.snapshot());assert again.state['tail_returns']==1
refuses('tail cap persists through checkpoint',lambda:step(again),kind='Scheduler_cap_before_entry54')
bad=w.snapshot();bad['full_phase_cycle']=True
refuses('46-to0 cannot become full cycle',lambda:Watch('cycle',bad),ValueError)
w=caller()
for i in range(127):step(w,False)
refuses('128-entry empty progression cap unchanged',lambda:step(w,False),kind='Finite_scheduler_cap_stop54')
refuses('129-entry refused with no advancing phases',lambda:step(w,False),kind='Scheduler_cap_before_entry54')
# Independent literal51 cap test on a correctly shaped synthetic completed sequence.
s=initial_state();s.update(entries=51,advances=[dict(before=(5+i)%51,after=(6+i)%51)for i in range(51)],
                         phase=5,tail_entries=1,tail_returns=1,caller_zero_branch=True,caller_accepted=True,full_phase_cycle=True)
w=Watch('cycle',s)
refuses('51 advancement cap unchanged',lambda:step(w),kind='Scheduler_cap_before_entry54')
with tempfile.TemporaryDirectory(prefix='stage55-own100-',dir=R/'runs')as temp:
 d=Path(temp);old=d/'old.txt';old.write_text('OWN FIXTURE ONLY');lp=d/'ledger.json';ap=d/'approval.json'
 l=draft(sha(old),{});l['prepared_manifest_sha256']=sha(old);lp.write_text(json.dumps(l))
 a=dict(status='approved',source_thread_id='01a10af1-425e-75ae-bded-bf47f843a029',scope_id='stage55-approved100-cold-caller-frame-finite-cycle',maximum_processes=100,hard_process_seconds=60,source_EXE_sha256=SOURCE_SHA,old_remaining_must_not_be_repurposed=True,no_original_DLL_execution=True,source_read_only=True,task_only_writes=True,no_upload=True,no_host_settings_changes=True,source_PCM_to_device=False,caps=l['caps'],prepared_manifest_sha256=l['prepared_manifest_sha256'],user_reply='允许，一次性批准 100 次',user_approval_utc='2026-10-07T07:00:00+00:00',approval_request_text='SYNTHETIC FIXTURE ONLY, NOT ACTUAL APPROVAL',synthetic_fixture=True)
 ap.write_text(json.dumps(a))
 def call(mode,tag):return claim(mode,json.loads(lp.read_text())['latest_checkpoint'],tag,['OWN FIXTURE'],lp,ap,old,check_pins=False)
 idx,_=call('restore','ownrestore');finish(idx,dict(status='cold_restore_passed'),initial_state(),path=lp,old_path=old)
 idx,_=call('caller','owncaller');state=caller().snapshot();files=[]
 for name in ('context','memory','state'):
  q=d/name;q.write_text('OWN FIXTURE ONLY');files.append(dict(path=str(q.relative_to(R)),bytes=q.stat().st_size,sha256=sha(q)))
 finish(idx,dict(status='refused',boundary=dict(kind='Caller_return_goal_stop54')),state,'owncaller',files,lp,old)
 refuses('completed caller is not repeated',lambda:call('caller','repeatcaller'),GateError)
 for i in range(98):
  tag='ownframe'+str(i);idx,_=call('frame',tag);finish(idx,dict(status='wall_budget'),state,tag,files,lp,old)
 final=json.loads(lp.read_text());assert len(final['executions'])==100 and final['remaining_processes']==0 and final['goal_closed']
 refuses('101st process rejected',lambda:call('frame','own101'),GateError)
 cases.append(dict(name='exact100 total claims including cold restore; synthetic only',passed=True))
assert sha(old_real)==old_sha
out=dict(passed=True,cases=cases,case_count=len(cases),original_instructions_executed=0,original_EXE_read=False,emulator_imported=False,native_DLL_loaded=False,synthetic_claims_are_not_actual_processes=True,old_closed_ledger_unchanged=True,process_cap=100,productive_advance_cap=51,tail_cap=1,entry_cap=128,first_tail_stops_at_phase0_partial=True)
(R/'evidence/stage55-own100-cap-probe.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps({k:v for k,v in out.items()if k!='cases'}))
