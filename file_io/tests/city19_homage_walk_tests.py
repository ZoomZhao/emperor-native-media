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
homage=struct.pack('<HIHII',1,501,3,502,503)
homage2=struct.pack('<HIHII',1,601,3,602,603)
if len(homage)!=16:raise RuntimeError('source Homage widths2+4+2+4+4')
solo=mh(1)+decl(b'cHomageGoal')+homage
add('homage-alone',solo+mh(0)*9,True,3448,1,3)
add('homage-checkpoint',solo+b'UNKNOWN NEXT',True,3394,1,3,checkpoint=1)
add('homage-class-reference',solo+mh(1)+struct.pack('<H',32769)+homage2+mh(0)*8,True,3466,2,4)
add('homage-object-reference',solo+mh(1)+struct.pack('<H',2)+mh(0)*8,True,3450,2,3)
add('homage-extended-class-reference',solo+mh(1)+struct.pack('<HI',32767,2147483649)+homage2+mh(0)*8,True,3470,2,4)
combo=mh(2)+decl(b'cHousingGoal')+body+decl(b'cHomageGoal')+homage
add('same-member-Housing-Homage',combo+mh(0)*9,True,3486,2,5)
add('Housing-Homage-cache-class3',combo+mh(1)+struct.pack('<H',32771)+homage2+mh(0)*8,True,3504,3,6)
add('Housing-Homage-cache-object4',combo+mh(1)+struct.pack('<H',4)+mh(0)*8,True,3488,3,5)
add('duplicate-homage-declaration',solo+mh(1)+decl(b'cHomageGoal')+homage2+mh(0)*8,True,3481,2,5)
add('unknown-after-homage',solo+mh(1)+decl(b'cOtherGoal')+b'UNQUALIFIED',True,3416,1,3,True)
add('unknown-before-homage',mh(1)+decl(b'cOtherGoal')+homage,True,3377,0,1,True)
add('wrapped-homage',solo+mh(0)*9,True,3448,1,3,compressed=True)
whole=context+solo;cuts=[1,3,45,47,95,97,99,3355,3357,3361,3363,3365,3367,3378,3379,3380,3384,3385,3386,3390,3394]
add('cross-block-homage',solo,True,3394,1,3,checkpoint=1,parts=[whole[x:y] for x,y in zip([0]+cuts,cuts)])
for label,version in [('legacy1-unqualified',1),('legacy2-unqualified',2),('future4-unqualified',4)]:
    changed=homage[:6]+struct.pack('<H',version)+homage[8:]
    add(label,mh(1)+decl(b'cHomageGoal')+changed,False,error='Homage goal version outside qualified source3 subset')
add('base-version-unqualified',mh(1)+decl(b'cHomageGoal')+struct.pack('<H',2)+homage[2:],False,error='base goal version outside modern scope1')
add('schema-mismatch',mh(1)+decl(b'cHomageGoal',1)+homage,False,error='Homage schema mismatch')
for n in [len(solo)-16,len(solo)-15,len(solo)-14,len(solo)-10,len(solo)-9,len(solo)-8,len(solo)-4,len(solo)-1]:
    add('homage-truncated-'+str(n),solo[:n],False,error='truncated')
add('input-sha',solo,False,checkpoint=1);add('decoded-sha',solo,False,checkpoint=1)
add('bad-adler',solo,False,checkpoint=1,compressed=True)
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
    exe=out/('homage-walk-'+mode);flags=['-O2'] if mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined']
    q=run(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-isysroot',sdk.stdout.strip(),'-I',root/'file_io/include',root/'file_io/src/strict_zlib.cpp',root/'file_io/src/framed_reader.cpp',root/'file_io/tools/qualify_city19_homage_walk.cpp','-lz','-o',exe],mode+'-build',60)
    if q.returncode:raise RuntimeError('bounded component build failed')
    for case,path,blocks in prepared:
        q=run([exe,path,path.stat().st_size,262144,4915584,blocks,4275878570,2882400255,2863386095,36144,36144,'0'*64 if case['name']=='input-sha' else sha(path),'0'*64 if case['name']=='decoded-sha' else hashlib.sha256(case['plain']).hexdigest(),case['checkpoint']],mode+'-'+case['name']);seen=json.loads(q.stdout) if q.stdout else None
        if case['valid']:
            passed=q.returncode==0 and seen is not None and seen['parsed_end']==case['end'] and seen['known_items']==case['items'] and seen['conditional_cache_next_id']==case['cache'] and (seen['pending_class'] is not None)==case['pending'] and seen['cache_runtime_state_proved'] is False and seen['field_values_emitted'] is False and seen['remaining_uninterpreted_decoded_bytes']==len(case['plain'])-case['end']
            if case['name']=='homage-checkpoint':
                passed=passed and seen['items'][0]['payload_sha256']==hashlib.sha256(homage).hexdigest()
            if case['name']=='Housing-Homage-cache-class3':
                passed=passed and seen['items'][1]['payload_sha256']==hashlib.sha256(homage).hexdigest() and seen['items'][2]['payload_sha256']==hashlib.sha256(homage2).hexdigest()
            if case['name'] in ['golden3399','golden3427','cross-block-golden3427']:
                passed=passed and seen['items'][0]['payload_sha256']==hashlib.sha256(body).hexdigest()
                if case['items']==2:passed=passed and seen['items'][1]['payload_sha256']==hashlib.sha256(second).hexdigest()
            if 'object-reference' in case['name']:passed=passed and seen['items'][-1]['payload_bytes']==0
        else:passed=q.returncode==2 and not q.stdout and (not case['error'] or case['error'] in q.stderr)
        results.append({'mode':mode,'name':case['name'],'passed':passed,'observed':seen})
if {x.name:sha(x) for x in inputs.iterdir()}!=before:raise RuntimeError('self-authored inputs changed')
report={'passed':sum(x['passed'] for x in results),'failed':sum(not x['passed'] for x in results),'cases_per_mode':len(cases),'results':results,'commands':commands,
        'component_source_sha256':sha(root/'file_io/include/city19_goal_archive.hpp'),'tool_source_sha256':sha(root/'file_io/tools/qualify_city19_homage_walk.cpp'),
        'production_sources':ready['fixed_production_source_sha256'],'original_data_or_game_used':False,'total_budget_seconds':180}
(out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['passed','failed','cases_per_mode']}));raise SystemExit(bool(report['failed']))
