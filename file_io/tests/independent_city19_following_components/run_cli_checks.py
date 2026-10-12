#!/usr/bin/env python3
"""Seven bounded self-authored Figure/Pheasant/Goods integration cases per build."""
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
bad=bytearray(plain);bad[56414]=255
cases=[('goods-mixed-complete','goods',plain,[4096,8192,1,8192],57379,None,False),('goods-split-words','goods',plain,[4096,8192,1,8192],57379,None,True),('goods-checkpoint1','goods',plain,[4096,8192,1,1],57109,None,False),('goods-extension-blocked','goods',bytes(bad),[4096,8192,1,8192],None,'GoodsControl preceding context',False),('variants-disabled','variants',plain,[4096,8192,0],56433,None,False),('variants-enabled','variants',plain,[4096,8192,1],56833,None,False),('goods-zero-budget','goods',plain,[4096,8192,1,0],None,'GoodsControl preceding context',False)]
prepared=[];inputs=out/'self-authored-inputs';inputs.mkdir()
for name,tool,b,flags,end,error,split in cases:
    cuts=[32768,56018,56019,56433,56434,56435,56832,56833,56834,56835] if split else [32768]
    parts=[b[i:j] for i,j in zip([0]+cuts,cuts+[len(b)])];encoded=struct.pack('<I',4275878570)
    for part in parts:
        packed=zlib.compress(part) if split else part
        encoded+=struct.pack('<II',2882400255 if split else 2863386095,len(packed))+(b'TEST' if split else b'')+packed
    f=inputs/(name+'.bin');f.write_bytes(encoded);prepared.append((name,tool,b,flags,end,error,f,len(parts),sha(encoded)))
sdk=run(['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'],'sdk',30)
if sdk.returncode:raise RuntimeError('SDK unavailable')
for mode in ['release','ubsan']:
    executables={}
    for tool,source in [('goods','qualify_city19_goodscontrol.cpp'),('variants','qualify_city19_figure_variants.cpp')]:
        exe=out/(tool+'-'+mode);flags=['-O2'] if mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined']
        q=run(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-isysroot',sdk.stdout.strip(),'-I',root/'file_io/include',root/'file_io/src/strict_zlib.cpp',root/'file_io/src/framed_reader.cpp',root/'file_io/tools'/source,'-lz','-o',exe],mode+'-'+tool+'-build',60)
        if q.returncode:raise RuntimeError('bounded integration build failed')
        executables[tool]=exe
    for name,tool,b,flags,end,error,f,blocks,input_sha in prepared:
        q=run([executables[tool],f,f.stat().st_size,262144,4915584,blocks,4275878570,2882400255,2863386095,36144,36144,input_sha,sha(b),*flags],mode+'-'+name)
        seen=json.loads(q.stdout) if q.stdout else None
        if error:ok=q.returncode==2 and not q.stdout and error in q.stderr
        else:
            ok=q.returncode==0 and not q.stderr and seen is not None and seen['parsed_end']==end and seen['decoded_sha256']==sha(b) and seen['remaining_uninterpreted_decoded_bytes']==len(b)-end and not seen['Figure_activity_status_interpreted']
            if tool=='goods':ok=ok and seen['goods']['conditional_cache_next']==7 and seen['goods']['processed']==(1 if name=='goods-checkpoint1' else 2) and seen['goods']['complete']==(name!='goods-checkpoint1')
            else:ok=ok and seen['afterpool']['processed']==(1 if name=='variants-disabled' else 2) and seen['afterpool']['cache_next']==(5 if name=='variants-disabled' else 7)
        results.append(dict(mode=mode,name=name,passed=bool(ok),observed=seen))
for _,_,_,_,_,_,f,_,s in prepared:
    if sha(f.read_bytes())!=s:raise RuntimeError('self-authored input changed')
report=dict(passed=sum(r['passed'] for r in results),failed=sum(not r['passed'] for r in results),cases_per_mode=7,results=results,commands=commands,elapsed_seconds=time.monotonic()-start,total_budget_seconds=180,original_payload_or_EXE_read=False,original_or_derived_game_executed=False,new_REA_query=False,main_written=False,pushed=False,driver_sha256=sha(Path(__file__).read_bytes()))
(out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['passed','failed','elapsed_seconds']}));raise SystemExit(bool(report['failed']))
