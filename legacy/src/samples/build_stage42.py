import hashlib,json,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[1];src=R/'samples/stage42_bink.c';lib=R/'runs/stage42-native/libstage42_bink.dylib';lib.parent.mkdir(exist_ok=True)
dep=R/'runs/stage37-native/libstage37_audio.dylib';assert hashlib.sha256(dep.read_bytes()).hexdigest()==json.loads((R/'evidence/stage37-build.json').read_text())['sha256']
cmd=['<LOCAL_MACHINE_PATH>','-arch','arm64','-isysroot','<LOCAL_MACHINE_PATH>','-std=c11','-O2','-Wall','-Wextra','-dynamiclib',str(src),str(dep),'-o',str(lib)]
subprocess.run(cmd,cwd=R,check=True,timeout=20)
out=dict(library=str(lib),sha256=hashlib.sha256(lib.read_bytes()).hexdigest(),source_sha256=hashlib.sha256(src.read_bytes()).hexdigest(),dependency_sha256=hashlib.sha256(dep.read_bytes()).hexdigest(),command=cmd,architecture='arm64',scope='actual native sample-module lookup failure + actual readonly missing movie IO only; no decoder or sound-open table manufactured')
(R/'evidence/stage42-build.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
