"""Task-local orchestration: each original child uses existing hard60 runner/gate."""
import argparse,datetime,json,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
def main():
    p=argparse.ArgumentParser();p.add_argument('--max-processes',type=int,required=True);p.add_argument('--seconds',type=int,required=True);a=p.parse_args()
    assert 1<=a.max_processes<=10 and 65<=a.seconds<=480
    started=time.monotonic();rows=[];reason=None
    for attempt in range(a.max_processes):
        if time.monotonic()-started+65>a.seconds:reason='bounded handback interval';break
        ledger=json.loads((E/'stage55-batch-ledger.json').read_text())
        if ledger['goal_closed']:reason=ledger['stop_reason'];break
        assert all(q['status']=='finished' for q in ledger['executions'])
        assert ledger['city_frame_review'] and ledger['remaining_processes']>0
        index=len(ledger['executions'])+1;tag=f'cycle61p{index:02d}'
        label=f'stage61-run{index:02d}-finite-cycle'
        command=['/usr/bin/python3','samples/run_bounded.py',label,'.',
                 '/usr/bin/python3','samples/checkpoint_startup61.py','--mode','cycle',
                 '--resume',ledger['latest_checkpoint'],'--tag',tag]
        process=subprocess.run(command,cwd=str(ROOT),timeout=64)
        after=json.loads((E/'stage55-batch-ledger.json').read_text());w=after['watch_state']
        row=dict(index=index,returncode=process.returncode,used=len(after['executions']),
            remaining=after['remaining_processes'],closed=after['goal_closed'],latest=after['latest_checkpoint'],
            phase=w['phase'],advances=len(w['advances']),entries=w['entries'],tail_entries=w['tail_entries'],
            result=after['executions'][-1].get('result',{}).get('status'),
            boundary=after['executions'][-1].get('result',{}).get('boundary'))
        rows.append(row);print(json.dumps(row),flush=True)
        (E/'stage61-cycle-driver-progress.json').write_text(json.dumps(dict(rows=rows,elapsed_seconds=time.monotonic()-started),indent=2)+'\n')
        if process.returncode or after['goal_closed'] or any(q['status']!='finished' for q in after['executions']):
            reason='child failure, closed goal/cap or unresolved process';break
    else:reason='bounded orchestration process count reached'
    (E/'stage61-cycle-driver-result.json').write_text(json.dumps(dict(rows=rows,reason=reason,
        elapsed_seconds=time.monotonic()-started,all_original_children_hard60=True),indent=2)+'\n')
    print(json.dumps(dict(stopped=reason,children=len(rows))),flush=True)
if __name__=='__main__':main()
