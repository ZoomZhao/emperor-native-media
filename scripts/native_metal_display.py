#!/usr/bin/env python3
"""Test display copying with authored textures/fake drawable; no App or window."""
from pathlib import Path
import argparse,hashlib,json,shlex,subprocess
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--mode',choices=['release','undefined'],default='release');p.add_argument('--output-dir',type=Path,required=True);a=p.parse_args()
    out=a.output_dir.resolve()
    if out==ROOT or ROOT in out.parents:p.error('output must be outside repository')
    out.mkdir(parents=True,exist_ok=True)
    flags=['-std=c++17','-fobjc-arc','-fno-modules','-Wall','-Wextra','-Werror','-mmacosx-version-min=15.2']
    flags+=['-O2'] if a.mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all']
    objects=[];commands=[];bindings={}
    for name in ['native_metal_display.mm','native_metal_display_tests.mm']:
        source=ROOT/'game_lab/app'/name;obj=out/(name+'.o');dep=out/(name+'.d')
        cmd=['/usr/bin/clang++',*flags,'-MMD','-MF',str(dep),'-c',str(source),'-o',str(obj)];subprocess.run(cmd,check=True);commands.append(cmd);objects.append(obj)
        for token in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
            path=Path(token).resolve()
            if ROOT in path.parents:bindings[str(path.relative_to(ROOT))]=sha(path)
    binary=out/'display-tests';cmd=['/usr/bin/clang++',*flags,*map(str,objects),'-framework','AppKit','-framework','QuartzCore','-framework','Metal','-o',str(binary)]
    subprocess.run(cmd,check=True);commands.append(cmd)
    run=subprocess.run([str(binary)],text=True,capture_output=True,check=True)
    result=json.loads(run.stdout)
    assert all(sha(ROOT/n)==h for n,h in bindings.items())
    record={'mode':a.mode,'sources_sha256':bindings,'driver_sha256':sha(__file__),'objects_sha256':{o.name:sha(o) for o in objects},'binary_sha256':sha(binary),'commands':commands,'result':result,'stderr':run.stderr,'assets_read':False,'window_created':False,'App_executed':False}
    (out/'verification.json').write_text(json.dumps(record,indent=2)+'\n');print(run.stdout,end='')
if __name__=='__main__':main()
