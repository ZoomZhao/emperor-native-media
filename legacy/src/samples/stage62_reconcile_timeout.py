"""Record external hard timeout after inner checkpoint/finish; never rerun originals."""
import copy,fcntl,json
from pathlib import Path
from stage54_batch import LEDGER,atomic,sha,validate,utc
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
def main():
    external=json.loads((E/'stage62-run19-finite-cycle.result.json').read_text())
    assert external['timed_out'] and external['returncode'] is None
    with LEDGER.with_name(LEDGER.name+'.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX);ledger=json.loads(LEDGER.read_text());validate(ledger)
        assert len(ledger['executions'])==19 and ledger['remaining_processes']==81
        assert ledger['latest_checkpoint']=='cycle62p19' and all(q['status']=='finished' for q in ledger['executions'])
        q=ledger['executions'][-1];assert q['index']==19 and q['result']['status']=='wall_budget'
        for f in q['checkpoint_files']:assert sha(ROOT/f['path'])==f['sha256']
        before=copy.deepcopy(ledger)
        with (E/'stage62-ledger-before-hard-timeout-reconciliation.json').open('xb') as f:f.write(LEDGER.read_bytes())
        q['runtime_result_before_process_timeout']=copy.deepcopy(q['result'])
        q['whole_process_outcome']=dict(status='hard_timeout',hard_seconds=60,
            runner_elapsed_seconds=external['duration_seconds'],runner_result_path='evidence/stage62-run19-finite-cycle.result.json',
            runner_result_sha256=sha(E/'stage62-run19-finite-cycle.result.json'),
            original_checkpoint_and_inner_finish_written_before_external_kill=True,
            checkpoint_hashes_verified_after_kill=True,normal_process_exit_proven=False)
        q['result']=dict(status='failed',boundary=dict(kind='Whole_process_hard_timeout62',
            hard_seconds=60,runner_elapsed_seconds=external['duration_seconds'],
            original_API_boundary=False,inner_original_run_status='wall_budget',
            checkpoint_complete=True,normal_process_exit_proven=False))
        ledger.update(goal_closed=True,stop_reason='whole-process hard60 timeout; keep saved checkpoint/counters and hand back before next process')
        ledger['external_timeout_reconciliation']=dict(recorded_utc=utc(),previous_ledger_sha256=sha(E/'stage62-ledger-before-hard-timeout-reconciliation.json'),
            maximum_processes=100,used=19,remaining=81,reset_allowed=False,original_instructions_executed=0)
        for k in ['watch_state','latest_checkpoint','remaining_processes','caps','mode_limits','city_frame_review']:
            assert ledger[k]==before[k],k
        assert ledger['executions'][:-1]==before['executions'][:-1]
        validate(ledger);atomic(LEDGER,ledger)
        out=dict(status='closed_after_external_hard_timeout',used=19,remaining=81,
            latest='cycle62p19',phase=ledger['watch_state']['phase'],advances=len(ledger['watch_state']['advances']),
            entries=ledger['watch_state']['entries'],tail_entries=ledger['watch_state']['tail_entries'],
            checkpoint_complete_verified=True,history_and_counters_preserved=True,
            original_instructions_executed=0,ledger_sha256=sha(LEDGER))
        (E/'stage62-timeout-reconciliation.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out))
if __name__=='__main__':main()
