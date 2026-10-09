import hashlib,json,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[1];src=R/'samples/stage45_pump.m';lib=R/'runs/stage45-native/libstage45_pump.dylib';lib.parent.mkdir(exist_ok=True)
cmd=['<LOCAL_MACHINE_PATH>','-arch','arm64','-isysroot','<LOCAL_MACHINE_PATH>','-O2','-Wall','-Wextra','-dynamiclib','-fobjc-arc',str(src),'-framework','AppKit','-o',str(lib)]
subprocess.run(cmd,check=True,timeout=20);out=dict(library=str(lib),sha256=hashlib.sha256(lib.read_bytes()).hexdigest(),source_sha256=hashlib.sha256(src.read_bytes()).hexdigest(),command=cmd,scope='owned window actual NSEvent FIFO; serialized pending inputs; physical NSApp queue polled only when owned FIFO empty; WindowMoved acknowledged only for same own window and unchanged native geometry; local mouse event route; no global event/cursor/focus API')
(R/'evidence/stage45-pump-build.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
