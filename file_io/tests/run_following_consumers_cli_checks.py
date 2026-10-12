#!/usr/bin/env python3
"""Bounded self-authored following fixed4/empty vectors1 CLI integration; no original input."""
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
flags=[4096,8192,1,8192,8192,22,12,27]
prior=empire(3)+grid*22+bytes([255])*4
fixed=struct.pack('<H',4)+bytes([255])*32+b'\0'
vector=struct.pack('<H6I',1,0,0,0,0,0,0)
closed=prior+fixed*12+vector*27+struct.pack('<H',7)+bytes([255])*64
if len(prior)!=395481:raise RuntimeError('preceding synthetic widths')
cases=[('closed-empty',closed,flags,396605,12,27,None,False),('fixed-checkpoint1',closed,flags[:-2]+[1,27],395516,1,0,None,False),('fixed-legacy3',prior+struct.pack('<H',3)+bytes([255])*60,flags,395483,0,0,None,False),('fixed-presence7',prior+struct.pack('<H',4)+bytes([255])*32+b'\7'+bytes([255])*60,flags,395516,1,0,None,False),('vector-positive',prior+fixed*12+struct.pack('<HIH',1,2,65535)+bytes([255])*64,flags,395907,12,0,None,False),('vector-unknown2',prior+fixed*12+struct.pack('<H',2)+bytes([255])*24,flags,395903,12,0,None,False),('vector-checkpoint1',closed,flags[:-1]+[1],395927,12,1,None,False),('split-fixed-count-words',closed,flags,396605,12,27,None,True),('zero-fixed-budget',closed,flags[:-2]+[0,27],None,None,None,'fixed preceding context/modern budget',False)]
prepared=[];inputs=out/'self-authored-inputs';inputs.mkdir()
for name,b,options,end,fixed_n,vector_n,error,split in cases:
    cuts=set(range(32768,len(b),32768))
    if split:cuts.update([395482,395901+1,395903+1,396603+1])
    cuts=sorted(n for n in cuts if 0<n<len(b));parts=[b[i:j] for i,j in zip([0]+cuts,cuts+[len(b)])]
    encoded=struct.pack('<I',4275878570)
    for i,part in enumerate(parts):
        packed=zlib.compress(part)
        raw=split and i==0
        encoded+=struct.pack('<II',2863386095 if raw else 2882400255,len(part) if raw else len(packed))+(b'' if raw else b'TEST')+(part if raw else packed)
    f=inputs/(name+'.bin');f.write_bytes(encoded);prepared.append((name,b,options,end,fixed_n,vector_n,error,f,len(parts),sha(encoded)))
sdk=run(['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'],'sdk',30)
if sdk.returncode:raise RuntimeError('SDK unavailable')
for mode in ['release','ubsan']:
    exe=out/('following-consumers-'+mode);compiler=['-O2'] if mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined']
    q=run(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*compiler,'-isysroot',sdk.stdout.strip(),'-I',root/'file_io/include',root/'file_io/src/strict_zlib.cpp',root/'file_io/src/framed_reader.cpp',root/'file_io/tools/qualify_city19_following_consumers.cpp','-lz','-o',exe],mode+'-build',60)
    if q.returncode:raise RuntimeError('CLI build failed')
    for name,b,options,end,fixed_n,vector_n,error,f,blocks,input_sha in prepared:
        q=run([exe,f,f.stat().st_size,262144,4915584,blocks,4275878570,2882400255,2863386095,36144,36144,input_sha,sha(b),*options],mode+'-'+name)
        seen=json.loads(q.stdout) if q.stdout else None
        if error:ok=q.returncode==2 and not q.stdout and error in q.stderr
        else:
            ok=q.returncode==0 and not q.stderr and seen is not None and seen['parsed_end']==end and seen['following_fixed']['processed']==fixed_n and seen['following_vectors']['processed']==vector_n and seen['conditional_cache_next_id']==7 and seen['decoded_sha256']==sha(b) and seen['empire_inline']['end']==57425 and not seen['empire_grids']['original_postload_executed']
            if name=='fixed-presence7':ok=ok and seen['following_fixed']['pending_extension']
            if name=='vector-positive':ok=ok and seen['following_vectors']['pending_objects'] and seen['following_vectors']['pending_counts']==[2]
            if vector_n==27:ok=ok and seen['following_vectors']['complete'] and seen['following_vectors']['next_version']==7
        results.append(dict(mode=mode,name=name,passed=bool(ok),observed=seen))
for _,_,_,_,_,_,_,f,_,s in prepared:
    if sha(f.read_bytes())!=s:raise RuntimeError('self-authored input changed')
report=dict(passed=sum(r['passed'] for r in results),failed=sum(not r['passed'] for r in results),cases_per_mode=len(cases),results=results,commands=commands,elapsed_seconds=time.monotonic()-start,total_budget_seconds=180,original_payload_or_EXE_read=False,original_or_derived_game_executed=False,new_REA_query=False,main_written=False,pushed=False,driver_sha256=sha(Path(__file__).read_bytes()),component_sha256=sha((root/'file_io/include/city19_following_vectors_archive.hpp').read_bytes()),tool_sha256=sha((root/'file_io/tools/qualify_city19_following_consumers.cpp').read_bytes()))
(out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['passed','failed','elapsed_seconds']}));raise SystemExit(bool(report['failed']))
