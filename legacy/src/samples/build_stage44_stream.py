import json,hashlib,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[1];src=R/'samples/stage44_stream.c';lib=R/'runs/stage44-native/libstage44_stream.dylib';lib.parent.mkdir(exist_ok=True)
cmd=['<LOCAL_MACHINE_PATH>','-arch','arm64','-isysroot','<LOCAL_MACHINE_PATH>','-O2','-Wall','-Wextra','-dynamiclib',str(src),str(R/'runs/stage37-native/libstage37_audio.dylib'),'-framework','AudioToolbox','-framework','CoreFoundation','-o',str(lib)]
subprocess.run(cmd,check=True,timeout=20);out=dict(library=str(lib),sha256=hashlib.sha256(lib.read_bytes()).hexdigest(),source_sha256=hashlib.sha256(src.read_bytes()).hexdigest(),command=cmd,scope='own arm64 ExtAudioFile offline decoder and real PCM ring queue; no original DLL; source PCM never device queued')
(R/'evidence/stage44-stream-build.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
