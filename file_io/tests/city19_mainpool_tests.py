#!/usr/bin/env python3
"""Self-authored mainpool wire/cache/resource fixtures; no original game execution."""
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
embedded=struct.pack('<H5I',2,11,12,13,14,15);child=struct.pack('<H',3)+embedded*3
religion=struct.pack('<H',1)+child*4+bytes(20);commerce=struct.pack('<H',1)
preceding=flood+festival+religion+commerce
prefix=context+goal+preceding
assert len(prefix)==55797
widths=[1,1,1,1,1,1,2,2,4,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,2,2,2,1,1,2,1,1,1,1,2,2,2,1,1,1,2,4,1,1,1,1,1,2,1,1,4,2,1,1,1,2,2,4,4,4,4,4,4,1,2,4,4,4,4,4,4,2,2,2,4,16]
assert len(widths)==83 and sum(widths)==180
building=struct.pack('<H',5)+b''.join(bytes([i+1])*n for i,n in enumerate(widths))
second=struct.pack('<H',5)+b''.join(bytes([i+101])*n for i,n in enumerate(widths))
pool=lambda count:struct.pack('<Hi',1,count)
bdecl=decl(b'Building')
cases=[]
def add(name,tail,valid=True,end=55999,items=1,cache=3,pending=False,error=None,maxitems=4096,wrapped=False,sourceprefix=None):
    cases.append(dict(name=name,plain=(prefix if sourceprefix is None else sourceprefix)+tail,valid=valid,end=end,items=items,cache=cache,pending=pending,error=error,maxitems=maxitems,wrapped=wrapped))
add('zero-pool-no-byte-padding',pool(0)+b'NEXT',end=55803,items=0,cache=1)
add('Building5-83-fields180',pool(1)+bdecl+building)
add('Building-sameclass-freshobject',pool(2)+bdecl+building+struct.pack('<H',32769)+second,end=56183,items=2,cache=4)
add('Building-object2-ref-no-payload',pool(2)+bdecl+building+struct.pack('<H',2)+b'POISON',end=56001,items=2,cache=3)
add('Building-extended-class1',pool(2)+bdecl+building+struct.pack('<HI',32767,2147483649)+second,end=56187,items=2,cache=4)
add('Building-extended-object2',pool(2)+bdecl+building+struct.pack('<HI',32767,2),end=56005,items=2,cache=3)
add('duplicate-Building-class',pool(2)+(bdecl+building)*2,end=56195,items=2,cache=5)
add('unknown-first-before-payload',pool(4000)+decl(b'Granary')+b'OPAQUE',end=55816,items=0,cache=1,pending=True)
add('unknown-after-Building',pool(2)+bdecl+building+decl(b'Market')+b'OPAQUE',end=56011,items=1,cache=3,pending=True)
add('maxitem-checkpoint-not-nexttag',pool(2)+bdecl+building+b'UNQUALIFIED',maxitems=1)
add('wrapped-Building',pool(1)+bdecl+building,wrapped=True)
housing=mh(1)+decl(b'cHousingGoal')+struct.pack('<HIHIII',1,21,1,22,23,24)+mh(0)*9
priorgoal=context+housing+preceding
assert len(priorgoal)==55835
add('carry-goal-class1-object2-next3',pool(1)+bdecl+building,end=56037,cache=5,sourceprefix=priorgoal)
add('earlier-goal-class-incompatible',pool(1)+struct.pack('<H',32769),False,error='earlier goal class',sourceprefix=priorgoal)
add('earlier-goal-object-incompatible',pool(1)+struct.pack('<H',2),False,error='earlier goal object',sourceprefix=priorgoal)
add('modern-count8192-unknown-first',pool(8192)+decl(b'Granary'),end=55816,items=0,cache=1,pending=True)
add('count8193-modern-reject-not-source4000',pool(8193)+bdecl+building,False,error='pool count exceeds modern budget')
add('negative-count',pool(-1),False,error='negative pool count')
add('negative-min-count',pool(-2147483648),False,error='negative pool count')
for v in [0,2,65535]:add('pool-version-'+str(v),struct.pack('<Hi',v,1)+bdecl+building,False,error='pool section version outside qualified source1 subset')
for v in [0,1,4,6,65535]:add('Building-field-version-'+str(v),pool(1)+bdecl+struct.pack('<H',v)+building[2:],False,error='Building field section outside qualified source5 subset')
add('Building-schema1',pool(1)+decl(b'Building',1)+building,False,error='Building schema mismatch')
for n in [0,1,2,3,5]:add('pool-header-truncated-'+str(n),pool(1)[:n],False,error='truncated')
for n in [0,1,2,3,4,5,6,13]:add('pool-declaration-truncated-'+str(n),pool(1)+bdecl[:n],False,error='truncated')
for n in [0,1,2,3,8,13,18,32,64,128,166,181]:add('Building-payload-truncated-'+str(n),pool(1)+bdecl+building[:n],False,error='truncated')
add('pool-null-positive',pool(1)+struct.pack('<H',0),False,error='pool nonnull consumer unsupported')
add('pool-unqualified-classref',pool(1)+struct.pack('<H',32769),False,error='unqualified pool class reference')
add('pool-unqualified-objectref',pool(1)+struct.pack('<H',1),False,error='unqualified pool object reference')
add('class-kind-mismatch',pool(2)+bdecl+building+struct.pack('<H',32770),False,error='cache kind mismatch class expected')
add('object-kind-mismatch',pool(2)+bdecl+building+struct.pack('<H',1),False,error='cache kind mismatch object expected')
add('extended80007fff-is-reference',pool(1)+struct.pack('<HI',32767,2147516415),False,error='unqualified pool class reference')
add('input-sha',pool(1)+bdecl+building,False,error='input SHA mismatch')
add('decoded-sha',pool(1)+bdecl+building,False,error='decoded SHA mismatch')
add('bad-adler',pool(1)+bdecl+building,False,error='unqualified block',wrapped=True)
# Full source fill-threshold-sized batch is a bounded modern iteration, not original allocation.
many=pool(4000)+bdecl+building+(struct.pack('<H',32769)+building)*3999
add('modern4000-continuous-Building-batch',many,end=len(prefix)+len(many),items=4000,cache=4002,wrapped=True)
inputs=out/'self-authored-inputs';inputs.mkdir();prepared=[]
for case in cases:
    plain=case['plain'];parts=[plain[i:i+32768] for i in range(0,len(plain),32768)];encoded=struct.pack('<I',4275878570)
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
    exe=out/('mainpool-'+mode);flags=['-O2'] if mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined']
    q=run(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-isysroot',sdk.stdout.strip(),'-I',root/'file_io/include',root/'file_io/src/strict_zlib.cpp',root/'file_io/src/framed_reader.cpp',root/'file_io/tools/qualify_city19_mainpool.cpp','-lz','-o',exe],mode+'-build',60)
    if q.returncode:raise RuntimeError('bounded mainpool build failed')
    for case,path,blocks in prepared:
        q=run([exe,path,path.stat().st_size,262144,4915584,blocks,4275878570,2882400255,2863386095,36144,36144,'0'*64 if case['name']=='input-sha' else sha(path),'0'*64 if case['name']=='decoded-sha' else hashlib.sha256(case['plain']).hexdigest(),case['maxitems']],mode+'-'+case['name']);seen=json.loads(q.stdout) if q.stdout else None
        if case['valid']:
            seenpool=seen['pool'] if seen else None
            passed=q.returncode==0 and q.stderr=='' and seen is not None and seen['parsed_end']==case['end'] and seenpool['processed']==case['items'] and seenpool['cache_next']==case['cache'] and (seenpool['pending_class'] is not None)==case['pending'] and seen['remaining_uninterpreted_decoded_bytes']==len(case['plain'])-case['end'] and seen['field_values_emitted'] is False
            if case['items']:
                passed=passed and seenpool['items'][0]['payload_bytes']==182 and seenpool['items'][0]['payload_sha256']==hashlib.sha256(building).hexdigest()
            if case['name'] in ('Building-object2-ref-no-payload','Building-extended-object2'):passed=passed and seenpool['items'][-1]['object_reference'] and seenpool['items'][-1]['payload_bytes']==0
            if case['name']=='carry-goal-class1-object2-next3':passed=passed and seenpool['cache_start']==3 and seenpool['items'][0]['class_model_id']==3 and seenpool['items'][0]['object_model_id']==4
            if case['name']=='modern4000-continuous-Building-batch':passed=passed and seenpool['declared_items_complete'] and seenpool['items'][-1]['object_model_id']==4001
        else:passed=q.returncode==2 and not q.stdout and (not case['error'] or case['error'] in q.stderr)
        results.append({'mode':mode,'name':case['name'],'passed':passed,'observed':seen if case['items']<100 else {'parsed_end':seen['parsed_end'],'processed':seen['pool']['processed'],'cache_next':seen['pool']['cache_next']} if seen else None})
assert {x.name:sha(x) for x in inputs.iterdir()}==before
report={'passed':sum(x['passed'] for x in results),'failed':sum(not x['passed'] for x in results),'cases_per_mode':len(cases),'results':results,'commands':commands,'mainpool_header_sha256':sha(root/'file_io/include/city19_mainpool_archive.hpp'),'building_layout_sha256':sha(root/'file_io/include/city19_building5_layout.hpp'),'new_tool_sha256':sha(root/'file_io/tools/qualify_city19_mainpool.cpp'),'production_sources':ready['fixed_production_source_sha256'],'original_data_or_game_used':False,'original_ctor_padding_or_postload_hooks_executed':False,'total_budget_seconds':180}
(out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['passed','failed','cases_per_mode']}));raise SystemExit(bool(report['failed']))
