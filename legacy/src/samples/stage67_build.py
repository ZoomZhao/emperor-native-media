"""Build only own native UI/DD sources against installed trusted frameworks."""
import hashlib,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];S=ROOT/'samples';D=ROOT/'runs/stage67-native';D.mkdir(exist_ok=True)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def run(args):subprocess.run(args,cwd=ROOT,check=True,timeout=35)
framework='<LOCAL_MACHINE_PATH>'
dd=D/'libstage67_dd.dylib'
run(['/usr/bin/clang','-fobjc-arc','-dynamiclib','-O2',str(S/'stage34_dd.m'),str(S/'stage67_pixels.c'),
 '-framework','AppKit','-framework','CoreGraphics','-framework','Metal','-framework','MetalKit','-framework','QuartzCore','-o',str(dd)])
meta=dict(path=str(dd),sha256=sha(dd),own_pixels_sha256=sha(S/'stage67_pixels.c'),
 reused_own_DD_source_sha256=sha(S/'stage34_dd.m'),translated_game_code_linked=False,CPU_runtime_linked=False)
(ROOT/'evidence/stage67-dd-build.json').write_text(json.dumps(meta,indent=2)+'\n')
old=(S/'stage34_dd.py').read_text();assert old.count('stage34-dd-build.json')==1
(S/'stage67_dd.py').write_text(old.replace('stage34-dd-build.json','stage67-dd-build.json'))
helper=D/'stage67-ui-helper'
run(['/usr/bin/clang','-fobjc-arc','-O2',str(S/'stage67_launcher.m'),'-I'+framework+'/Headers',
 '-F<LOCAL_MACHINE_PATH>','-framework','AppKit','-framework','Python3',
 '-Wl,-rpath,<LOCAL_MACHINE_PATH>','-o',str(helper)])
(ROOT/'evidence/stage67-helper-build.json').write_text(json.dumps(dict(path=str(helper),sha256=sha(helper),
 source_sha256=sha(S/'stage67_launcher.m'),trusted_installed_Python_framework=framework,no_game_or_Unicorn_linked=True),indent=2)+'\n')
print(json.dumps(dict(own_helper=str(helper),own_clean_DD=str(dd))))
