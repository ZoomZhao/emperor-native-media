#!/usr/bin/env python3
"""Bounded self-authored FigureFSA new-object tests; no original inputs or executables."""
import argparse,datetime,hashlib,json,os,shlex,subprocess,time
from pathlib import Path

ap=argparse.ArgumentParser()
ap.add_argument('--output',type=Path,required=True)
ap.add_argument('--dependency-root',type=Path)
args=ap.parse_args()
root=Path(__file__).resolve().parents[3]
dependency=(args.dependency_root or root).resolve()
output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
env={**os.environ,'HOMEBREW_NO_AUTO_UPDATE':'1','HOMEBREW_NO_ANALYTICS':'1'}
source=Path(__file__).parent/'properties.cpp'
# dependency-root is a fallback: quoted adjacent includes may win.
# Fingerprint Clang-selected dependencies, never assumed search roots.
compiled_dependencies=set()
records=[];results=[];started=time.monotonic()
def run(argv,label,timeout):
    t=time.monotonic()
    p=subprocess.run(argv,cwd=root,env=env,capture_output=True,timeout=timeout)
    (output/(label+'.stdout')).write_bytes(p.stdout)
    (output/(label+'.stderr')).write_bytes(p.stderr)
    records.append({'argv':argv,'label':label,'exit_code':p.returncode,'seconds':time.monotonic()-t,
        'stdout_sha256':hashlib.sha256(p.stdout).hexdigest(),'stderr_sha256':hashlib.sha256(p.stderr).hexdigest()})
    return p
sdk=run(['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'],'sdk',10)
if sdk.returncode:raise SystemExit('SDK lookup failed; saved logs')
for mode,flags in [('release',['-O2']),('ubsan',['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined'])]:
    exe=output/('properties-'+mode)
    deps=output/(mode+'.d')
    command=['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,
        '-isysroot',sdk.stdout.decode().strip(),'-I',str(root/'file_io/include'),
        '-I',str(dependency/'file_io/include'),'-MMD','-MF',str(deps),str(source),'-o',str(exe)]
    build=run(command,mode+'-compile',40)
    if build.returncode:results.append({'mode':mode,'compile_exit':build.returncode});continue
    deptext=deps.read_text().replace('\\\n',' ')
    selected=[Path(p).resolve() for p in shlex.split(deptext.split(':',1)[1])]
    compiled_dependencies.update(selected)
    test=run([str(exe)],mode+'-test',20)
    results.append({'mode':mode,'compile_exit':0,'exit_code':test.returncode,
        'stderr_empty':not test.stderr,
        'compiled_dependency_sha256':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in selected},
        'report':json.loads(test.stdout) if test.stdout else None})
report={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'results':results,'commands':records,
    'source_sha256':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted({source.resolve(),Path(__file__).resolve(),*compiled_dependencies})},
    'requested_dependency_root':str(dependency),
    'elapsed_seconds':time.monotonic()-started,'self_authored_input_only':True,
    'original_code_or_resources_loaded':False,'actual_file_read':False,
    'selected_contract':'exact_fsa_classes_root7b5bc8_reader4dad10','verifies_original_instance':False}
(output/'results.json').write_text(json.dumps(report,indent=2)+'\n')
ok=len(results)==2 and all(r.get('exit_code')==0 and r.get('stderr_empty') for r in results)
print(json.dumps({'passed':ok,'results':results,'output':str(output)},indent=2))
raise SystemExit(0 if ok else 1)
