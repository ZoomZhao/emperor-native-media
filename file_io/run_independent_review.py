#!/usr/bin/env python3
"""Run only reviewer-written tests plus fixed self-authored reader with system zlib."""
import sys
import argparse,hashlib,json,os,subprocess,time
from pathlib import Path
R=Path(__file__).resolve().parent
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);args=ap.parse_args()
if sys.flags.optimize:
 ap.error('Python optimization disables mandatory validation checks; use unoptimized Python')
out=args.output.resolve()
if not args.output.is_absolute() or out.exists():ap.error('output must be new absolute directory')
out.mkdir(parents=True);commands=[];start=time.monotonic()
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
cap={'commit':'16b4c8c8489b9003db8b31e1a66f9275bf52e91b','parent':'79726e882de14e6195816a806cef93b5aedb5ac1'}
known_cpp={'src/strict_zlib.cpp':'1c13bf06ba9721a85000105f4b18763f88bfadf71626f2cae1127288c6fb2f0e','src/framed_reader.cpp':'648bb362ff03f96afa0596cc5c84e628b1b666d78325d41099a70181eab5d68b'}
assert all(sha(R/path)==expected for path,expected in known_cpp.items()), 'core CPP differs from fixed review baseline'
sdk=subprocess.check_output(['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'],text=True).strip()
def run(argv,name,timeout=60,env=None):
 t=time.monotonic()
 try:
  p=subprocess.run([str(x) for x in argv],cwd=R,capture_output=True,text=True,timeout=timeout,env=env)
  stdout,stderr,code=p.stdout,p.stderr,p.returncode;timedout=False
 except subprocess.TimeoutExpired as error:
  stdout,stderr=error.stdout or '',error.stderr or '';code=124;timedout=True
  if isinstance(stdout,bytes):stdout=stdout.decode(errors='replace')
  if isinstance(stderr,bytes):stderr=stderr.decode(errors='replace')
 (out/(name+'.stdout')).write_text(stdout);(out/(name+'.stderr')).write_text(stderr)
 item={'argv':[str(x) for x in argv],'exit_code':code,'timeout_seconds':timeout,'timed_out':timedout,'elapsed_seconds':round(time.monotonic()-t,6),'stdout':name+'.stdout','stderr':name+'.stderr'}
 commands.append(item);return code,stdout,stderr
base=['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror','-isysroot',sdk,'-I',R/'include',R/'src/strict_zlib.cpp',R/'src/framed_reader.cpp']
results=[]
for mode,flags in [('release',['-O2']),('undefined',['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all'])]:
 exe=out/('independent-'+mode)
 code,_,_=run(base+flags+[R/'tests/independent_reader_tests.cpp','-lz','-o',exe],'build-'+mode);assert code==0
 code,stdout,stderr=run([exe],'run-'+mode,15)
 rows=[json.loads(line) for line in stdout.splitlines()];summary=rows[-1]
 results.append({'mode':mode,'exit_code':code,'checks':rows[:-1],'summary':summary,'sanitizer_diagnostics':stderr,'binary_sha256':sha(exe)})
 assert code==0 and summary['failed']==0 and not stderr
# Isolated alias/lifetime probe, never mixed into normal-input passes. It observes
# deallocation and refuses expired input before reading, without sanitizer need.
exe=out/'stream-alias-repro'
code,_,_=run(base+['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all',R/'tests/stream_alias_lifetime_probe.cpp','-lz','-o',exe],'build-stream-alias');assert code==0
code,stdout,stderr=run([exe],'run-stream-alias',15)
observation=json.loads(stdout)
alias={'classification':'conditional FramedReader borrowed-source/output-storage alias lifetime hazard; direct adapter overlap separately passes',
 'exit_code':code,'deallocation_and_prevented_expired_read':observation,'actual_expired_memory_read':False,'stderr':'run-stream-alias.stderr',
 'source_precondition_required':'stream backing storage must stay alive and disjoint from the output vector modified bynext(); this is now explicit in public header/README; negativeprobe deliberately exercises unsupported pattern',
 'qualified_disjoint_input_regression_failed':False,'binary_sha256':sha(exe)}
assert code==0 and observation['borrowed_storage_freed_after_first'] and observation['expired_read_prevented'] and not stderr
smoke=out/'asan-smoke'
code,_,_=run(['/usr/bin/clang++','-std=c++17','-isysroot',sdk,'-fsanitize=address',R/'tests/asan_smoke.cpp','-o',smoke],'build-asan-smoke');assert code==0
environment=dict(os.environ);environment['ASAN_OPTIONS']='detect_leaks=0:halt_on_error=1'
code,_,stderr=run([smoke],'run-asan-smoke',15,environment)
asan={'smoke_exit_code':code,'available':code==0,'stderr':'run-asan-smoke.stderr','initialization_check_failure':'asan_init_is_running' in stderr,'reader_or_codec_linked':False}
receipt={'commit':cap['commit'],'parent':cap['parent'],'elapsed_seconds':round(time.monotonic()-start,6),'commands':commands,'results':results,
 'alias_negative_probe':alias,'asan_environment_diagnostic':asan,'source_hashes':[{'path':name,'sha256':sha(R/name)} for name in ['tests/independent_reader_tests.cpp','tests/stream_alias_lifetime_probe.cpp','run_independent_review.py','tests/asan_smoke.cpp']],
 'original_exe_or_GameData_read':False,'original_or_derived_code_executed':False,'main_repository_modified':False,'network_or_push':False}
(out/'results.json').write_text(json.dumps(receipt,ensure_ascii=False,indent=2)+'\n')
print(json.dumps({'commit':cap['commit'],'modes':[r['summary'] for r in results],'alias_probe':alias,'elapsed_seconds':receipt['elapsed_seconds']},ensure_ascii=False))
