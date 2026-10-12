#!/usr/bin/env python3
"""Synthetic cHousingGoal payload widths/versions and following-consumer classification."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import time
import zlib

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-root',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
if sys.flags.optimize:parser.error('optimized test invocation unsupported')
root=args.source_root.resolve();out=args.output.resolve()
if out.exists():parser.error('fresh output required')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
ready=json.loads((root/'docs/original-profile-owned-readiness.json').read_text())
for name,h in ready['fixed_production_source_sha256'].items():
    if sha(root/name)!=h:raise RuntimeError('reviewed production source mismatch')
out.mkdir(parents=True);started=time.monotonic();commands=[];results=[]
def command(argv,label,timeout=10):
    remaining=180-(time.monotonic()-started)
    if remaining<=0:raise RuntimeError('180-second total budget exhausted')
    q=subprocess.run([str(x) for x in argv],capture_output=True,text=True,timeout=min(timeout,remaining))
    (out/(label+'.stdout')).write_text(q.stdout);(out/(label+'.stderr')).write_text(q.stderr)
    commands.append({'argv':[str(x) for x in argv],'exit_code':q.returncode,
                     'stdout_sha256':hashlib.sha256(q.stdout.encode()).hexdigest(),
                     'stderr_sha256':hashlib.sha256(q.stderr.encode()).hexdigest()})
    return q

# Spelled-out compact widths are independent of the qualifier.
first=struct.pack('<HHBIII',19,6,1,11,12,13)+b'abcdefghijklmnop'+struct.pack('<III',14,15,16)
second=struct.pack('<HIII8II',2,21,22,23,*range(101,109),24)
fixed=bytes((i*17+5)%256 for i in range(3256))
context=first+second+struct.pack('<HH',3,4)+fixed
name=b'cHousingGoal'
declaration=struct.pack('<HHH',65535,0,len(name))+name
payload=struct.pack('<HIHIII',1,0x01020304,1,0x11223344,0x55667788,0x99aabbcc)
if len(context)!=3355 or len(declaration)!=18 or len(payload)!=20:raise RuntimeError('independent source widths')
plain=context+struct.pack('<Hi',1,1)+declaration+payload
if len(plain)!=3399:raise RuntimeError('independent source fixture end')
def count_fixture(n):return context+struct.pack('<Hi',1,n)+declaration+payload
cases=[('one-object-next-member',plain,True,True,False,None),
       ('two-objects-next-object',count_fixture(2),True,False,False,None),
       ('huge-count-no-object-allocation',count_fixture(2147483647),True,False,False,None),
       ('unknown-tail-uninterpreted',plain+b'NO NEXT CONSUMER QUALIFIED',True,True,False,None),
       ('wrapped-zlib-payload',plain,True,True,True,None)]
cuts=[1,3,45,47,95,97,99,3355,3357,3361,3363,3365,3367,3379,3380,3381,3384,3385,3386,3387,3391,3395,3399]
cases.append(('cross-block-version-and-fields',plain,True,True,False,[plain[a:b] for a,b in zip([0]+cuts,cuts)]))
for n in [3378,3379,3380,3381,3384,3385,3386,3387,3390,3391,3394,3395,3398]:
    cases.append(('truncated-'+str(n),plain[:n],False,True,False,None))
for label,offset,value in [('base-version',3379,2),('housing-version',3385,2),('class-schema',3363,1)]:
    cases.append(('wrong-'+label,plain[:offset]+struct.pack('<H',value)+plain[offset+2:],False,True,False,None))
wrongname=b'cUnknownGoal'
cases.append(('wrong-class-name',context+struct.pack('<Hi',1,1)+struct.pack('<HHH',65535,0,len(wrongname))+wrongname+payload,False,True,False,None))
cases += [('negative-member-count',count_fixture(-1),False,True,False,None),
          ('zero-member-count',count_fixture(0),False,True,False,None),
          ('input-sha',plain,False,True,False,None),('decoded-sha',plain,False,True,False,None),
          ('bad-adler',plain,False,True,True,None),('output-budget',plain,False,True,False,None)]
inputs=out/'self-authored-inputs';inputs.mkdir();prepared=[]
for label,body,valid,complete,compressed,chunks in cases:
    chunks=chunks or [body];frame=struct.pack('<I',4275878570)
    for chunk in chunks:
        if compressed:
            encoded=zlib.compress(chunk)
            if label=='bad-adler':encoded=encoded[:-1]+bytes([encoded[-1]^1])
            frame+=struct.pack('<II',2882400255,len(encoded))+b'TEST'+encoded
        else:frame+=struct.pack('<II',2863386095,len(chunk))+chunk
    path=inputs/(label+'.bin');path.write_bytes(frame);prepared.append((label,path,body,valid,complete,len(chunks)))
before={p.name:sha(p) for p in inputs.iterdir()}
sdk=command(['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'],'sdk',30)
if sdk.returncode:raise RuntimeError('installed SDK unavailable')
for mode in ('release','ubsan'):
    exe=out/('housing-payload-'+mode);flags=['-O2'] if mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined']
    q=command(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-isysroot',sdk.stdout.strip(),'-I',root/'file_io/include',
               root/'file_io/src/strict_zlib.cpp',root/'file_io/src/framed_reader.cpp',root/'file_io/tools/qualify_city19_housing_payload.cpp','-lz','-o',exe],mode+'-build',60)
    if q.returncode:raise RuntimeError('self-authored payload qualifier build failed')
    for label,path,body,valid,complete,blocks in prepared:
        q=command([exe,path,path.stat().st_size,262144,3398 if label=='output-budget' else 4915584,blocks,
                   4275878570,2882400255,2863386095,36144,36144,'0'*64 if label=='input-sha' else sha(path),
                   '0'*64 if label=='decoded-sha' else hashlib.sha256(body).hexdigest()],mode+'-'+label)
        seen=json.loads(q.stdout) if q.stdout else None
        if valid:
            passed=q.returncode==0 and seen is not None and seen['declaration_end']==3379 and seen['parsed_end']==3399 and seen['object_payload_bytes']==20 and seen['object_fixed_field_requests']==4 and seen['object_embedded_version_bytes']==4 and seen['object_payload_sha256']==hashlib.sha256(payload).hexdigest() and seen['base_field32_sha256']==hashlib.sha256(payload[2:6]).hexdigest() and seen['housing_fields12_sha256']==hashlib.sha256(payload[8:20]).hexdigest() and seen['first_member_complete_after_first_object']==complete and seen['next_consumer_kind']==('next_member_header' if complete else 'next_object_tag_same_member') and seen['original_class_factory_or_schema_reader_executed'] is False and seen['modern_fixed_payload_parser_executed'] is True and seen['cache_runtime_state_proved'] is False and seen['actual_descriptor_resolved'] is False and seen['object_field_values_emitted'] is False and seen['remaining_uninterpreted_decoded_bytes']==len(body)-3399
        else:passed=q.returncode==2 and not q.stdout
        results.append({'mode':mode,'name':label,'passed':passed,'observed':seen})
if {p.name:sha(p) for p in inputs.iterdir()}!=before:raise RuntimeError('synthetic inputs changed')
report={'passed':sum(x['passed'] for x in results),'failed':sum(not x['passed'] for x in results),'cases_per_mode':len(cases),'results':results,'commands':commands,
        'tool_source_sha256':sha(root/'file_io/tools/qualify_city19_housing_payload.cpp'),'production_sources':ready['fixed_production_source_sha256'],
        'original_data_or_game_used':False,'total_budget_seconds':180}
(out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['passed','failed','cases_per_mode']}));raise SystemExit(bool(report['failed']))
