import hashlib,json,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[1];src=R/'samples/stage48_game_window.m';lib=R/'runs/stage48-native/libstage48_game_window_v2.dylib';lib.parent.mkdir(exist_ok=True)
cmd=['<LOCAL_MACHINE_PATH>','-arch','arm64','-isysroot','<LOCAL_MACHINE_PATH>','-O2','-Wall','-Wextra','-fobjc-arc','-dynamiclib',str(src),'-framework','AppKit','-framework','CoreGraphics','-o',str(lib)];subprocess.run(cmd,check=True,timeout=20)
out=dict(command=cmd,library=str(lib),sha256=hashlib.sha256(lib.read_bytes()).hexdigest(),source_sha256=hashlib.sha256(src.read_bytes()).hexdigest(),scope='task authored host-only diagnostic hide of actual nonactivating game panel; no guest writes or global input/focus calls');(R/'evidence/stage48-game-window-build.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
