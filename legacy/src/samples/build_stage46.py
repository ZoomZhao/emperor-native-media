import hashlib,json,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[1];src=R/'samples/stage46_samples.c';lib=R/'runs/stage46-native/libstage46_samples.dylib';lib.parent.mkdir(exist_ok=True)
dependency=R/'runs/stage37-native/libstage37_audio.dylib';assert hashlib.sha256(dependency.read_bytes()).hexdigest()==json.loads((R/'evidence/stage37-build.json').read_text())['sha256'];included=R/'samples/stage39_samples.c';assert hashlib.sha256(included.read_bytes()).hexdigest()==json.loads((R/'evidence/stage39-native-build.json').read_text())['source_sha256']
cmd=['<LOCAL_MACHINE_PATH>','-arch','arm64','-isysroot','<LOCAL_MACHINE_PATH>','-std=c11','-O2','-Wall','-Wextra','-dynamiclib',str(src),str(dependency),'-o',str(lib)]
subprocess.run(cmd,check=True,timeout=20);out=dict(library=str(lib),sha256=hashlib.sha256(lib.read_bytes()).hexdigest(),source_sha256=hashlib.sha256(src.read_bytes()).hexdigest(),included_source=str(included),included_source_sha256=hashlib.sha256(included.read_bytes()).hexdigest(),command=cmd,scope='task-authored actual mono22050 S16 PCM owner and offline ring; no source PCM device output; no original DLL execution')
(R/'evidence/stage46-native-build.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
