import hashlib,json,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[1];src=R/'samples/stage47_visibility.m';lib=R/'runs/stage47-native/libstage47_visibility_v2.dylib';lib.parent.mkdir(exist_ok=True)
cmd=['<LOCAL_MACHINE_PATH>','-arch','arm64','-isysroot','<LOCAL_MACHINE_PATH>','-O2','-Wall','-Wextra','-fobjc-arc','-dynamiclib',str(src),'-framework','AppKit','-framework','CoreGraphics','-o',str(lib)]
subprocess.run(cmd,check=True,timeout=20)
out=dict(library=str(lib),sha256=hashlib.sha256(lib.read_bytes()).hexdigest(),source_sha256=hashlib.sha256(src.read_bytes()).hexdigest(),command=cmd,scope='own arm64 recovery of task nonactivating NSPanel after delayed AppKit launch; no activation/event/cursor APIs')
(R/'evidence/stage47-visibility-build.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
