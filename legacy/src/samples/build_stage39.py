import hashlib,json,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[1];src=R/'samples/stage39_samples.c';lib=R/'runs/stage39-native/libstage39_samples.dylib';lib.parent.mkdir(exist_ok=True)
dependency=R/'runs/stage37-native/libstage37_audio.dylib';assert hashlib.sha256(dependency.read_bytes()).hexdigest()==json.loads((R/'evidence/stage37-build.json').read_text())['sha256']
cmd=['<LOCAL_MACHINE_PATH>','-arch','arm64','-isysroot','<LOCAL_MACHINE_PATH>','-std=c11','-O2','-Wall','-dynamiclib',str(src),str(dependency),'-Wl,-rpath,'+str(R),'-o',str(lib)]
subprocess.run(cmd,cwd=R,check=True,timeout=20);m=dict(library=str(lib),sha256=hashlib.sha256(lib.read_bytes()).hexdigest(),source_sha256=hashlib.sha256(src.read_bytes()).hexdigest(),dependency=str(dependency),dependency_sha256=hashlib.sha256(dependency.read_bytes()).hexdigest(),command=cmd,architecture='arm64',scope='owned empty sample lifecycle only; data mixer/codec unsupported')
(R/'evidence/stage39-native-build.json').write_text(json.dumps(m,indent=2));print(json.dumps(m))
