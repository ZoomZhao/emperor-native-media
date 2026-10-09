"""Build only our own event-state code with existing Apple tools."""
import hashlib,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
source=ROOT/'samples/stage59_keys.m'
lib=ROOT/'runs/stage59-native/libstage59_keys.dylib'
lib.parent.mkdir(exist_ok=True)
command=['<LOCAL_MACHINE_PATH>','-arch','arm64','-isysroot',
         '<LOCAL_MACHINE_PATH>','-O2','-Wall','-Wextra','-Werror',
         '-fobjc-arc','-dynamiclib',str(source),'-framework','AppKit','-o',str(lib)]
subprocess.run(command,check=True,timeout=30)
record=dict(command=command,library=str(lib),sha256=hashlib.sha256(lib.read_bytes()).hexdigest(),
            source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),third_party_code=False,
            original_EXECUTION=False,global_dependencies_installed=False)
(ROOT/'evidence/stage59-build.json').write_text(json.dumps(record,indent=2)+'\n')
print(json.dumps(record))
