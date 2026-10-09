import hashlib,json,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[1]
source=R/'samples/stage38_miles.c';library=R/'runs/stage38-native/libstage38_miles.dylib';library.parent.mkdir(exist_ok=True)
command=['<LOCAL_MACHINE_PATH>','-arch','arm64','-isysroot','<LOCAL_MACHINE_PATH>','-std=c11','-O2','-Wall','-dynamiclib',str(source),'-o',str(library)]
subprocess.run(command,cwd=R,check=True,timeout=20)
m=dict(library=str(library),sha256=hashlib.sha256(library.read_bytes()).hexdigest(),source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),command=command,architecture='arm64')
(R/'evidence/stage38-native-build.json').write_text(json.dumps(m,indent=2));print(json.dumps(m))
