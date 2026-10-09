import hashlib,json,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[1];src=R/'samples/stage43_cursor.m';lib=R/'runs/stage43-native/libstage43_cursor.dylib';lib.parent.mkdir(exist_ok=True)
cmd=['<LOCAL_MACHINE_PATH>','-arch','arm64','-isysroot','<LOCAL_MACHINE_PATH>','-O2','-Wall','-Wextra','-dynamiclib','-fobjc-arc',str(src),'-framework','AppKit','-o',str(lib)]
subprocess.run(cmd,cwd=R,check=True,timeout=20);out=dict(library=str(lib),sha256=hashlib.sha256(lib.read_bytes()).hexdigest(),source_sha256=hashlib.sha256(src.read_bytes()).hexdigest(),command=cmd,architecture='arm64',scope='actual owned NSView cursor rectangles and previous cursor object, serialized image+hotspot+guest token; no global set/hide or events')
(R/'evidence/stage43-cursor-build.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
