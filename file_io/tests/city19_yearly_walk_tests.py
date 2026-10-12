#!/usr/bin/env python3
"""Independent wire fixtures for the reusable bounded Goal metadata component."""
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
cases=[]
def add(name,tail,valid,end=None,items=None,cache=None,pending=False,checkpoint=0,error=None,compressed=False,parts=None):
    cases.append({'name':name,'plain':context+tail,'valid':valid,'end':end,'items':items,'cache':cache,'pending':pending,'checkpoint':checkpoint,'error':error,'compressed':compressed,'parts':parts})
yearly=struct.pack('<HIHIII',1,901,1,902,903,904)
yearly2=struct.pack('<HIHIII',1,1001,1,1002,1003,1004)
if len(yearly)!=20:raise RuntimeError('source Yearly widths2+4+2+12')
solo=mh(1)+decl(b'cYearlyProductionGoal')+yearly
add('yearly-alone',solo+mh(0)*9,True,3462,1,3)
add('yearly-checkpoint',solo+b'UNKNOWN',True,3408,1,3,checkpoint=1)
add('yearly-class-ref',solo+mh(1)+struct.pack('<H',32769)+yearly2+mh(0)*8,True,3484,2,4)
add('yearly-object-ref',solo+mh(1)+struct.pack('<H',2)+mh(0)*8,True,3464,2,3)
add('yearly-extended-class-ref',solo+mh(1)+struct.pack('<HI',32767,2147483649)+yearly2+mh(0)*8,True,3488,2,4)
add('duplicate-Yearly',solo+mh(1)+decl(b'cYearlyProductionGoal')+yearly2+mh(0)*8,True,3509,2,5)
combo=mh(2)+decl(b'cHousingGoal')+body+decl(b'cYearlyProductionGoal')+yearly
add('shared-Housing-Yearly-distinctclasses',combo+mh(0)*9,True,3500,2,5)
add('shared-pattern-cache-class3',combo+mh(1)+struct.pack('<H',32771)+yearly2+mh(0)*8,True,3522,3,6)
add('shared-pattern-cache-object4',combo+mh(1)+struct.pack('<H',4)+mh(0)*8,True,3502,3,5)
add('unknown-after-Yearly',solo+mh(1)+decl(b'cUnknownGoal')+b'UNQUALIFIED',True,3432,1,3,True)
add('wrapped-Yearly',solo+mh(0)*9,True,3462,1,3,compressed=True)
whole=context+solo;cuts=[1,3,45,47,95,97,99,3355,3357,3361,3363,3365,3367,3388,3389,3390,3394,3395,3396,3400,3404,3408]
add('cross-block-Yearly',solo,True,3408,1,3,checkpoint=1,parts=[whole[x:y] for x,y in zip([0]+cuts,cuts)])
for version in [0,2,65535]:add('Yearly-version-'+str(version),mh(1)+decl(b'cYearlyProductionGoal')+yearly[:6]+struct.pack('<H',version)+yearly[8:],False,error='Yearly production goal version outside modern source1 subset')
add('shared-base-version',mh(1)+decl(b'cYearlyProductionGoal')+struct.pack('<H',2)+yearly[2:],False,error='base goal version outside modern scope1')
add('schema-mismatch',mh(1)+decl(b'cYearlyProductionGoal',1)+yearly,False,error='Yearly schema mismatch')
for n in [len(solo)-20,len(solo)-19,len(solo)-18,len(solo)-14,len(solo)-13,len(solo)-12,len(solo)-8,len(solo)-4,len(solo)-1]:add('yearly-truncated-'+str(n),solo[:n],False,error='truncated')
add('class-kind-mismatch',solo+mh(1)+struct.pack('<H',32770),False,error='cache kind mismatch: class expected')
add('object-kind-mismatch',solo+mh(1)+struct.pack('<H',1),False,error='cache kind mismatch: object expected')
add('item-budget',mh(129)+decl(b'cYearlyProductionGoal')+yearly+struct.pack('<H',2)*128,False,error='item budget exhausted')
add('cache-budget',mh(128)+(decl(b'cYearlyProductionGoal')+yearly)*128,False,error='cache budget exhausted')
add('input-sha',solo,False,checkpoint=1);add('decoded-sha',solo,False,checkpoint=1);add('bad-adler',solo,False,checkpoint=1,compressed=True)
inputs=out/'self-authored-inputs';inputs.mkdir();prepared=[]
for case in cases:
    parts=case['parts'] or [case['plain']];encoded=struct.pack('<I',4275878570)
    for part in parts:
        if case['compressed']:
            data=zlib.compress(part)
            if case['name']=='bad-adler':data=data[:-1]+bytes([data[-1]^1])
            encoded+=struct.pack('<II',2882400255,len(data))+b'TEST'+data
        else:encoded+=struct.pack('<II',2863386095,len(part))+part
    path=inputs/(case['name']+'.bin');path.write_bytes(encoded);prepared.append((case,path,len(parts)))
before={x.name:sha(x) for x in inputs.iterdir()}
sdk=run(['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'],'sdk',30)
if sdk.returncode:raise RuntimeError('SDK unavailable')
for mode in ['release','ubsan']:
    exe=out/('yearly-walk-'+mode);flags=['-O2'] if mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined']
    q=run(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-isysroot',sdk.stdout.strip(),'-I',root/'file_io/include',root/'file_io/src/strict_zlib.cpp',root/'file_io/src/framed_reader.cpp',root/'file_io/tools/qualify_city19_yearly_walk.cpp','-lz','-o',exe],mode+'-build',60)
    if q.returncode:raise RuntimeError('bounded component build failed')
    for case,path,blocks in prepared:
        q=run([exe,path,path.stat().st_size,262144,4915584,blocks,4275878570,2882400255,2863386095,36144,36144,'0'*64 if case['name']=='input-sha' else sha(path),'0'*64 if case['name']=='decoded-sha' else hashlib.sha256(case['plain']).hexdigest(),case['checkpoint']],mode+'-'+case['name']);seen=json.loads(q.stdout) if q.stdout else None
        if case['valid']:
            passed=q.returncode==0 and seen is not None and seen['parsed_end']==case['end'] and seen['known_items']==case['items'] and seen['conditional_cache_next_id']==case['cache'] and (seen['pending_class'] is not None)==case['pending'] and seen['cache_runtime_state_proved'] is False and seen['field_values_emitted'] is False and seen['remaining_uninterpreted_decoded_bytes']==len(case['plain'])-case['end']
            if case['name']=='yearly-checkpoint':passed=passed and seen['items'][0]['payload_sha256']==hashlib.sha256(yearly).hexdigest()
            if case['name']=='shared-pattern-cache-class3':passed=passed and seen['items'][1]['payload_sha256']==hashlib.sha256(yearly).hexdigest() and seen['items'][2]['payload_sha256']==hashlib.sha256(yearly2).hexdigest()
            if case['name'] in ['golden3399','golden3427','cross-block-golden3427']:
                passed=passed and seen['items'][0]['payload_sha256']==hashlib.sha256(body).hexdigest()
                if case['items']==2:passed=passed and seen['items'][1]['payload_sha256']==hashlib.sha256(second).hexdigest()
            if 'object-reference' in case['name']:passed=passed and seen['items'][-1]['payload_bytes']==0
        else:passed=q.returncode==2 and not q.stdout and (not case['error'] or case['error'] in q.stderr)
        results.append({'mode':mode,'name':case['name'],'passed':passed,'observed':seen})
if {x.name:sha(x) for x in inputs.iterdir()}!=before:raise RuntimeError('self-authored inputs changed')
report={'passed':sum(x['passed'] for x in results),'failed':sum(not x['passed'] for x in results),'cases_per_mode':len(cases),'results':results,'commands':commands,
        'component_source_sha256':sha(root/'file_io/include/city19_goal_archive.hpp'),'tool_source_sha256':sha(root/'file_io/tools/qualify_city19_yearly_walk.cpp'),
        'production_sources':ready['fixed_production_source_sha256'],'original_data_or_game_used':False,'total_budget_seconds':180}
(out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['passed','failed','cases_per_mode']}));raise SystemExit(bool(report['failed']))
