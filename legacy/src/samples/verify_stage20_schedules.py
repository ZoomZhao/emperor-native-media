#!/usr/bin/env python3
import json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];wine=json.loads((ROOT/'evidence/stage20-thread-Wine.json').read_text());assert wine['all_passed'];rows=[]
for quantum in [100,1000,10000]:
 for first in ['main','worker']:
  tag=f'lock{quantum}{first}';cmd=['/usr/bin/python3','-I','samples/checkpoint_stage20.py','--sample','--quantum',str(quantum),'--tag',tag]
  if first=='worker':cmd.append('--worker-first')
  p=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=22);(ROOT/f'evidence/stage20-{tag}.run.log').write_text(p.stdout+p.stderr);assert p.returncode==0,(cmd,p.stderr,p.stdout[-500:]);r=json.loads((ROOT/f'evidence/stage20-{tag}.json').read_text());assert r['assessment']['all_passed']and r['assessment']['checks']==wine['checks'];blocks=[i for i in r['thread_log']if i['action']=='critical_block'];rows.append(dict(quantum=quantum,first_after_creation=first,all_semantic_contracts_match_same_Wine=True,critical_contentions=len(blocks),switches=r['thread_switches'],waits=r['thread_waits'],elapsed_seconds=r['elapsed_seconds'],result=r['result']['status']))
assert any(r['critical_contentions']for r in rows);out=dict(all_passed=True,rows=rows,source_sha256=wine['probe_sha256'],normalized_opaque_IDs_addresses_and_CriticalSection_internal_layout=True,covered_ordering='caller-first and worker-first after atomic CreateThread; caller holding recursive lock vs worker; waits and implicit worker exit',early_worker_before_CreateThread_returns_not_exhaustively_explored=True,all_Windows_preemption_schedules_not_claimed=True,APC_delivery_and_exception_and_priority_unimplemented=True);(ROOT/'evidence/stage20-schedule-regression.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out))
