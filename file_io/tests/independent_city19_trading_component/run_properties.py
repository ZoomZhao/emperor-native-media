#!/usr/bin/env python3
"""Build and run only the new Trading increment properties; no original inputs."""
import argparse,hashlib,json,subprocess,sys,time
if sys.flags.optimize:
    raise SystemExit("assertions must remain enabled; do not use python -O")
from pathlib import Path
ROOT=Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
p.add_argument('--source-root',type=Path,required=True)
a=p.parse_args();out=a.output.resolve()
if out.exists():p.error('fresh output required')
out.mkdir(parents=True);start=time.monotonic();commands=[];results=[]
def run(argv,label,timeout=10):
    remaining=180-(time.monotonic()-start)
    if remaining<=0:raise RuntimeError('180-second total budget')
    q=subprocess.run([str(x) for x in argv],capture_output=True,text=True,timeout=min(timeout,remaining))
    (out/(label+'.stdout')).write_text(q.stdout);(out/(label+'.stderr')).write_text(q.stderr)
    commands.append(dict(argv=[str(x) for x in argv],exit_code=q.returncode,stdout_sha256=hashlib.sha256(q.stdout.encode()).hexdigest(),stderr_sha256=hashlib.sha256(q.stderr.encode()).hexdigest()))
    return q
q=run(['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'],'sdk',30)
if q.returncode:raise RuntimeError('SDK unavailable')
sdk=q.stdout.strip()
for mode in ['release','ubsan']:
    exe=out/('trading-properties-'+mode)
    flags=['-O2'] if mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined']
    q=run(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-isysroot',sdk,'-I',a.source_root.resolve()/'file_io/include',ROOT/'trading_properties.cpp','-o',exe],mode+'-build',60)
    if q.returncode:raise RuntimeError('independent component properties build failed')
    q=run([exe],mode+'-properties');d=json.loads(q.stdout) if q.stdout else None
    if d is None:raise RuntimeError('component properties returned no report')
    results.append(dict(mode=mode,exit_code=q.returncode,stderr_empty=q.stderr=='',report=d))
result=dict(passed=sum(q['report']['passed'] for q in results),failed=sum(q['report']['failed'] for q in results),
            execution_failed=any(q['exit_code'] or not q['stderr_empty'] for q in results),results=results,commands=commands,
            component_sha256=hashlib.sha256((a.source_root.resolve()/'file_io/include/city19_goal_archive.hpp').read_bytes()).hexdigest(),
            test_sha256=hashlib.sha256((ROOT/'trading_properties.cpp').read_bytes()).hexdigest(),elapsed_seconds=time.monotonic()-start,
            total_budget_seconds=180,original_payload_or_EXE_read=False,original_or_derived_game_executed=False,new_REA_query=False,main_written=False,pushed=False)
(out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k] for k in ['passed','failed','execution_failed','elapsed_seconds']}))
raise SystemExit(bool(result['failed'] or result['execution_failed']))
