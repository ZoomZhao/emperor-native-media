#!/usr/bin/env python3
"""Approval-gated SRW build. Do not execute without explicit scoped approval."""
from pathlib import Path
import shutil, subprocess, json, time

ROOT=Path(__file__).resolve().parents[1]
CLANG=Path('<LOCAL_MACHINE_PATH>')
SDK=Path('<LOCAL_MACHINE_PATH>')
src=ROOT/'build'/'SRW'
if src.exists(): raise SystemExit('Refusing to replace existing build/SRW')
shutil.copytree(ROOT/'vendor-src'/'SR'/'SRW',src)
defs=src/'SR_defs.h'
text=defs.read_text()
assert text.count('#define OUTPUT_TYPE  OUT_X86')==1
defs.write_text(text.replace('#define OUTPUT_TYPE  OUT_X86','#define OUTPUT_TYPE  OUT_LLASM'))
compat=ROOT/'build'/'compat'
compat.mkdir(exist_ok=True)
(compat/'malloc.h').write_text('#include <stdlib.h>\n')
objdir=ROOT/'build'/'objects'
objdir.mkdir(exist_ok=True)
bindir=ROOT/'build'/'bin'
bindir.mkdir(exist_ok=True)
log=ROOT/'evidence'/'srw-build.log'
manifest=[]
files=sorted(src.glob('*.c'))+sorted((src/'udis86-1.7.2'/'libudis86').glob('*.c'))+sorted(src.glob('*.cpp'))
objects=[]
with log.open('w') as out:
 for i,file in enumerate(files):
  obj=objdir/(str(i)+'-'+file.stem+'.o')
  cmd=[str(CLANG)+('++' if file.suffix=='.cpp' else ''),'-isysroot',str(SDK),'-O1','-Wall','-DHAVE_STRING_H=1', '-I'+str(compat),'-I'+str(src/'udis86-1.7.2'),'-c',str(file),'-o',str(obj)]
  manifest.append(cmd)
  out.write(json.dumps(cmd)+'\n'); out.flush()
  result=subprocess.run(cmd,cwd=ROOT,stdout=out,stderr=out,timeout=60)
  if result.returncode: raise SystemExit('Compile failed; see '+str(log))
  objects.append(str(obj))
 cmd=[str(CLANG)+'++','-isysroot',str(SDK),*objects,'-o',str(bindir/'SRW-llasm')]
 manifest.append(cmd); out.write(json.dumps(cmd)+'\n'); out.flush()
 result=subprocess.run(cmd,cwd=ROOT,stdout=out,stderr=out,timeout=60)
 if result.returncode: raise SystemExit('Link failed; see '+str(log))
(ROOT/'evidence'/'srw-build-commands.json').write_text(json.dumps(manifest,indent=2))
print('Built '+str(bindir/'SRW-llasm'))
