#!/usr/bin/env python3
"""Authored lifecycle/late-frame tests only. No data access, App or GPU launch."""
from pathlib import Path
import argparse, hashlib, json, shlex, subprocess
ROOT = Path(__file__).resolve().parents[1]
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=['release','undefined'], default='release')
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args(); out = args.output_dir.resolve()
    if out == ROOT or ROOT in out.parents: parser.error('output must be outside repository')
    out.mkdir(parents=True, exist_ok=True)
    binary = out/'scene-frame-gate-tests'; dep = out/'dependencies.d'
    flags = ['-std=c++17','-Wall','-Wextra','-Werror','-pedantic','-mmacosx-version-min=15.2']
    flags += ['-O2'] if args.mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all']
    command = ['/usr/bin/clang++',*flags,'-MMD','-MF',str(dep),str(ROOT/'game_lab/scene_frame_gate_tests.cpp'),'-o',str(binary)]
    subprocess.run(command,check=True)
    bindings = {}
    for item in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
        path = Path(item).resolve()
        if ROOT in path.parents: bindings[str(path.relative_to(ROOT))]=sha(path)
    run = subprocess.run([str(binary)],text=True,capture_output=True,check=True)
    if run.stderr: raise RuntimeError(run.stderr)
    result=json.loads(run.stdout); assert result['passed']
    assert all(sha(ROOT/name)==digest for name,digest in bindings.items())
    record = {'mode':args.mode,'compiler':subprocess.check_output(['/usr/bin/clang++','--version'],text=True),
        'command':command,'sources_sha256':bindings,'driver_sha256':sha(__file__),'binary_sha256':sha(binary),
        'result':result,'assets_read':False,'App_executed':False,'GPU_executed':False}
    (out/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
    print(run.stdout,end='')
if __name__=='__main__': main()
