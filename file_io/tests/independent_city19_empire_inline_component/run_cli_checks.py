#!/usr/bin/env python3
"""Six bounded final Empire CLI cases per build."""
import argparse,hashlib,json,struct,subprocess,sys,time,zlib
from pathlib import Path
ROOT=Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
a=p.parse_args()
if sys.flags.optimize:p.error('optimized test invocation unsupported')
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
plain=base+header(2)+decl(b'Figure')+figure+decl(b'cPheasantFig')+pheasant+header(2)+(struct.pack('<H',1)+bytes([255])*268)*2+struct.pack('<HI',3,200)+bytes([255])*34
if len(base)!=55999 or len(plain)!=57419:raise RuntimeError('synthetic source widths')

prefix=plain[:57379]
record=struct.pack('<H',2)+bytes([255])*38
tail=struct.pack('<H',65535)+bytes([255])*62
cases=[]
for v in [1,2,3]:cases.append(('parent-'+str(v),prefix+struct.pack('<Hi',v,2)+record*2+tail,[4096,8192,1,8192,8192],57465,None,v,2,True))
cases.append(('checkpoint1',prefix+struct.pack('<Hi',3,2)+record*2+tail,[4096,8192,1,8192,1],57425,None,3,1,False))
cases.append(('legacy-child1',prefix+struct.pack('<Hi',3,1)+struct.pack('<H',1)+bytes(40),[4096,8192,1,8192,8192],None,'EmpireObject outside qualified explicit source2',3,0,False))
cases.append(('zero-budget',prefix+struct.pack('<Hi',3,1)+record,[4096,8192,1,8192,0],None,'Empire preceding context/modern budget',3,0,False))
inputs=out/'self-authored-inputs';inputs.mkdir();prepared=[]
for name,b,flags,end,error,v,n,complete in cases:
    cuts=[32768,57380,57381,57382,57385,57386,57424] if name=='parent-3' else [32768]
    parts=[b[i:j] for i,j in zip([0]+cuts,cuts+[len(b)])];encoded=struct.pack('<I',4275878570)
    for part in parts:
        packed=zlib.compress(part);encoded+=struct.pack('<II',2882400255,len(packed))+b'TEST'+packed
    f=inputs/(name+'.bin');f.write_bytes(encoded);prepared.append((name,b,flags,end,error,v,n,complete,f,len(parts),sha(encoded)))
sdk=run(['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'],'sdk',30)
if sdk.returncode:raise RuntimeError('SDK unavailable')
for mode in ['release','ubsan']:
    exe=out/('empire-'+mode);flags=['-O2'] if mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined']
    q=run(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-isysroot',sdk.stdout.strip(),'-I',root/'file_io/include',root/'file_io/src/strict_zlib.cpp',root/'file_io/src/framed_reader.cpp',root/'file_io/tools/qualify_city19_empire_inline.cpp','-lz','-o',exe],mode+'-build',60)
    if q.returncode:raise RuntimeError('bounded final Empire build failed')
    for name,b,flags,end,error,v,n,complete,f,blocks,input_sha in prepared:
        q=run([exe,f,f.stat().st_size,262144,4915584,blocks,4275878570,2882400255,2863386095,36144,36144,input_sha,sha(b),*flags],mode+'-'+name)
        seen=json.loads(q.stdout) if q.stdout else None
        if error:ok=q.returncode==2 and not q.stdout and error in q.stderr
        else:
            r=seen['empire_inline'] if seen else None
            ok=q.returncode==0 and not q.stderr and r is not None and seen['parsed_end']==end and seen['decoded_sha256']==sha(b) and seen['remaining_uninterpreted_decoded_bytes']==len(b)-end and r['version']==v and r['processed']==n and r['inline_complete']==complete and r['grids_required']==(v>=2) and r['parent4_required']==(v==3) and r['conditional_cache_next']==7 and r['full_Empire_complete'] is False and seen['Figure_activity_status_interpreted'] is False
        results.append(dict(mode=mode,name=name,passed=bool(ok),observed=seen))
for *_,f,blocks,input_sha in prepared:
    if sha(f.read_bytes())!=input_sha:raise RuntimeError('self-authored input changed')
report=dict(passed=sum(r['passed'] for r in results),failed=sum(not r['passed'] for r in results),cases_per_mode=6,results=results,commands=commands,elapsed_seconds=time.monotonic()-start,total_budget_seconds=180,original_payload_or_EXE_read=False,original_or_derived_game_executed=False,new_REA_query=False,main_written=False,pushed=False,driver_sha256=sha(Path(__file__).read_bytes()),component_sha256=sha((root/'file_io/include/city19_empire_inline_archive.hpp').read_bytes()),tool_sha256=sha((root/'file_io/tools/qualify_city19_empire_inline.cpp').read_bytes()))
(out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['passed','failed','elapsed_seconds']}));raise SystemExit(bool(report['failed']))
