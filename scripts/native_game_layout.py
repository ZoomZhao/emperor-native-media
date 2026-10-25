#!/usr/bin/env python3
"""Check original-reference UI regions and point-space hit targets; no assets or GUI."""
from pathlib import Path
import argparse,hashlib,json,subprocess
R=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--mode',choices=['release','undefined'],default='release');p.add_argument('--output-dir',type=Path,required=True);a=p.parse_args();out=a.output_dir.resolve();assert out!=R and R not in out.parents;out.mkdir(parents=True,exist_ok=True)
source=R/'game_lab/app/native_game_layout_tests.cpp';header=R/'game_lab/app/native_game_layout.hpp';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();bindings={str(x.relative_to(R)):sha(x) for x in [source,header,Path(__file__).resolve()]}
flags=['-O2'] if a.mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all'];binary=out/'layout-tests';cmd=['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-I'+str(R),str(source),'-o',str(binary)];subprocess.run(cmd,check=True);run=subprocess.run([str(binary)],capture_output=True,text=True,check=True);result=json.loads(run.stdout);assert result['passed'];assert all(sha(R/n)==h for n,h in bindings.items());(out/'verification.json').write_text(json.dumps({'mode':a.mode,'command':cmd,'sources_sha256':bindings,'binary_sha256':sha(binary),'result':result,'stderr':run.stderr},indent=2));print(run.stdout,end='')
