import hashlib,json,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[1];source=R/'samples/stage31_window.m';body=source.read_text();old='setActivationPolicy:NSApplicationActivationPolicyRegular';assert body.count(old)==1
body=body.replace(old,'setActivationPolicy:NSApplicationActivationPolicyProhibited').replace('S31Window','S43Window')
first='[NSApplication sharedApplication];Core*c=calloc';assert body.count(first)==1;body=body.replace(first,'[NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];Core*c=calloc')
src=R/'samples/stage43_window.m';src.write_text(body)
loader=R/'samples/stage31_window.py';py=loader.read_text();assert py.count('runs/stage31-native/libstage31_window.dylib')==1 and py.count('stage31-window-build.json')==1
newloader=R/'samples/stage43_window.py';newloader.write_text(py.replace('runs/stage31-native/libstage31_window.dylib','runs/stage43-native/libstage43_window.dylib').replace('stage31-window-build.json','stage43-window-build.json'))
lib=R/'runs/stage43-native/libstage43_window.dylib';cmd=['<LOCAL_MACHINE_PATH>','-arch','arm64','-isysroot','<LOCAL_MACHINE_PATH>','-O2','-Wall','-Wextra','-dynamiclib',str(src),'-framework','AppKit','-framework','CoreGraphics','-o',str(lib)]
subprocess.run(cmd,cwd=R,check=True,timeout=20);out=dict(library=str(lib),sha256=hashlib.sha256(lib.read_bytes()).hexdigest(),source_sha256=hashlib.sha256(src.read_bytes()).hexdigest(),loader_sha256=hashlib.sha256(newloader.read_bytes()).hexdigest(),base_own_source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),command=cmd,architecture='arm64',scope='existing own stage31 adapter ABI; process-local Accessory policy prevents implicit regular-app activation; no original project implementation used')
out['scope']='existing own stage31 ABI; distinct S43Window class; process-local Prohibited activation policy from first AppKit initialization and show; no original project implementation used'
(R/'evidence/stage43-window-build.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
