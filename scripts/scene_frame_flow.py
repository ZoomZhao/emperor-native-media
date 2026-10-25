#!/usr/bin/env python3
"""Authored callback routing and ARC weak-owner tests; no renderer, window or assets."""
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
    binary=out/'flow-tests';dep=out/'dependencies.d';cmd=['/usr/bin/clang++',*flags,'-MMD','-MF',str(dep),str(ROOT/'game_lab/scene_frame_flow_tests.mm'),'-framework','Foundation','-o',str(binary)]
    subprocess.run(cmd,check=True)
    bindings={}
    for token in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
        path=Path(token).resolve()
        if ROOT in path.parents:bindings[str(path.relative_to(ROOT))]=sha(path)
    run=subprocess.run([str(binary)],text=True,capture_output=True,check=True);result=json.loads(run.stdout)
    assert result['passed'] and not run.stderr
    assert all(sha(ROOT/n)==h for n,h in bindings.items())
    rec={'mode':a.mode,'command':cmd,'compiler':subprocess.check_output(['/usr/bin/clang++','--version'],text=True),'sources_sha256':bindings,'driver_sha256':sha(__file__),'binary_sha256':sha(binary),'result':result,'stderr':run.stderr,'assets_read':False,'GPU_executed':False,'App_executed':False,'window_created':False}
    (out/'verification.json').write_text(json.dumps(rec,indent=2)+'\n');print(run.stdout,end='')
if __name__=='__main__':main()
