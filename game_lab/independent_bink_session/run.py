#!/usr/bin/env python3
"""Cooperative-child Session regression. Timeouts retain an unfinished PID."""
import argparse,ast,hashlib,json,os,pathlib,subprocess,time
p=argparse.ArgumentParser();p.add_argument('--snapshot',required=True,type=pathlib.Path);p.add_argument('--output',required=True,type=pathlib.Path)
a=p.parse_args();root=a.snapshot.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
here=pathlib.Path(__file__).resolve().parent;deadline=time.monotonic()+140;commands=[];rows=[]
def run(cmd,label,limit,cwd=None):
    remaining=deadline-time.monotonic()
    if remaining<=0:raise TimeoutError('140-second total deadline')
    argv=[str(x) for x in cmd];child=subprocess.Popen(argv,cwd=cwd,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    try:stdout,stderr=child.communicate(timeout=min(limit,remaining))
    except subprocess.TimeoutExpired as e:
        (out/(label+'.stdout')).write_bytes(e.stdout or b'');(out/(label+'.stderr')).write_bytes(e.stderr or b'')
        commands.append({'argv':argv,'label':label,'exit':None,'pid':child.pid,'state':'unfinished','forced_signal_sent':False})
        (out/'unfinished.json').write_text(json.dumps({'commands':commands,'results':rows,'cleanup_confirmed':False},indent=2)+'\n')
        raise SystemExit(2)
    (out/(label+'.stdout')).write_bytes(stdout);(out/(label+'.stderr')).write_bytes(stderr)
    commands.append({'argv':argv,'label':label,'exit':child.returncode,'pid':child.pid,'forced_signal_sent':False})
    return subprocess.CompletedProcess(argv,child.returncode,stdout,stderr)
tree=ast.parse((root/'game_lab/run_bink_tests.py').read_text())
values={n.targets[0].id:ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and isinstance(n.targets[0],ast.Name)}
fake=out/'cooperative decoder';run(['/usr/bin/clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror',here/'fake_decoder.cpp','-o',fake],'fake-build',15).check_returncode()
for mode,flags in [('release',['-O2']),('ubsan',['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined'])]:
    exe=out/('session-'+mode);cmd=['/usr/bin/clang++','-std=c++17','-fobjc-arc','-fno-modules','-mmacosx-version-min=15.0','-Wall','-Wextra','-Werror',*flags,
        '-I',root,'-I',root/'file_io/include',here/'session.mm',*[root/x for x in values['SOURCES']]]
    for framework in values['FRAMEWORKS']:cmd.extend(['-framework',framework])
    cmd.extend(['-lz','-o',exe]);b=run(cmd,mode+'-build',45);row={'mode':mode,'compile_exit':b.returncode};rows.append(row)
    if b.returncode:continue
    work=out/(mode+'-work');work.mkdir();r=run([exe,work,fake],mode+'-test',15,cwd=work)
    mask_path=work/'child-signal-mask.json'
    mask=json.loads(mask_path.read_text()) if mask_path.exists() else None
    child_gone=False
    if mask and mask.get('pid',0)>0:
        try:os.kill(mask['pid'],0) # existence query only; no signal is sent
        except ProcessLookupError:child_gone=True
    row.update(exit=r.returncode,stderr_empty=not r.stderr,report=json.loads(r.stdout) if r.stdout else None,
        child_signal_mask=mask,shutdown_signals_unblocked=bool(mask and mask['term_blocked']==0 and mask['int_blocked']==0),
        recorded_child_no_longer_exists=child_gone)
report={'results':rows,'commands':commands,'overall_seconds_limit':140,'self_authored_inputs_only':True,
        'child_SIGTERM_policy':'cooperative handler, normal exit; natural 12-second deadline','test_timeout_policy':'record unfinished PID; send no forced signal',
        'original_data_progress':None,'new_integrated_safe_close_not_yet_reviewed':True,
        'source_sha256':{str(x):hashlib.sha256(x.read_bytes()).hexdigest() for x in [here/'fake_decoder.cpp',here/'session.mm',here/'run.py',root/'game_lab/movie_session.mm',root/'stream_lab/bink_process.hpp']}}
(out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(rows,indent=2))
raise SystemExit(0 if len(rows)==2 and all(x.get('exit')==0 and x.get('shutdown_signals_unblocked') and x.get('recorded_child_no_longer_exists') for x in rows) else 1)
