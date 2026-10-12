#!/usr/bin/env python3
"""Bounded self-authored Empire grid15/child5 CLI integration; no original input."""
import argparse,hashlib,json,struct,subprocess,sys,time,zlib
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
a=p.parse_args()
if sys.flags.optimize:p.error('optimized invocation unsupported')
root=a.source_root.resolve();out=a.output.resolve()
if out.exists():p.error('fresh output required')
out.mkdir(parents=True);start=time.monotonic();commands=[];results=[];sha=lambda b:hashlib.sha256(b).hexdigest()
def run(argv,label,timeout=10):
    left=180-(time.monotonic()-start)
    if left<=0:raise RuntimeError('180-second total budget')
    q=subprocess.run([str(x) for x in argv],capture_output=True,text=True,timeout=min(left,timeout))
    (out/(label+'.stdout')).write_text(q.stdout);(out/(label+'.stderr')).write_text(q.stderr)
    commands.append(dict(argv=[str(x) for x in argv],exit_code=q.returncode,stdout_sha256=sha(q.stdout.encode()),stderr_sha256=sha(q.stderr.encode())))
    return q
first=struct.pack('<HHBIII',19,6,1,11,12,13)+b'abcdefghijklmnop'+struct.pack('<III',14,15,16)
context=first+struct.pack('<HIII8II',2,21,22,23,*range(101,109),24)+struct.pack('<HH',3,4)+bytes((i*17+5)%256 for i in range(3256))
header=lambda n:struct.pack('<Hi',1,n)
decl=lambda n:struct.pack('<HHH',65535,0,len(n))+n
flood=struct.pack('<HIBIII',1,11,12,13,14,15)+bytes(0xcb10)
festival=struct.pack('<H',4)+struct.pack('<H4I',1,21,22,23,24)+struct.pack('<8I',*range(101,109))+struct.pack('<IIBBIBIIII',201,202,1,2,203,3,204,205,206,207)
embedded=struct.pack('<H5I',2,11,12,13,14,15);child=struct.pack('<H',3)+embedded*3
religion=struct.pack('<H',1)+child*4+bytes(20)+struct.pack('<H',1)
base=context+header(0)*10+flood+festival+religion+header(1)+decl(b'Building')+struct.pack('<H',5)+bytes([255])*180
figure=struct.pack('<H',7)+bytes([255])*395+b'\0';pheasant=struct.pack('<H',2)+figure
figure=struct.pack('<H',7)+bytes([255])*395+b'\0';pheasant=struct.pack('<H',2)+figure
following=base+header(2)+decl(b'Figure')+figure+decl(b'cPheasantFig')+pheasant+header(2)+(struct.pack('<H',1)+bytes([255])*268)*2
empire=lambda v:following+struct.pack('<HiH',v,1,2)+bytes([255])*38
grid=struct.pack('<H',15)+bytes([255])*1192+(struct.pack('<H',5)+bytes([255])*627)*22+bytes([255])*334
if len(following)!=57379 or len(empire(3))!=57425 or len(grid)!=15366:raise RuntimeError('self-authored width mismatch')
flags=[4096,8192,1,8192,8192,22]
whole=empire(3)+grid*22+bytes([255])*4+bytes([255])*128
cases=[('parent3-full',whole,flags,57425+338056,22,None,False),('parent2-full',empire(2)+grid*22,flags,57425+338052,22,None,False),('parent1-no-grids',empire(1)+b'\xff'*128,flags,57425,0,None,False),('grid-checkpoint1',whole,flags[:-1]+[1],57425+15366,1,None,False),('unknown-grid14',empire(3)+struct.pack('<H',14)+b'\xff'*20000,flags,57427,0,None,False),('unknown-child4',empire(3)+struct.pack('<H',15)+b'\xff'*1192+struct.pack('<H',4)+b'\xff'*20000,flags,58621,0,None,False),('split-grid-and-child-words',whole,flags,57425+338056,22,None,True),('truncated-parent-tail',empire(3)+grid*22+b'\xff'*3,flags,None,None,'truncated',False),('zero-grid-budget',whole,flags[:-1]+[0],None,None,'grid preceding context/modern budget',False)]
prepared=[];inputs=out/'self-authored-inputs';inputs.mkdir()
for name,b,options,end,processed,error,split in cases:
    cuts=set(range(32768,len(b),32768))
    if split:cuts.update([57426,58620,72790])
    cuts=sorted(n for n in cuts if 0<n<len(b));parts=[b[i:j] for i,j in zip([0]+cuts,cuts+[len(b)])]
    encoded=struct.pack('<I',4275878570)
    for i,part in enumerate(parts):
        packed=zlib.compress(part)
        raw=split and i==0
        encoded+=struct.pack('<II',2863386095 if raw else 2882400255,len(part) if raw else len(packed))+(b'' if raw else b'TEST')+(part if raw else packed)
    f=inputs/(name+'.bin');f.write_bytes(encoded);prepared.append((name,b,options,end,processed,error,f,len(parts),sha(encoded)))
sdk=run(['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'],'sdk',30)
if sdk.returncode:raise RuntimeError('SDK unavailable')
for mode in ['release','ubsan']:
    exe=out/('empire-grids-'+mode);compiler=['-O2'] if mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined']
    q=run(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*compiler,'-isysroot',sdk.stdout.strip(),'-I',root/'file_io/include',root/'file_io/src/strict_zlib.cpp',root/'file_io/src/framed_reader.cpp',root/'file_io/tools/qualify_city19_empire_grids.cpp','-lz','-o',exe],mode+'-build',60)
    if q.returncode:raise RuntimeError('CLI build failed')
    for name,b,options,end,processed,error,f,blocks,input_sha in prepared:
        q=run([exe,f,f.stat().st_size,262144,4915584,blocks,4275878570,2882400255,2863386095,36144,36144,input_sha,sha(b),*options],mode+'-'+name)
        seen=json.loads(q.stdout) if q.stdout else None
        if error:ok=q.returncode==2 and not q.stdout and error in q.stderr
        else:
            ok=q.returncode==0 and not q.stderr and seen is not None and seen['parsed_end']==end and seen['empire_grids']['processed']==processed and seen['conditional_cache_next_id']==7 and seen['decoded_sha256']==sha(b) and seen['empire_inline']['end']==57425 and not seen['empire_grids']['original_postload_executed']
            if name=='unknown-child4':ok=ok and seen['empire_grids']['pending']['child'] and seen['empire_grids']['pending']['version']==4
            if name=='unknown-grid14':ok=ok and not seen['empire_grids']['pending']['child'] and seen['empire_grids']['pending']['version']==14
            if processed==22:ok=ok and seen['empire_grids']['parent_byte_requests_complete'] and all(len(g['children'])==22 for g in seen['empire_grids']['grids'])
        results.append(dict(mode=mode,name=name,passed=bool(ok),observed=seen))
for _,_,_,_,_,_,f,_,s in prepared:
    if sha(f.read_bytes())!=s:raise RuntimeError('self-authored input changed')
report=dict(passed=sum(r['passed'] for r in results),failed=sum(not r['passed'] for r in results),cases_per_mode=len(cases),results=results,commands=commands,elapsed_seconds=time.monotonic()-start,total_budget_seconds=180,original_payload_or_EXE_read=False,original_or_derived_game_executed=False,new_REA_query=False,main_written=False,pushed=False,driver_sha256=sha(Path(__file__).read_bytes()),component_sha256=sha((root/'file_io/include/city19_empire_grids_archive.hpp').read_bytes()),tool_sha256=sha((root/'file_io/tools/qualify_city19_empire_grids.cpp').read_bytes()))
(out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['passed','failed','elapsed_seconds']}));raise SystemExit(bool(report['failed']))
