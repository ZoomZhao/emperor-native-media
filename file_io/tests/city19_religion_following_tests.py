#!/usr/bin/env python3
"""Self-authored bounded Religion/Commerce fixtures; no original game execution."""
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
flood=struct.pack('<HIBIII',1,0xffffffff,0xff,0x80000000,0x12345678,0x87654321)+bytes((i*17+3)%256 for i in range(0xcb10))
festival=struct.pack('<H',4)+struct.pack('<H4I',1,21,22,23,24)+struct.pack('<8I',*range(101,109))+struct.pack('<IIBBIBIIII',201,202,1,2,203,3,204,205,206,207)
prefix=context+goal+flood+festival
assert len(prefix)==55501
embedded=[struct.pack('<H5I',2,0xffffffff,0x80000000,101+i,201+i,301+i) for i in range(12)]
children=[struct.pack('<H',3)+b''.join(embedded[i*3:i*3+3]) for i in range(4)]
raw20=struct.pack('<5I',0xdeadbeef,0x80000000,0xffffffff,17,18)
religion=struct.pack('<H',1)+b''.join(children)+raw20
commerce=struct.pack('<H',1)
assert len(religion)==294 and all(len(x)==68 for x in children)
cases=[]
def add(name,tail,valid=True,end=55797,error=None,checkpoint=0,wrapped=False,cuts=None):
    cases.append(dict(name=name,plain=prefix+tail,valid=valid,end=end,error=error,checkpoint=checkpoint,wrapped=wrapped,cuts=cuts))
add('four-children-three-embedded-each',religion+commerce)
add('Religion-complete-checkpoint',religion+b'UNREAD',end=55795,checkpoint=1)
add('Commerce-has-no-field-payload',religion+commerce+bytes(4096))
add('wrapped-full-segment',religion+commerce,wrapped=True)
add('large-unread-mainpool-tail',religion+commerce+bytes(100000))
add('cross-embedded-scalar-frames',religion+commerce,cuts=[32768,55501,55502,55503,55504,55505,55506,55507,55510,55514,55518,55522,55525,55571,55775,55794,55795,55796])
for v in [0,2,65535]:add('Religion-version-'+str(v),struct.pack('<H',v)+religion[2:]+commerce,False,error='Religion section version outside qualified source1 subset')
for child in range(4):
    start=2+child*68
    for v in [0,1,2,4]:add('child-'+str(child)+'-version-'+str(v),religion[:start]+struct.pack('<H',v)+religion[start+2:]+commerce,False,error='Worship child version outside qualified source3 subset')
for child in range(4):
    for member in range(3):
        start=2+child*68+2+member*22
        add('embedded-'+str(child)+'-'+str(member)+'-version1',religion[:start]+struct.pack('<H',1)+religion[start+2:]+commerce,False,error='Worship embedded version outside qualified source2 subset')
for n in [0,1,2,3,4,5,6,9,13,17,21,25,26,27,69,70,71,137,138,205,206,273,274,275,289,293]:add('Religion-truncated-'+str(n),religion[:n],False,error='truncated',checkpoint=1)
for n in [294,295]:add('Commerce-truncated-'+str(n-294),(religion+commerce)[:n],False,error='truncated')
for v in [0,2,65535]:add('Commerce-version-'+str(v),religion+struct.pack('<H',v),False,error='Commerce section version outside modern source1 subset')
add('input-sha',religion+commerce,False,error='input SHA mismatch')
add('decoded-sha',religion+commerce,False,error='decoded SHA mismatch')
add('bad-adler',religion+commerce,False,error='unqualified block',wrapped=True)
inputs=out/'self-authored-inputs';inputs.mkdir();prepared=[]
for case in cases:
    plain=case['plain'];cuts=case['cuts'] or list(range(32768,len(plain),32768));assert cuts==sorted(cuts)
    parts=[plain[x:y] for x,y in zip([0]+cuts,cuts+[len(plain)])];encoded=struct.pack('<I',4275878570)
    for i,part in enumerate(parts):
        if case['wrapped']:
            data=zlib.compress(part)
            if case['name']=='bad-adler' and i==0:data=data[:-1]+bytes([data[-1]^1])
            encoded+=struct.pack('<II',2882400255,len(data))+b'TEST'+data
        else:encoded+=struct.pack('<II',2863386095,len(part))+part
    path=inputs/(case['name']+'.bin');path.write_bytes(encoded);prepared.append((case,path,len(parts)))
before={x.name:sha(x) for x in inputs.iterdir()}
sdk=run(['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'],'sdk',30)
if sdk.returncode:raise RuntimeError('SDK unavailable')
for mode in ['release','ubsan']:
    exe=out/('religion-following-'+mode);flags=['-O2'] if mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined']
    q=run(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-isysroot',sdk.stdout.strip(),'-I',root/'file_io/include',root/'file_io/src/strict_zlib.cpp',root/'file_io/src/framed_reader.cpp',root/'file_io/tools/qualify_city19_religion_following.cpp','-lz','-o',exe],mode+'-build',60)
    if q.returncode:raise RuntimeError('bounded religion build failed')
    for case,path,blocks in prepared:
        q=run([exe,path,path.stat().st_size,262144,4915584,blocks,4275878570,2882400255,2863386095,36144,36144,'0'*64 if case['name']=='input-sha' else sha(path),'0'*64 if case['name']=='decoded-sha' else hashlib.sha256(case['plain']).hexdigest(),case['checkpoint']],mode+'-'+case['name']);seen=json.loads(q.stdout) if q.stdout else None
        if case['valid']:
            rel=seen['religion'] if seen else None
            passed=q.returncode==0 and q.stderr=='' and seen is not None and seen['parsed_end']==case['end'] and seen['postgoal_parsed_end']==55501 and seen['completed_members']==10 and len(seen['postgoal_sections'])==2 and rel['version']==1 and rel['end']-rel['start']==294 and rel['sha256']==hashlib.sha256(religion).hexdigest() and len(rel['children'])==4 and rel['raw20']['sha256']==hashlib.sha256(raw20).hexdigest() and seen['remaining_uninterpreted_decoded_bytes']==len(case['plain'])-case['end'] and seen['field_values_emitted'] is False
            for i,child in enumerate(rel['children']):
                passed=passed and child['version']==3 and child['end']-child['start']==68 and len(child['embedded'])==3
                for j,entry in enumerate(child['embedded']):passed=passed and entry['version']==2 and entry['end']-entry['start']==22 and entry['payload_sha256']==hashlib.sha256(embedded[i*3+j][2:]).hexdigest()
            passed=passed and ((seen['commerce'] is None) if case['checkpoint'] else seen['commerce']['end']-seen['commerce']['start']==2 and seen['commerce']['field_bytes']==0)
        else:passed=q.returncode==2 and not q.stdout and (not case['error'] or case['error'] in q.stderr)
        results.append({'mode':mode,'name':case['name'],'passed':passed,'observed':seen})
assert {x.name:sha(x) for x in inputs.iterdir()}==before
report={'passed':sum(x['passed'] for x in results),'failed':sum(not x['passed'] for x in results),'cases_per_mode':len(cases),'results':results,'commands':commands,'religion_header_sha256':sha(root/'file_io/include/city19_religion_archive.hpp'),'postgoal_header_sha256':sha(root/'file_io/include/city19_postgoals_archive.hpp'),'goal_header_sha256':sha(root/'file_io/include/city19_goal_archive.hpp'),'new_tool_sha256':sha(root/'file_io/tools/qualify_city19_religion_following.cpp'),'production_sources':ready['fixed_production_source_sha256'],'original_data_or_game_used':False,'legacy_numeric_adjustments_implemented':False,'total_budget_seconds':180}
(out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['passed','failed','cases_per_mode']}));raise SystemExit(bool(report['failed']))
