#!/usr/bin/env python3
import hashlib,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];out=ROOT/'runs/stage19-native';out.mkdir(exist_ok=True)
src=ROOT/'samples/stage19_profile.c';lib=out/'libstage19_profile.dylib';cmd=['xcrun','clang','-arch','arm64','-std=c11','-Wall','-Wextra','-Werror','-O2','-dynamiclib',str(src),'-o',str(lib)]
p=subprocess.run(cmd,capture_output=True,text=True,timeout=30);(ROOT/'evidence/stage19-profile-build.log').write_text(p.stdout+p.stderr);assert p.returncode==0,p.stderr
record=dict(command=cmd,sha256=hashlib.sha256(lib.read_bytes()).hexdigest(),source_sha256=hashlib.sha256(src.read_bytes()).hexdigest(),self_authored=True);(ROOT/'evidence/stage19-profile-build.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record))
