"""Every child independently claims same batch and has complete-process hard60."""
import argparse,json,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
def main():
    p=argparse.ArgumentParser();p.add_argument('--max-processes',type=int,required=True);p.add_argument('--seconds',type=int,required=True);a=p.parse_args()
    assert 1<=a.max_processes<=24 and 65<=a.seconds<=1020
    started=time.monotonic();rows=[];reason=None
    for attempt in range(a.max_processes):
        if time.monotonic()-started+65>a.seconds:reason='bounded handback interval';break
        ledger=json.loads((E/'stage55-batch-ledger.json').read_text())
        if ledger['goal_closed']:reason=ledger['stop_reason'];break
        assert all(q['status']=='finished' for q in ledger['executions'])
        w=ledger['watch_state']
        assert ledger['city_frame_review'] and ledger['remaining_processes']>0
        assert w['tail_returns']==0 and w['tail_entries']<=1 and len(w['advances'])<51 and w['entries']<128
        if w['tail_entries']:
            assert any(f['kind']=='tail' for f in w['frames'])
        index=len(ledger['executions'])+1;tag=f'cycle62p{index:02d}';label=f'stage62-run{index:02d}-finite-cycle'
        command=['/usr/bin/python3','samples/run_bounded.py',label,'.',
                 '/usr/bin/python3','samples/checkpoint_startup62.py','--mode','cycle',
                 '--resume',ledger['latest_checkpoint'],'--tag',tag]
        process=subprocess.run(command,cwd=str(ROOT),timeout=64)
        after=json.loads((E/'stage55-batch-ledger.json').read_text());w=after['watch_state']
        outpath=E/f'stage54-{tag}.json';out=json.loads(outpath.read_text()) if outpath.exists() else {}
        row=dict(index=index,returncode=process.returncode,used=len(after['executions']),remaining=after['remaining_processes'],
            closed=after['goal_closed'],latest=after['latest_checkpoint'],phase=w['phase'],advances=len(w['advances']),
            entries=w['entries'],tail_entries=w['tail_entries'],tail_returns=w['tail_returns'],
            result=after['executions'][-1].get('result',{}).get('status'),
            boundary=after['executions'][-1].get('result',{}).get('boundary'),natural=out.get('natural_state'))
        rows.append(row);print(json.dumps(row),flush=True)
        (E/'stage62-cycle-driver-progress.json').write_text(json.dumps(dict(rows=rows,elapsed_seconds=time.monotonic()-started),indent=2)+'\n')
        if process.returncode or after['goal_closed'] or w['tail_returns'] or any(q['status']!='finished' for q in after['executions']):
            reason='child failure, first tail, closed goal/cap or unresolved process';break
    else:reason='bounded orchestration process count reached'
    (E/'stage62-cycle-driver-result.json').write_text(json.dumps(dict(rows=rows,reason=reason,
        elapsed_seconds=time.monotonic()-started,all_original_children_hard60=True),indent=2)+'\n')
    print(json.dumps(dict(stopped=reason,children=len(rows))),flush=True)
if __name__=='__main__':main()
