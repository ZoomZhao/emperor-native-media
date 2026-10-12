#!/usr/bin/env python3
"""Bounded tests of the modern nominal1479 source contract, using only synthetic input."""
import argparse, hashlib, json, os, subprocess, sys, time
from pathlib import Path
ROOT = Path(__file__).resolve().parent
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--source-root', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
if sys.flags.optimize:
    p.error('optimized test invocation unsupported')
out = a.output.resolve()
if out.exists():
    p.error('fresh output directory required')
out.mkdir(parents=True)
source = a.source_root.resolve()
started = time.monotonic()
commands, modes = [], []
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
expect = json.loads((ROOT/'test-expectations.json').read_text())
golden = expect['requests']
if len(golden) != 66 or sum(r['bytes'] for r in golden)+2 != 1461:
    raise ValueError('independent golden request ledger invalid')
(out/'golden_requests.hpp').write_text(
    '#pragma once\nstruct GoldenRequest { unsigned call; size_t bytes,offset; };\n'
    'inline constexpr GoldenRequest golden_requests[]={\n' +
    ''.join('{%s,%d,%d},\n' % (r['source_call'],r['bytes'],r['offset']) for r in golden) + '};\n')
def run(argv, label, timeout):
    remaining = 180-(time.monotonic()-started)
    if remaining <= 0:
        raise RuntimeError('180-second total budget exhausted')
    env = dict(os.environ)
    env['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
    args = [str(x) for x in argv]
    q = subprocess.run(args,capture_output=True,text=True,timeout=min(timeout,remaining),env=env)
    (out/(label+'.stdout')).write_text(q.stdout)
    (out/(label+'.stderr')).write_text(q.stderr)
    commands.append({'argv':args,'exit_code':q.returncode,'stdout_sha256':sha(out/(label+'.stdout')),
                     'stderr_sha256':sha(out/(label+'.stderr'))})
    return q
failure = None
try:
    q = run(['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'],'sdk',30)
    if q.returncode:
        raise RuntimeError('SDK unavailable')
    sdk = q.stdout.strip()
    for mode in ['release','ubsan']:
        exe = out/('properties-'+mode)
        flags = ['-O2'] if mode == 'release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined']
        q = run(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-isysroot',sdk,
                 '-I',source/'file_io/include','-I',out,ROOT/'properties.cpp','-o',exe],mode+'-build',60)
        if q.returncode:
            raise RuntimeError(mode+' build failed')
        q = run([exe],mode+'-properties',10)
        report = json.loads(q.stdout)
        modes.append({'mode':mode,'exit_code':q.returncode,'stderr_empty':not q.stderr,'report':report})
except Exception as e:
    failure = str(e)
files = [source/'file_io/include'/name for name in ['city19_next_owner6_nominal_record.hpp','city19_next_owner6_prefix.hpp','city19_next_owner6_layout.hpp','city19_goal_archive.hpp']]
files += [ROOT/'run.py',ROOT/'properties.cpp',ROOT/'test-expectations.json',out/'golden_requests.hpp']
result = {'baseline_commit':expect['baseline_commit'],'passed':sum(x['report']['passed'] for x in modes),
          'failed':sum(x['report']['failed'] for x in modes),'results':modes,'commands':commands,
          'execution_failed':bool(failure) or any(x['exit_code'] != int(bool(x['report']['failed'])) or not x['stderr_empty'] for x in modes),
          'execution_error':failure,'files':[{'path':str(x),'sha256':sha(x)} for x in files],
          'elapsed_seconds':time.monotonic()-started,'total_budget_seconds':180,
          'compile_timeout_seconds':60,'run_timeout_seconds':10,'input':'synthetic only',
          'original_or_real_payload_read':False,'new_REA_query':False,'main_written':False,'pushed':False}
(out/'results.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
print(json.dumps({k:result[k] for k in ['passed','failed','execution_failed','execution_error','elapsed_seconds']}))
raise SystemExit(bool(result['failed'] or result['execution_failed']))
