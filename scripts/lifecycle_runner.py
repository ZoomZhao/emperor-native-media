"""Compile only reviewed own production sources and a synthetic event regression."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import signal
import subprocess
import tempfile
import time

repo=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--mode',choices=['release','undefined'],default='undefined')
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
assert args.output.is_absolute()
SOURCES=['scripts/lifecycle_runner.py','scripts/self_fixture_evidence.hpp',
 'game_lab/app_media_lifecycle_tests.mm','game_lab/app_media.cpp','game_lab/app_media.hpp',
 'game_lab/emperor_assets.mm','game_lab/emperor_assets.hpp','game_lab/asset_bytes.hpp','game_lab/asset_limits.hpp',
 'game_lab/wav_pcm.cpp','game_lab/asset_pixels.cpp','game_lab/resource_root.cpp','game_lab/resource_root.hpp',
 'game_lab/audio_policy.hpp','surface_lab/surface.cpp','surface_lab/surface.hpp']
def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as stream:
        while block:=stream.read(1024*1024):h.update(block)
    return h.hexdigest()
args.output.mkdir(parents=True,exist_ok=True)
report=Path(tempfile.mkdtemp(prefix='lifecycle-'+args.mode+'-',dir=args.output))
snapshot=report/'source'
records={}
for name in SOURCES:
    original=repo/name
    if original.is_symlink() or not original.is_file():raise RuntimeError('Missing or unreviewed source: '+name)
    content=original.read_bytes();records[name]=hashlib.sha256(content).hexdigest()
    copy=snapshot/name;copy.parent.mkdir(parents=True,exist_ok=True);copy.write_bytes(content)
source_manifest={'sources_sha256':records,'mode':args.mode}
(report/'source-manifest.json').write_text(json.dumps(source_manifest,indent=2)+'\n')
test=report/'event_regression.mm'
test.write_bytes((snapshot/'game_lab/app_media_lifecycle_tests.mm').read_bytes())
temporary=report/'tmp';temporary.mkdir()
env={'PATH':'/usr/bin:/bin:/usr/sbin:/sbin','LANG':'en_US.UTF-8','TMPDIR':str(temporary),
     'SELF_FIXTURE_DIR':str(report/'fixtures'),'SELF_INPUT_TRACE':str(report/'inputs.jsonl')}
compiled=['game_lab/app_media.cpp','game_lab/emperor_assets.mm','game_lab/wav_pcm.cpp',
          'game_lab/asset_pixels.cpp','game_lab/resource_root.cpp','surface_lab/surface.cpp']
commands=[('compile',['/usr/bin/clang++','-std=c++17','-fobjc-arc','-fno-modules','-Wall','-Wextra','-Werror',
                    *(['-O2'] if args.mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all']),'-I',str(snapshot/'game_lab'),
                    '-I',str(snapshot),str(test),*[str(snapshot/n) for n in compiled],
                    '-framework','Foundation','-framework','ImageIO','-framework','CoreGraphics','-lz',
                    '-o',str(report/'event-tests')]),('original-four',[str(report/'event-tests')]),*[ (path+'-'+kind,[str(report/'event-tests'),path,kind]) for path in ('music-start','music-resume','sfx-start') for kind in ('error','finished') ],
          *[(action,[str(report/'event-tests'),action]) for action in ('nested-replacement','stop-during-play')]]
processes=[]
for label,argv in commands:
    case_dir=report if label=='compile' else report/'cases'/label
    case_dir.mkdir(parents=True,exist_ok=True)
    case_env=env.copy()
    if label!='compile':
        (case_dir/'tmp').mkdir()
        case_env.update(TMPDIR=str(case_dir/'tmp'),SELF_FIXTURE_DIR=str(case_dir/'fixtures'),SELF_INPUT_TRACE=str(case_dir/'inputs.jsonl'))
    start=time.monotonic()
    with (report/(label+'.stdout')).open('wb') as out,(report/(label+'.stderr')).open('wb') as err:
        p=subprocess.Popen(argv,cwd=case_dir,env=case_env,stdout=out,stderr=err,start_new_session=True)
        try:code=p.wait(timeout=60);timed_out=False
        except subprocess.TimeoutExpired:
            os.killpg(p.pid,signal.SIGKILL);p.wait(timeout=5);code=p.returncode;timed_out=True
    processes.append({'label':label,'argv':argv,'cwd':str(case_dir),'exit_code':code,'timed_out':timed_out,
                      'elapsed_seconds':time.monotonic()-start,'timeout_seconds':60})
    if code and label=='compile':break
inputs={}
for process in processes:
    case_dir=Path(process['cwd'])
    for directory in ['fixtures','self-root']:
        if not (case_dir/directory).is_dir():continue
        for path in (case_dir/directory).rglob('*'):
            if path.is_file() and not path.is_symlink():inputs[path.relative_to(report).as_posix()]={'sha256':sha(path),'bytes':path.stat().st_size,'process':process['label']}
    if (case_dir/'inputs.jsonl').is_file():inputs[(case_dir/'inputs.jsonl').relative_to(report).as_posix()]={'sha256':sha(case_dir/'inputs.jsonl'),'bytes':(case_dir/'inputs.jsonl').stat().st_size,'process':process['label']}
result={'source_report':str(report),'sources_sha256':source_manifest['sources_sha256'],
        'test_source_sha256':sha(test),'processes':processes,'actual_inputs':inputs,
        'case_outputs':{p['label']:{'stdout':(report/(p['label']+'.stdout')).read_text(),'stderr':(report/(p['label']+'.stderr')).read_text()} for p in processes},

        'source_stable':all(sha(repo/n)==h and sha(snapshot/n)==h for n,h in records.items()),
        'passed':all(sha(repo/n)==h and sha(snapshot/n)==h for n,h in records.items()) and all(p['exit_code']==0 and not p['timed_out'] for p in processes) and len(processes)==len(commands),
        'binary_sha256':sha(report/'event-tests') if (report/'event-tests').is_file() else None,
        'ui':False,'audio_devices':False,'GameData':False,'legacy':False}
(report/'regression-result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')

print(json.dumps({'report':str(report),'passed':result['passed'],'case_outputs':result['case_outputs'],
                  'processes':[{'label':p['label'],'exit_code':p['exit_code']} for p in processes]},ensure_ascii=False))

raise SystemExit(0 if result["passed"] else 1)
