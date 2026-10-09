"""Own report enrichment only; never instantiates or runs the original runtime."""
import json,fcntl
from pathlib import Path
R=Path(__file__).resolve().parents[1];ledger=R/'evidence/stage51-batch-ledger.json'
def load(p):return json.loads(Path(p).read_text())
with ledger.open('r+')as f:
 fcntl.flock(f,fcntl.LOCK_EX);x=json.load(f);runs=[];targets=[]
 for q in x['executions']:
  p=R/('evidence/stage51-'+q['tag']+'.json')
  if not p.exists():continue
  r=load(p);logs=[(p,load(p))for p in(R/'evidence').glob('stage51-batch*.result.json')]
  logs=[(p,l)for p,l in logs if '--tag'in l['command']and l['command'][l['command'].index('--tag')+1]==q['tag']]
  assert len(logs)==1,(q['tag'],len(logs));lp,l=logs[0]
  reads=[v for v in r['native_file_read_calls_this_run49']if v['guest_path'].lower().endswith('save\\zoom\\st9base.sav')]
  targets.extend(reads);returns=[v for v in r['city_observations49']if v['kind']=='returned'and v['label']=='city_load']
  q['verified_outcome']=dict(log=str(lp.relative_to(R)),returncode=l['returncode'],timed_out=l['timed_out'],duration_seconds=l['duration_seconds'],status=r['result']['status'],boundary=r['result'].get('boundary'),actual_ST9BASE_ReadFile_calls=len(reads),actual_ST9BASE_read_bytes=sum(v['bytes']for v in reads),max_read_position=max([v['position_after']for v in reads]or[0]),all_read_slices_equal=all(v['source_slice_equal']for v in reads),original_loader_returns=returns,natural_state=r['natural_state'],source_frames=r['native_audio']['source_frames'])
  runs.append(q['verified_outcome'])
 f.seek(0);json.dump(x,f,indent=2);f.truncate();f.flush()
intervals=sorted((v['position_before'],v['position_after'])for v in targets if v['bytes']);merged=[]
for lo,hi in intervals:
 if merged and lo<=merged[-1][1]:merged[-1][1]=max(hi,merged[-1][1])
 else:merged.append([lo,hi])
out=dict(executions_claimed=len(x['executions']),remaining=x['remaining_original_executions'],verified_runs=runs,actual_ST9BASE_ReadFile_calls=len(targets),actual_ST9BASE_read_bytes=sum(v['bytes']for v in targets),unique_read_bytes=sum(hi-lo for lo,hi in merged),read_intervals=merged,all_read_slices_equal=all(v['source_slice_equal']for v in targets),city_loader_success_returned=any(v['EAX']==0 for r in runs for v in r['original_loader_returns']))
(R/'evidence/stage51-batch-progress.json').write_text(json.dumps(out,indent=2));print(json.dumps({k:out[k]for k in ['executions_claimed','remaining','actual_ST9BASE_ReadFile_calls','unique_read_bytes','city_loader_success_returned']}))
