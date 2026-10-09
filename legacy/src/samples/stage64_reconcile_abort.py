"""Record child SIGABRT without retrying or altering checkpoint/input history."""
import copy,fcntl,json,os
from pathlib import Path
from stage54_batch import LEDGER,sha,atomic,finish,validate,utc
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
CRASH=Path('<LOCAL_MACHINE_PATH>')
def main():
    runner=E/'stage63-run20-bounded-cycle.result.json';run=json.loads(runner.read_text())
    assert run['returncode']==-6 and not run['timed_out'] and run['duration_seconds']<60
    journal=E/'stage63-cycle63p20-timing.json';j=json.loads(journal.read_text())
    assert j[-1]['kind']=='RUNTIME_IMPORT_END' and not any(q['kind'] in ['CONSTRUCTION_END','RESTORE_BEGIN','GUEST_BEGIN'] for q in j)
    text=CRASH.read_text();head,body=text.split('\n',1);h=json.loads(head);c=json.loads(body)
    assert c['pid']==94111 and c['exception']['signal']=='SIGABRT'
    thread=c['threads'][c['faultingThread']];frames=thread['frames']
    assert any(q.get('symbol')=='s26_window_new' for q in frames)
    assert any(q.get('symbol')=='___RegisterApplication_block_invoke' for q in frames)
    try:os.kill(c['pid'],0)
    except ProcessLookupError:pid_absent=True
    else:raise AssertionError('aborted process still exists')
    old=json.loads((E/'stage64-ledger-before-timeout-recovery.json').read_text())
    with LEDGER.with_name(LEDGER.name+'.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX);l=json.loads(LEDGER.read_text());validate(l)
        assert len(l['executions'])==20 and l['remaining_processes']==80
        assert l['executions'][-1]['status']=='claimed' and l['executions'][-1]['tag']=='cycle63p20'
        assert l['executions'][:19]==old['executions'] and l['watch_state']==old['watch_state']
        assert l['latest_checkpoint']=='cycle62p19'
        for f in l['executions'][18]['checkpoint_files']:assert sha(ROOT/f['path'])==f['sha256']
        assert not (ROOT/'runs/stage12-state-cycle63p20').exists()
        assert not (ROOT/'runs/stage12-state-cycle63p20.stage63-pending').exists()
        with (E/'stage64-ledger-before-abort-reconciliation.json').open('xb') as f:f.write(LEDGER.read_bytes())
        watch=copy.deepcopy(l['watch_state'])
    minimal=dict(source_path=str(CRASH),source_sha256=sha(CRASH),app_name=h.get('app_name'),timestamp=h.get('timestamp'),
        pid=c['pid'],exception=c['exception'],termination=c['termination'],abort_annotation=c.get('asi'),
        relevant_stack=[dict(symbol=q.get('symbol'),image=c['usedImages'][q['imageIndex']].get('name')) for q in frames[:17]],
        no_original_instruction_frame=True,precise_low_level_registration_failure_cause_unknown=True)
    (E/'stage64-run20-native-crash-summary.json').write_text(json.dumps(minimal,indent=2)+'\n')
    result=dict(status='failed',boundary=dict(kind='Native_process_SIGABRT_before_restore64',signal=6,pid=c['pid'],
        location='s26_window_new -> NSApplication.sharedApplication -> HIServices RegisterApplication -> abort',
        runner_elapsed_seconds=run['duration_seconds'],whole_process_hard_timeout=False,
        original_guest_started=False,complete_cold_restore_passed=False,normal_process_exit_proven=False))
    finish(20,result,watch,None,None)
    with LEDGER.with_name(LEDGER.name+'.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX);l=json.loads(LEDGER.read_text());validate(l)
        q=l['executions'][19];assert q['status']=='finished' and q['result']==result
        q['whole_process_outcome']=dict(status='signal_abort',signal=6,pid=c['pid'],normal_process_exit_proven=False,
            hard_process_seconds=60,runner_elapsed_seconds=run['duration_seconds'],runner_result_path=str(runner.relative_to(ROOT)),
            runner_result_sha256=sha(runner),journal_path=str(journal.relative_to(ROOT)),journal_sha256=sha(journal),
            crash_summary_path='evidence/stage64-run20-native-crash-summary.json',crash_summary_sha256=sha(E/'stage64-run20-native-crash-summary.json'),
            checkpoint_saved=False,original_guest_started=False,pid_absent=pid_absent)
        l['stop_reason']='native SIGABRT during constructor before cold restore; no further original process'
        assert l['goal_closed'] and l['executions'][:19]==old['executions'] and l['watch_state']==old['watch_state']
        assert l['latest_checkpoint']=='cycle62p19' and l['remaining_processes']==80
        atomic(LEDGER,l)
    out=dict(status='closed_after_constructor_SIGABRT',used=20,remaining=80,latest='cycle62p19',phase=22,
        advances=17,entries=17,tail=0,prior_19_records_preserved=True,checkpoint_hashes_unchanged=True,
        state_and_input_not_reset=True,new_checkpoint=False,original_guest_started=False,original_DLL_executed=False,
        no_run21=True,PID_absent=pid_absent,ledger_sha256=sha(LEDGER),result=result)
    (E/'stage64-abort-reconciliation.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out))
if __name__=='__main__':main()
