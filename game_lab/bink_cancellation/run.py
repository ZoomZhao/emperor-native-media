#!/usr/bin/env python3
"""Authored review regressions: no original data, UI, or native audio output."""
from pathlib import Path
import argparse
import importlib.util
import os
import struct
import subprocess
import sys
import tempfile
import json
import hashlib

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=['release', 'undefined'], default='release')
    parser.add_argument('--build-dir', type=Path)
    args = parser.parse_args()
    if args.build_dir:
        if not args.build_dir.is_absolute() or args.build_dir.exists():
            parser.error('--build-dir must be a fresh absolute directory')
        data = os.environ.get('EMPEROR_GAMEDATA_ROOT')
        if data and args.build_dir.resolve().is_relative_to(Path(data).resolve()):
            parser.error('test output must stay outside GameData')
        work = args.build_dir
        work.mkdir(parents=True)
    else:
        work = Path(tempfile.mkdtemp(prefix='emperor-bink-cancellation-'))
    print('owned_test_output='+str(work), flush=True)
    (work/'root').mkdir()
    header = bytearray(56)
    header[:4] = b'BIKi'
    for offset, value in [(4,48),(8,30),(20,1),(24,1),(28,30),(32,1),(40,1)]:
        struct.pack_into('<I',header,offset,value)
    struct.pack_into('<H',header,48,44100)
    header[51] = 0x20
    (work/'root/own.bik').write_bytes(header)
    child = work/'owned_decoder.py'
    child.write_text('#!'+sys.executable+'\n'+r'''
import os,sys,time,struct
mode=os.environ.get('REVIEW_CHILD_MODE','flow')
def send(fd,data):
 while data:
  count=os.write(fd,data);data=data[count:]
if mode in ('blocked','backpressure'):
 with open(os.environ['REVIEW_CHILD_PID'],'w') as f:f.write(str(os.getpid()))
 if mode=='blocked':time.sleep(20);sys.exit(0)
if mode=='stderr':send(2,b'e'*(1024*1024));send(1,bytes(30*4))
elif mode=='partial_video':send(1,bytes(119))
elif mode=='bad_exit':send(1,bytes(16));sys.exit(9)
elif mode=='partial_pcm':send(1,bytes(3))
elif mode=='backpressure':send(1,bytes(4*1024*1024))
else:
 audio='0:a:0' in sys.argv
 if mode=='hold':
  send(1,bytes(6*4096*4) if audio else bytes(30*4))
  if audio:time.sleep(20)
 else:
  data=b''.join(struct.pack('<hh',i%32767,-(i%32767)) for i in range(44100)) if audio else b''.join(bytes([i,0,0,255]) for i in range(30))
  partial=(mode=='pcm' and audio) or (mode=='video' and not audio)
  if partial:
   prefix=6*4096*4+137 if audio else 5
   send(1,data[:prefix]);deadline=time.monotonic()+20
   while not os.path.exists(os.environ['REVIEW_GATE']):
    if time.monotonic()>=deadline:sys.exit(7)
    time.sleep(.002)
   send(1,data[prefix:])
  else:send(1,data)
''')
    child.chmod(0o700)
    spec = importlib.util.spec_from_file_location('bink_build',ROOT/'game_lab/run_bink_tests.py')
    cfg = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(cfg)
    flags = ['-O2'] if args.mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all']
    common = ['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-I',str(ROOT),'-I',str(ROOT/'file_io/include')]
    target = work/'pause-continuity'
    command = [*common,'-fobjc-arc','-fno-modules','-mmacosx-version-min=15.0','-include',str(HERE/'fake_audio_hooks.hpp'),str(HERE/'pause_cancel.mm'),str(HERE/'fake_audio_hooks.cpp'),*[str(ROOT/p) for p in cfg.SOURCES]]
    for framework in cfg.FRAMEWORKS:
        command.extend(['-framework',framework])
    command.extend(['-lz','-o',str(target)])
    subprocess.run(command,check=True,timeout=60)
    symbols=subprocess.check_output(['/usr/bin/nm','-u',str(target)],text=True)
    names=['NewOutput','Start','Stop','Dispose','AllocateBuffer','EnqueueBuffer','GetCurrentTime','SetParameter','GetProperty','Pause','CreateTimeline']
    assert not any((' _AudioQueue'+name) in symbols for name in names), 'native output symbol leaked into authored queue test'
    environment=dict(os.environ,EMPEROR_FFMPEG_PATH=str(child))
    environment.pop('EMPEROR_GAMEDATA_ROOT',None)
    results=[]
    def execute(name,arguments):
        process=subprocess.Popen(arguments,env=environment,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        try:
            output,errors=process.communicate(timeout=45)
        except subprocess.TimeoutExpired:
            print('BLOCKED: owned test PID %d still running; no signal sent; cleanup unconfirmed'%process.pid,flush=True)
            raise SystemExit(124)
        (work/(name+'.log')).write_text(output+errors)
        print(output,end='',flush=True)
        if errors:print(errors,end='',file=sys.stderr,flush=True)
        if process.returncode:raise subprocess.CalledProcessError(process.returncode,arguments)
        result=next(json.loads(line) for line in reversed(output.splitlines()) if line.startswith('{'))
        results.append({'test':name,'exit':process.returncode,'result':result})
    execute('pause-continuity',[str(target),str(work)])
    boundary=work/'process-boundaries'
    subprocess.run([*common,str(HERE/'process_boundaries.cpp'),'-o',str(boundary)],check=True,timeout=30)
    execute('process-boundaries',[str(boundary),str(work/'root/own.bik'),str(work/'child.pid')])
    sources=[HERE/name for name in ['run.py','pause_cancel.mm','fake_audio_hooks.hpp','fake_audio_hooks.cpp','process_boundaries.cpp']]
    sources += [ROOT/name for name in ['stream_lab/bink_process.hpp','game_lab/bink_audio.hpp','game_lab/bink_preview.hpp','game_lab/movie_session.mm']]
    report={'mode':args.mode,'tests':results,'no_native_AudioQueue_symbols':True,'original_data':False,'UI':False,'audio_devices':False,'forced_signal_sent_to_test':False,'source_sha256':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}}
    (work/'results.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0

if __name__=='__main__':
    try:
        raise SystemExit(main())
    except (OSError,subprocess.CalledProcessError,subprocess.TimeoutExpired) as error:
        print('FAIL:',error,file=sys.stderr)
        raise SystemExit(1)
