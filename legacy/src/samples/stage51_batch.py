"""Own persistent execution budget. Claim before any original instruction run."""
import json,datetime,fcntl,hashlib
from pathlib import Path
R=Path(__file__).resolve().parents[1]
LEDGER=R/'evidence/stage51-batch-ledger.json'
def claim(purpose,resume,tag,runtime_argv,path=LEDGER):
    with Path(path).open('r+')as f:
        fcntl.flock(f,fcntl.LOCK_EX);x=json.load(f)
        assert x['maximum_original_executions']==10 and x['user_approval_utc']=='2026-10-07 05:21:04 UTC'
        assert not x.get('goal_closed',False),'loader returned; no further original execution within this goal'
        assert len(x['executions'])<10,'approved original execution budget exhausted'
        assert all(q['tag']!=tag for q in x['executions']),'checkpoint tag already used'
        index=len(x['executions'])+1
        sources={str(p.relative_to(R)):hashlib.sha256(p.read_bytes()).hexdigest()for p in sorted((R/'samples').glob('*51*'))if p.is_file()}
        x['executions'].append(dict(index=index,purpose=purpose,resume=resume,tag=tag,runtime_argv=runtime_argv,source_hashes=sources,claimed_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),status='claimed_before_original_execution',conservatively_consumes_one_execution=True))
        x['remaining_original_executions']=10-index;f.seek(0);json.dump(x,f,indent=2);f.truncate();f.flush()
        return index
def finish(index,result,detail,path=LEDGER):
    with Path(path).open('r+')as f:
        fcntl.flock(f,fcntl.LOCK_EX);x=json.load(f);q=x['executions'][index-1];assert q['index']==index and q['status']=='claimed_before_original_execution'
        q.update(status='finished',finished_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),result=result,detail=detail)
        if result.get('status')=='original_loader_returned':x.update(goal_closed=True,stop_reason='original loader naturally returned; do not execute further caller instructions or city ticks')
        f.seek(0);json.dump(x,f,indent=2);f.truncate();f.flush()
