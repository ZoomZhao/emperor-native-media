#!/usr/bin/python3
"""One bounded local task per invocation. Existing Apple tools only, silent."""
import hashlib,json,os,signal,subprocess,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
TASK=sys.argv[1] if len(sys.argv)==2 else ''
TASKS=('generate','build','surface','audio','video','metal','av','stream','game')
if TASK not in TASKS:raise SystemExit('run.py '+'|'.join(TASKS))
OUT=ROOT/'.local-results'/(TASK+'-'+str(time.time_ns()));OUT.mkdir(parents=True)
DEADLINE=time.monotonic()+55
def run(label,args):
    start=time.monotonic()
    with (OUT/(label+'.stdout.log')).open('wb') as out,(OUT/(label+'.stderr.log')).open('wb') as error:
        child=subprocess.Popen(args,stdout=out,stderr=error,start_new_session=True)
        try:code=child.wait(timeout=max(.1,DEADLINE-time.monotonic()))
        except subprocess.TimeoutExpired:os.killpg(child.pid,signal.SIGKILL);child.wait();code=124
    (OUT/(label+'.status.json')).write_text(json.dumps({'returncode':code,'seconds':time.monotonic()-start})+'\n')
    if code:raise RuntimeError(label+': '+(OUT/(label+'.stderr.log')).read_text())
def compile_program(label,sources,frameworks,output):
    cmd=['/usr/bin/clang++','-std=c++17','-fobjc-arc','-O2','-mmacosx-version-min=15.0','-Wall','-Wextra','-Wpedantic','-Werror']+[str(ROOT/p) for p in sources]
    for framework in frameworks:cmd+=['-framework',framework]
    run(label,cmd+['-o',str(output)])
try:
    if TASK=='generate':
        generated=ROOT/'.generated';generated.mkdir(exist_ok=True);manifest=generated/'source-and-fixtures.sha256'
        if manifest.exists():run('fixture-integrity',['/usr/bin/shasum','-a','256','-c',str(manifest)])
        else:
            movie=generated/'self_authored_8s.mov'
            if movie.exists() or (generated/'reliability-fixtures').exists():raise RuntimeError('incomplete previous generation; archive .generated before retrying')
            compile_program('compile-writer',['stream_lab/generate_fixture.mm','stream_lab/movie_io.mm','video_lab/stream.cpp'],['Foundation','AVFoundation','AudioToolbox','CoreMedia','CoreVideo'],OUT/'generate-fixture')
            run('generate-8s',[str(OUT/'generate-fixture'),str(movie)])
            compile_program('compile-variants',['media_demo/fixtures.mm'],['Foundation','AVFoundation','CoreMedia'],OUT/'fixture-variants')
            run('generate-variants',[str(OUT/'fixture-variants'),str(movie),str(generated/'reliability-fixtures')])
            files=[ROOT/'stream_lab/generate_fixture.mm',ROOT/'stream_lab/movie_io.mm',ROOT/'stream_lab/movie_io.hpp',ROOT/'media_demo/fixtures.mm',movie]
            files+=sorted((generated/'reliability-fixtures').glob('*.mov'))
            manifest.write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+str(p)+'\n' for p in files))
    elif TASK=='build':run('build',['/bin/sh',str(ROOT/'media_demo/build.sh')])
    else:run('test',['/bin/sh',str(ROOT/(TASK+'_lab/run.sh'))])
    summary={'passed':True,'task':TASK,'process_seconds':55-(DEADLINE-time.monotonic()),'silent':True,'logs_directory':str(OUT)}
    (OUT/'summary.json').write_text(json.dumps(summary,indent=2)+'\n');print('PASS '+TASK+' (bounded local task)')
except Exception as error:
    (OUT/'summary.json').write_text(json.dumps({'passed':False,'error':str(error)})+'\n');raise
