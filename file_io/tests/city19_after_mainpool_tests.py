#!/usr/bin/env python3
"""Self-authored Figure7 wire/cache/resource fixtures; no original game execution."""
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
prefix0=context+goal+preceding
assert len(prefix0)==55797
widths=[1,1,1,1,1,1,2,2,4,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,2,2,2,1,1,2,1,1,1,1,2,2,2,1,1,1,2,4,1,1,1,1,1,2,1,1,4,2,1,1,1,2,2,4,4,4,4,4,4,1,2,4,4,4,4,4,4,2,2,2,4,16]
assert len(widths)==83 and sum(widths)==180
building=struct.pack('<H',5)+b''.join(bytes([i+1])*n for i,n in enumerate(widths))
second=struct.pack('<H',5)+b''.join(bytes([i+101])*n for i,n in enumerate(widths))
pool=lambda count:struct.pack('<Hi',1,count)
bdecl=decl(b'Building')
prefix=prefix0+struct.pack('<Hi',1,1)+decl(b'Building')+building
assert len(prefix)==55999
figure=struct.pack('<H',7)+bytes((i*29+11)%256 for i in range(395))+b'\0'
figure2=struct.pack('<H',7)+bytes((i*7+17)%256 for i in range(395))+b'\0'
figure_decl=decl(b'Figure')
figure_pool=lambda n:struct.pack('<Hi',1,n)
cases=[]
def add(name,tail,valid=True,end=56415,items=1,cache=5,pending=False,extension=False,error=None,maxitems=8192,wrapped=False,sourceprefix=None):
    cases.append(dict(name=name,plain=(prefix if sourceprefix is None else sourceprefix)+tail,valid=valid,end=end,items=items,cache=cache,pending=pending,extension=extension,error=error,maxitems=maxitems,wrapped=wrapped))
add('Figure7-161-fields395-plus-presence',figure_pool(1)+figure_decl+figure)
add('zero-Figure-no-objects',figure_pool(0)+b'NEXT',end=56005,items=0,cache=3)
add('Figure-classref-freshobject',figure_pool(2)+figure_decl+figure+struct.pack('<H',32771)+figure2,end=56815,items=2,cache=6)
add('Figure-objectref-tag-only',figure_pool(2)+figure_decl+figure+struct.pack('<H',4)+b'POISON',end=56417,items=2,cache=5)
add('Figure-extended-classref',figure_pool(2)+figure_decl+figure+struct.pack('<HI',32767,2147483651)+figure2,end=56819,items=2,cache=6)
add('Figure-extended-objectref',figure_pool(2)+figure_decl+figure+struct.pack('<HI',32767,4),end=56421,items=2,cache=5)
add('duplicate-Figure-declarations',figure_pool(2)+(figure_decl+figure)*2,end=56825,items=2,cache=7)
add('Figure-unknown-first',figure_pool(4000)+decl(b'Other')+b'OPAQUE',end=56016,items=0,cache=3,pending=True)
add('Figure-unknown-after-known',figure_pool(2)+figure_decl+figure+decl(b'Other')+b'OPAQUE',end=56426,items=1,cache=5,pending=True)
add('Figure-item-checkpoint',figure_pool(2)+figure_decl+figure+b'OPAQUE',maxitems=1)
add('Figure-wrapped',figure_pool(1)+figure_decl+figure,wrapped=True)
for value in [1,2,255]:add('Figure-present-extension-'+str(value),figure_pool(1)+figure_decl+figure[:-1]+bytes([value])+b'UNKNOWN',end=56415,items=0,cache=5,extension=True)
add('Figure-opaque-activity-zero',figure_pool(1)+figure_decl+figure[:2]+b'\0'+figure[3:])
add('Figure-opaque-activity-ff',figure_pool(1)+figure_decl+figure[:2]+b'\xff'+figure[3:])
housing=mh(1)+decl(b'cHousingGoal')+struct.pack('<HIHIII',1,21,1,22,23,24)+mh(0)*9
priorgoal=context+housing+preceding+struct.pack('<Hi',1,1)+decl(b'Building')+building
assert len(priorgoal)==56037
add('Figure-shared-Goal-Building-cache',figure_pool(1)+figure_decl+figure,end=56453,cache=7,sourceprefix=priorgoal)
add('Figure-old-Goal-class-incompatible',figure_pool(1)+struct.pack('<H',32769),False,error='earlier Goal/Building class',sourceprefix=priorgoal)
add('Figure-old-Building-class-incompatible',figure_pool(1)+struct.pack('<H',32769),False,error='earlier Goal/Building class')
add('Figure-old-Building-object-incompatible',figure_pool(1)+struct.pack('<H',2),False,error='incompatible or incomplete cached object')
add('Figure-null-positive',figure_pool(1)+struct.pack('<H',0),False,error='Figure nonnull consumer')
add('Figure-class-kind-mismatch',figure_pool(2)+figure_decl+figure+struct.pack('<H',32772),False,error='class expected')
add('Figure-object-kind-mismatch',figure_pool(2)+figure_decl+figure+struct.pack('<H',3),False,error='object expected')
add('Figure-future-classref',figure_pool(1)+struct.pack('<H',32771),False,error='unqualified Figure class reference')
add('Figure-future-objectref',figure_pool(1)+struct.pack('<H',3),False,error='unqualified Figure object reference')
add('Figure-max-extended-tag',figure_pool(1)+struct.pack('<HI',32767,4294967295),False,error='unqualified Figure class reference')
for v in [0,2,65535]:add('Figure-list-version-'+str(v),struct.pack('<Hi',v,1)+figure_decl+figure,False,error='Figure list section')
for v in [0,1,2,6,8,65535]:add('Figure-field-version-'+str(v),figure_pool(1)+figure_decl+struct.pack('<H',v)+figure[2:],False,error='Figure field section')
add('Figure-schema1',figure_pool(1)+decl(b'Figure',1)+figure,False,error='schema mismatch')
add('Figure-count16384-unknown',figure_pool(16384)+decl(b'Other'),end=56016,items=0,cache=3,pending=True)
for v in [-1,-2147483648,16385]:add('Figure-count-reject-'+str(v),figure_pool(v),False,error='negative Figure count' if v<0 else 'Figure count exceeds modern budget')
for n in range(6):add('Figure-header-truncated-'+str(n),figure_pool(1)[:n],False,error='truncated')
for n in [0,1,2,3,4,5,6,11]:add('Figure-declaration-truncated-'+str(n),figure_pool(1)+figure_decl[:n],False,error='truncated')
for n in range(398):add('Figure-payload-truncated-'+str(n),figure_pool(1)+figure_decl+figure[:n],False,error='truncated')
add('input-sha',figure_pool(1)+figure_decl+figure,False,error='input SHA mismatch')
add('decoded-sha',figure_pool(1)+figure_decl+figure,False,error='decoded SHA mismatch')
add('bad-adler',figure_pool(1)+figure_decl+figure,False,error='unqualified block',wrapped=True)
many=figure_pool(2000)+figure_decl+figure+(struct.pack('<H',32771)+figure)*1999
add('modern2000-continuous-Figure-batch',many,end=len(prefix)+len(many),items=2000,cache=2004,wrapped=True)
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
    exe=out/('after-mainpool-'+mode);flags=['-O2'] if mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined']
    q=run(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-isysroot',sdk.stdout.strip(),'-I',root/'file_io/include',root/'file_io/src/strict_zlib.cpp',root/'file_io/src/framed_reader.cpp',root/'file_io/tools/qualify_city19_after_mainpool.cpp','-lz','-o',exe],mode+'-build',60)
    if q.returncode:raise RuntimeError('bounded mainpool build failed')
    for case,path,blocks in prepared:
        q=run([exe,path,path.stat().st_size,262144,4915584,blocks,4275878570,2882400255,2863386095,36144,36144,'0'*64 if case['name']=='input-sha' else sha(path),'0'*64 if case['name']=='decoded-sha' else hashlib.sha256(case['plain']).hexdigest(),4096,case['maxitems']],mode+'-'+case['name']);seen=json.loads(q.stdout) if q.stdout else None
        if case['valid']:
            seenpool=seen['afterpool'] if seen else None
            passed=q.returncode==0 and q.stderr=='' and seen is not None and seen['parsed_end']==case['end'] and seenpool['processed']==case['items'] and seenpool['cache_next']==case['cache'] and (seenpool['pending_class'] is not None)==case['pending'] and (seenpool['pending_extension'] is not None)==case['extension'] and seen['Building_activity_status_interpreted'] is False and seen['Figure_activity_status_interpreted'] is False and seen['counts_are_stream_records_only'] is True and seen['remaining_uninterpreted_decoded_bytes']==len(case['plain'])-case['end'] and seen['field_values_emitted'] is False
            if case['items']:
                passed=passed and seenpool['items'][0]['payload_bytes']==398
            if case['name'] in ('Figure-objectref-tag-only','Figure-extended-objectref'):passed=passed and seenpool['items'][-1]['object_reference'] and seenpool['items'][-1]['payload_bytes']==0
            if case['name']=='Figure-shared-Goal-Building-cache':passed=passed and seenpool['cache_start']==5 and seenpool['items'][0]['class_model_id']==5 and seenpool['items'][0]['object_model_id']==6
            if case['name']=='modern2000-continuous-Figure-batch':passed=passed and seenpool['figures_complete'] and seenpool['items'][-1]['object_model_id']==2003
            if case['extension']:passed=passed and seenpool['pending_extension']['full_object_payload_qualified'] is False
        else:passed=q.returncode==2 and not q.stdout and (not case['error'] or case['error'] in q.stderr)
        results.append({'mode':mode,'name':case['name'],'passed':passed,'observed':seen if case['items']<100 else {'parsed_end':seen['parsed_end'],'processed':seen['afterpool']['processed'],'cache_next':seen['afterpool']['cache_next']} if seen else None})
assert {x.name:sha(x) for x in inputs.iterdir()}==before
report={'passed':sum(x['passed'] for x in results),'failed':sum(not x['passed'] for x in results),'cases_per_mode':len(cases),'results':results,'commands':commands,'afterpool_header_sha256':sha(root/'file_io/include/city19_after_mainpool_archive.hpp'),'Figure_layout_sha256':sha(root/'file_io/include/city19_figure7_layout.hpp'),'new_tool_sha256':sha(root/'file_io/tools/qualify_city19_after_mainpool.cpp'),'production_sources':ready['fixed_production_source_sha256'],'original_data_or_game_used':False,'original_ctor_padding_or_postload_hooks_executed':False,'total_budget_seconds':180}
(out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['passed','failed','cases_per_mode']}));raise SystemExit(bool(report['failed']))
