import json,hashlib,struct,sys
from pathlib import Path
r=Path(__file__).resolve().parents[1];p=r/'runs/stage30-native/libstage30_dd.dylib';assert struct.unpack_from('<II',p.read_bytes())==(0xfeedfacf,0x100000c)
sources=['samples/stage23_bridge.c','samples/stage24_bridge.c','samples/stage24_x86.c','samples/stage24_aborts.c','samples/stage30_dd.m','runs/stage24-sprites/combined.ll']
m=dict(path=str(p),sha256=hashlib.sha256(p.read_bytes()).hexdigest(),inputs=[dict(path=s,sha256=hashlib.sha256((r/s).read_bytes()).hexdigest())for s in sources],build_execution=sys.argv[1],native_architecture='arm64');(r/'evidence/stage30-dd-build.json').write_text(json.dumps(m,indent=2));print(m['sha256'])
