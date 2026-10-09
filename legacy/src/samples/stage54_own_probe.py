"""Pure synthetic counter/budget negatives; no original image, engine or DLL imports."""
import copy, hashlib, json, tempfile
from pathlib import Path
from stage54_watch import Watch, WatchStop, EmulationGate, initial_state
from stage54_batch import draft, claim, finish, record_frame_review, GateError, SOURCE_SHA

R=Path(__file__).resolve().parents[1]
old_real=R/'evidence/stage51-batch-ledger.json'
old_real_sha=hashlib.sha256(old_real.read_bytes()).hexdigest()
cases=[]
def rejects(name, fn, kind=None, error=WatchStop):
    try: fn()
    except error as e:
        if kind: assert e.details['kind']==kind,(name,e)
        cases.append(dict(name=name,passed=True,refused=True))
    else: raise AssertionError(name+' must refuse')
def passed(name): cases.append(dict(name=name,passed=True))

def accepted():
    w=Watch('caller');w.caller_branch(1,0x401adc)
    rejects('caller real return pause',lambda:w.caller_return(1,0x44a7e2,0x700ffaa4,1,1),'Caller_return_goal_stop54')
    w.mode='cycle';return w

def step(w, advance=True):
    phase=w.state['phase'];w.scheduler_entry(1,0x1000,0x5371c4,phase,0)
    after=(phase+1)%51 if advance else phase
    if advance and phase==50:
        w.wrap_jump(1,0x1000,0);w.tail_entry(1,0x1000,0x5371c4,0)
    w.returns(1,0x5371c4,0x1004,after)

rejects('restore original event',lambda:Watch('restore').scheduler_entry(1,0x1000,0x5371c4,5,0),
        'Original_execution_forbidden_during_restore54')
rejects('caller wrong stack',lambda:Watch('caller').caller_return(1,0x44a7e2,0x700ffaa0,1,1),
        'Caller_return_or_state_mismatch54')
rejects('caller no real branch',lambda:Watch('caller').caller_return(1,0x44a7e2,0x700ffaa4,1,1),
        'Caller_return_or_state_mismatch54')
w=Watch('caller');w.caller_branch(1,0x401adc)
rejects('caller old menu screen',lambda:w.caller_return(1,0x44a7e2,0x700ffaa4,13,1),
        'Caller_return_or_state_mismatch54')
w=accepted()
rejects('scheduler wrong caller',lambda:w.scheduler_entry(1,0x1000,0x401aa8,5,0),
        'Scheduler_caller_gate_or_phase_mismatch54')
rejects('scheduler gate nonzero',lambda:w.scheduler_entry(1,0x1000,0x5371c4,5,1),
        'Scheduler_caller_gate_or_phase_mismatch54')
rejects('scheduler wrong thread',lambda:w.scheduler_entry(2,0x1000,0x5371c4,5,0),
        'Scheduler_caller_gate_or_phase_mismatch54')
step(w,False);assert w.state['entries']==1 and not w.state['advances'];passed('precheck early return does not count advancement')
w.scheduler_entry(1,0x1000,0x5371c4,5,0)
rejects('wrong stack does not count return',lambda:w.returns(1,0x5371c4,0x1000,6),
        'Observed_return_stack_mismatch54')
w.returns(2,0x5371c4,0x1004,6);assert not w.state['advances'];passed('foreign-thread return does not count')
rejects('phase jump is not progression',lambda:w.returns(1,0x5371c4,0x1004,7),
        'Unexpected_original_phase_sequence54')
w=accepted()
for i in range(46):step(w)
assert len(w.state['advances'])==46 and w.state['phase']==0 and w.state['tail_returns']==1
assert not w.state['full_phase_cycle'];passed('phase5 to0 is partial, not full cycle')
snapshot=w.snapshot();w=Watch('cycle',snapshot)
assert len(w.state['advances'])==46 and w.state['tail_returns']==1;passed('counts persist across checkpoint reload')
for i in range(4):step(w)
rejects('51st natural advancement pauses before caller',lambda:step(w),'Finite_scheduler_cap_stop54')
assert w.state['phase']==5 and len(w.state['advances'])==51 and w.state['tail_returns']==1 and w.state['full_phase_cycle']
passed('full cycle requires51 ordered productive advances plus real tail return')
rejects('52nd advancement blocked before entry',lambda:step(w),'Scheduler_cap_before_entry54')
assert len(w.state['advances'])==51
rejects('second tail blocked',lambda:w.tail_entry(1,0x1000,0x5371c4,0),'Second_cycle_tail_forbidden54')
bad=copy.deepcopy(snapshot);bad['full_phase_cycle']=True
rejects('partial mislabeled full rejected on restore',lambda:Watch('cycle',bad),error=ValueError)
bad=copy.deepcopy(snapshot);bad['advances'][5]['after']=12
rejects('serialized phase sequence corruption',lambda:Watch('cycle',bad),error=ValueError)
w=accepted()
rejects('tail cannot be called independently',lambda:w.tail_entry(1,0x1000,0x5371c4,0),
        'Tail_without_original_shared_stack_jump54')
for i in range(45):step(w)
w.scheduler_entry(1,0x1000,0x5371c4,50,0)
rejects('tail requires observed jump, not address alone',lambda:w.tail_entry(1,0x1000,0x5371c4,0),
        'Tail_without_original_shared_stack_jump54')
w.wrap_jump(1,0x1000,0)
rejects('tail cannot invent nested call stack',lambda:w.tail_entry(1,0xffc,0x4ac4e7,0),
        'Tail_without_original_shared_stack_jump54')
w=accepted()
for i in range(127):step(w,False)
rejects('128th early-return entry stops empty progression',lambda:step(w,False),'Finite_scheduler_cap_stop54')
assert w.state['entries']==128 and not w.state['advances']
rejects('129th entry blocked',lambda:step(w,False),'Scheduler_cap_before_entry54')

w=accepted();w.mode='frame';w.state['baseline_primary_sha256']='old-primary'
event=dict(source='only original successful primary Blt pixels',method='2.5',PNG_path='own.png',
           stats=dict(GPU_byte_mismatches=0,actual_drawable_presentations=1),
           geometry=dict(client_w=1024,client_h=768))
w.primary_present(1,1,event,'new-primary');assert not w.state['frame_candidate'];passed('pixels outside original refresh cannot pass')
w.refresh_entry(1,0x2000,0x400100,1)
w.primary_present(1,13,event,'new-primary');assert not w.state['frame_candidate'];passed('menu pixels cannot pass')
w.primary_present(1,1,event,'old-primary');assert w.state['frames'][-1]['candidate'] is None;passed('restored identical primary cannot pass')
bad=copy.deepcopy(event);bad['source']='host restored primary'
rejects('host presentation not original Blt',lambda:w.primary_present(1,1,bad,'new-primary'),
        'Unproven_original_primary_present54')
bad=copy.deepcopy(event);bad['geometry']['client_w']=800
rejects('wrong city canvas rejected',lambda:w.primary_present(1,1,bad,'new-primary'),
        'Unproven_original_primary_present54')
w.primary_present(1,1,event,'new-primary')
rejects('fresh frame waits for real refresh return',lambda:w.returns(1,0x400100,0x2004,5),
        'Fresh_city_frame_candidate_stop54')
assert w.state['frame_candidate']['semantic_city_content_reviewed'] is False;passed('fresh candidate never auto-claims city content')
g=EmulationGate();called=[];guarded=g.wrap(lambda:called.append('OWN FIXTURE ONLY'))
rejects('engine call blocked during restore',guarded,'Emulation_disabled_before_run_or_during_restore54')
assert not called and g.calls==0
g.allowed=True;guarded();assert g.calls==1 and len(called)==1;passed('gate delegates only to own fake callable when enabled')

with tempfile.TemporaryDirectory(prefix='stage54-own-fixture-',dir=R/'runs') as tmp:
    d=Path(tmp);old=d/'own-old-ledger.txt';old.write_text('SYNTHETIC; no original ledger changes\n')
    old_sha=hashlib.sha256(old.read_bytes()).hexdigest();lp=d/'own-ledger.json';ap=d/'own-approval-fixture.json'
    l=draft(old_sha,{});l['prepared_manifest_sha256']=hashlib.sha256(b'OWN SYNTHETIC MANIFEST').hexdigest()
    lp.write_text(json.dumps(l))
    def call(mode,tag):
        latest=json.loads(lp.read_text())['latest_checkpoint']
        return claim(mode,latest,tag,['OWN FIXTURE'],lp,ap,old,check_pins=False)
    rejects('missing new approval leaves budget untouched',lambda:call('restore','ownrestore'),error=GateError)
    assert not json.loads(lp.read_text())['executions']
    a=dict(status='approved',source_thread_id='01a10af1-425e-75ae-bded-bf47f843a029',
           scope_id='stage54-cold-caller-frame-finite-cycle',maximum_processes=8,hard_process_seconds=60,
           source_EXE_sha256=SOURCE_SHA,old_remaining_must_not_be_repurposed=True,
           no_original_DLL_execution=True,source_read_only=True,task_only_writes=True,
           no_upload=True,no_host_settings_changes=True,source_PCM_to_device=False,caps=l['caps'],
           user_reply='允许',user_approval_utc='2026-10-07T07:00:00+00:00',synthetic_fixture=True,
           fixture_timestamp_is_not_an_actual_user_reply=True,
           approval_request_text='OWN SYNTHETIC NEGATIVE TEST ONLY',prepared_manifest_sha256=l['prepared_manifest_sha256'])
    bad=copy.deepcopy(a);bad['maximum_processes']=9;ap.write_text(json.dumps(bad))
    rejects('broader9-process approval cannot authorize8-process ledger',lambda:call('restore','ownrestore'),error=GateError)
    bad=copy.deepcopy(a);bad['prepared_manifest_sha256']='0'*64;ap.write_text(json.dumps(bad))
    rejects('approval for different prepared source rejected',lambda:call('restore','ownrestore'),error=GateError)
    bad=copy.deepcopy(a);bad['user_approval_utc']='2026-10-07T05:21:04+00:00';ap.write_text(json.dumps(bad))
    rejects('old10-process approval cannot authorize new batch',lambda:call('restore','ownrestore'),error=GateError)
    assert not json.loads(lp.read_text())['executions']
    ap.write_text(json.dumps(a));idx,_=call('restore','ownrestore')
    rejects('unresolved process forbids retry',lambda:call('caller','owncaller'),error=GateError)
    finish(idx,dict(status='cold_restore_passed'),initial_state(),path=lp,old_path=old)
    rejects('second cold restore exceeds its allocation',lambda:call('restore','ownrestore2'),error=GateError)
    idx,_=call('caller','owncaller');watch=accepted().snapshot()
    files=[]
    for name in ('own-context.bin','own-memory.bin','own-state.json'):
        q=d/name;q.write_text('OWN FIXTURE ONLY')
        files.append(dict(path=str(q.relative_to(R)),bytes=q.stat().st_size,sha256=hashlib.sha256(q.read_bytes()).hexdigest()))
    finish(idx,dict(status='refused',boundary=dict(kind='Caller_return_goal_stop54')),watch,
           'owncaller',files,lp,old)
    rejects('cycle requires actual frame review',lambda:call('cycle','owncycle'),error=GateError)
    for j in range(2):
        idx,_=call('frame','ownframe'+str(j))
        if j==0:
            own=d/'own-pixels-fixture.txt';own.write_text('NOT AN IMAGE; SYNTHETIC COUNTER TEST')
            watch['frame_candidate']=dict(PNG_path=str(own),owned_window_capture=dict(own_fixture=True),
                                           semantic_city_content_reviewed=False)
        finish(idx,dict(status='wall_budget'),watch,'ownframe'+str(j),files,lp,old)
    rejects('third frame process forbidden',lambda:call('frame','ownframe2'),error=GateError)
    record_frame_review(dict(passed=True,checkpoint='ownframe1',PNG_path=str(own),
                             PNG_sha256=hashlib.sha256(own.read_bytes()).hexdigest(),
                             city_content_observation='SYNTHETIC FIXTURE, NO GAME CONTENT REVIEW',
                             old_menu_or_restore_presentation=False),lp)
    for j in range(4):
        idx,_=call('cycle','owncycle'+str(j))
        finish(idx,dict(status='wall_budget'),watch,'owncycle'+str(j),files,lp,old)
    final=json.loads(lp.read_text())
    assert len(final['executions'])==8 and final['remaining_processes']==0 and final['goal_closed']
    passed('exact8 process budget includes cold restore and per-mode limits')
    rejects('ninth process forbidden',lambda:call('cycle','owncycle4'),error=GateError)

assert hashlib.sha256(old_real.read_bytes()).hexdigest()==old_real_sha
assert not (R/'evidence/stage54-batch-approval.json').exists()
import sys,checkpoint_startup54
assert 'unicorn' not in sys.modules and 'checkpoint_startup51' not in sys.modules
passed('prepared wrapper import does not import original runtime or emulator')
out=dict(passed=True,cases=cases,case_count=len(cases),original_instructions_executed=0,
         original_EXE_read=False,emulator_imported=False,native_DLL_loaded=False,
         synthetic_fixture_only=True,fixture_approval_is_not_user_approval=True,
         old_production_ledger_unchanged=True,old_production_ledger_sha256=old_real_sha,
         actual_new_approval_received=False,actual_new_runtime_run=False)
(R/'evidence/stage54-own-probe.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n')
print(json.dumps({k:v for k,v in out.items() if k!='cases'}))
