#!/usr/bin/env python3
"""Self-authored bounded postgoal Flood fixtures; no original game execution."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import time
import zlib
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
a=p.parse_args()
if sys.flags.optimize:p.error('optimized test invocation unsupported')
root=a.source_root.resolve();out=a.output.resolve()
if out.exists():p.error('fresh output required')
sha=lambda x:hashlib.sha256(x.read_bytes()).hexdigest();ready=json.loads((root/'docs/original-profile-owned-readiness.json').read_text())
for name,h in ready['fixed_production_source_sha256'].items():
    if sha(root/name)!=h:raise RuntimeError('reviewed production source mismatch')
out.mkdir(parents=True);start=time.monotonic();commands=[];results=[]
def run(argv,label,timeout=10):
    left=180-(time.monotonic()-start)
    if left<=0:raise RuntimeError('180-second test budget exhausted')
    q=subprocess.run([str(x) for x in argv],capture_output=True,text=True,timeout=min(left,timeout))
    (out/(label+'.stdout')).write_text(q.stdout);(out/(label+'.stderr')).write_text(q.stderr)
    commands.append({'argv':[str(x) for x in argv],'exit_code':q.returncode,'stdout_sha256':hashlib.sha256(q.stdout.encode()).hexdigest(),'stderr_sha256':hashlib.sha256(q.stderr.encode()).hexdigest()});return q
first=struct.pack('<HHBIII',19,6,1,11,12,13)+b'abcdefghijklmnop'+struct.pack('<III',14,15,16)
context=first+struct.pack('<HIII8II',2,21,22,23,*range(101,109),24)+struct.pack('<HH',3,4)+bytes((i*17+5)%256 for i in range(3256))
mh=lambda n:struct.pack('<Hi',1,n)
decl=lambda name,schema=0:struct.pack('<HHH',65535,schema,len(name))+name
body=struct.pack('<HIHIII',1,101,1,102,103,104)
second=struct.pack('<HIHIII',1,201,1,202,203,204)
one=mh(1)+decl(b'cHousingGoal')+body
if len(context)!=3355 or len(context+one)!=3399:raise RuntimeError('independent source fixture sizes')
goal=mh(0)*10
array=bytes((i*17+3)%256 for i in range(0xcb10))
flood=struct.pack('<HIBIII',1,0xffffffff,0xff,0x80000000,0x12345678,0x87654321)+array
assert len(context+goal)==3415 and len(flood)==52003
cases=[]
def add(name,plain,valid=True,end=55418,error=None,wrapped=False,cuts=None,festival=False):
    cases.append(dict(name=name,plain=plain,valid=valid,end=end,error=error,wrapped=wrapped,cuts=cuts,festival=festival))
golden=context+goal+flood
add('golden-Flood52003',golden)
add('wrapped-Flood52003',golden,wrapped=True)
add('following-owner-poison-left',golden+b'NEXT_OWNER'*7)
add('bounded-prefix-with-large-unread-tail',golden+bytes(100000))
pop=mh(1)+decl(b'cPopulationGoal')+struct.pack('<HIHI',1,11,1,12)+mh(0)*9
add('Population-goal-end-not-hardcoded3736',context+pop+flood,end=55451)
add('cross-frame-Flood',golden,cuts=[1,3,45,47,95,97,99,3355,3415,3416,3417,3421,3422,3426,3430,3434,32768,55417])
for v in [0,2,65535]:add('Flood-version-'+str(v),context+goal+struct.pack('<H',v)+flood[2:],False,error='Flood section version outside qualified source1 subset')
for n in [0,1,2,5,6,7,10,11,14,15,18,19,20,4096,32768,52002]:add('Flood-truncated-'+str(n),context+goal+flood[:n],False,error='truncated')
add('goal-loop-incomplete',context+mh(0)*9,False,error='truncated')
add('goal-loop-unknown-class',context+mh(1)+decl(b'cUnknownGoal')+flood,False,error='post-goal prefix/baseline')
add('input-sha',golden,False,error='input SHA mismatch')
add('decoded-sha',golden,False,error='decoded SHA mismatch')
add('bad-adler',golden,False,error='unqualified block',wrapped=True)
festival=struct.pack('<H',4)+struct.pack('<H4I',1,21,22,23,24)+struct.pack('<8I',*range(101,109))+struct.pack('<IIBBIBIIII',201,202,1,2,203,3,204,205,206,207)
assert len(festival)==83
add('Festival-full-source4-child1',golden+festival,end=55501,festival=True)
add('Festival-wrapped',golden+festival,end=55501,festival=True,wrapped=True)
add('Festival-third-consumer-poison-left',golden+festival+b'THIRD_OWNER',end=55501,festival=True)
add('Festival-scalar-cross-frames',golden+festival,end=55501,festival=True,cuts=[32768,55418,55419,55420,55421,55422,55425,55426,55429,55430,55433,55434,55437,55438,55470,55478,55479,55480,55484,55485,55500])
for v in [0,1,2,3,5,65535]:add('Festival-own-version-'+str(v),golden+struct.pack('<H',v)+festival[2:],False,error='Festival section version outside qualified source4 subset',festival=True)
for v in [0,2,65535]:add('Festival-child-version-'+str(v),golden+festival[:2]+struct.pack('<H',v)+festival[4:],False,error='Festival embedded member version outside qualified source1 subset',festival=True)
for n in [0,1,2,3,4,7,8,11,12,15,16,19,20,23,35,51,52,53,54,58,59,63,67,71,75,79,82]:add('Festival-truncated-'+str(n),golden+festival[:n],False,error='truncated',festival=True)
inputs=out/'self-authored-inputs';inputs.mkdir();prepared=[]
for case in cases:
    plain=case['plain'];cuts=case['cuts'] or list(range(32768,len(plain),32768));assert cuts==sorted(cuts)
    parts=[plain[x:y] for x,y in zip([0]+cuts,cuts+[len(plain)])];encoded=struct.pack('<I',4275878570)
    for i,part in enumerate(parts):
        if case['wrapped']:
            compressed=zlib.compress(part)
            if case['name']=='bad-adler' and i==0:compressed=compressed[:-1]+bytes([compressed[-1]^1])
            encoded+=struct.pack('<II',2882400255,len(compressed))+b'TEST'+compressed
        else:encoded+=struct.pack('<II',2863386095,len(part))+part
    path=inputs/(case['name']+'.bin');path.write_bytes(encoded);prepared.append((case,path,len(parts)))
before={x.name:sha(x) for x in inputs.iterdir()}
sdk=run(['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'],'sdk',30)
if sdk.returncode:raise RuntimeError('SDK unavailable')
for mode in ['release','ubsan']:
    exe=out/('postgoals-'+mode);flags=['-O2'] if mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined']
    q=run(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-isysroot',sdk.stdout.strip(),'-I',root/'file_io/include',root/'file_io/src/strict_zlib.cpp',root/'file_io/src/framed_reader.cpp',root/'file_io/tools/qualify_city19_postgoals.cpp','-lz','-o',exe],mode+'-build',60)
    if q.returncode:raise RuntimeError('bounded postgoal component build failed')
    for case,path,blocks in prepared:
        q=run([exe,path,path.stat().st_size,262144,4915584,blocks,4275878570,2882400255,2863386095,36144,36144,'0'*64 if case['name']=='input-sha' else sha(path),'0'*64 if case['name']=='decoded-sha' else hashlib.sha256(case['plain']).hexdigest(),0 if case['festival'] else 1],mode+'-'+case['name']);seen=json.loads(q.stdout) if q.stdout else None
        if case['valid']:
            end=case['end'];section=seen['postgoal_sections'][0] if seen else None
            passed=q.returncode==0 and q.stderr=='' and seen is not None and seen['parsed_end']==end and seen['completed_members']==10 and len(seen['postgoal_sections'])==(2 if case['festival'] else 1) and section['name']=='cFlood' and section['version']==1 and section['end']-section['start']==52003 and section['payload_sha256']==hashlib.sha256(flood).hexdigest() and [x['bytes'] for x in section['fields']]==[4,1,4,4,4,51984] and section['fields'][-1]['sha256']==hashlib.sha256(array).hexdigest() and section['fields'][-1]['offset']==section['end']-51984 and seen['remaining_uninterpreted_decoded_bytes']==len(case['plain'])-end and seen['retained_prefix_bytes']==min(len(case['plain']),65536) and seen['field_values_emitted'] is False
            if case['festival']:
                f=seen['postgoal_sections'][1]
                passed=passed and f['name']=='cFestival' and f['version']==4 and f['embedded_version']==1 and f['end']-f['start']==83 and f['payload_sha256']==hashlib.sha256(festival).hexdigest() and len(f['fields'])==22 and [x['bytes'] for x in f['fields']]==[4]*14+[1,1,4,1,4,4,4,4]
        else:passed=q.returncode==2 and not q.stdout and (not case['error'] or case['error'] in q.stderr)
        results.append({'mode':mode,'name':case['name'],'passed':passed,'observed':seen})
assert {x.name:sha(x) for x in inputs.iterdir()}==before
report={'passed':sum(x['passed'] for x in results),'failed':sum(not x['passed'] for x in results),'cases_per_mode':len(cases),'results':results,'commands':commands,'postgoal_component_sha256':sha(root/'file_io/include/city19_postgoals_archive.hpp'),'goal_component_sha256':sha(root/'file_io/include/city19_goal_archive.hpp'),'tool_sha256':sha(root/'file_io/tools/qualify_city19_postgoals.cpp'),'production_sources':ready['fixed_production_source_sha256'],'original_data_or_game_used':False,'total_budget_seconds':180}
(out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['passed','failed','cases_per_mode']}));raise SystemExit(bool(report['failed']))
