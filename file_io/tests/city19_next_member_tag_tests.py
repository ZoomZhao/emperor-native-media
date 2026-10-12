#!/usr/bin/env python3
"""Next member/object tag and typed conditional cache: self-authored data only."""
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
    if sha(root/name)!=h:raise RuntimeError('reviewed source mismatch')
out.mkdir(parents=True);start=time.monotonic();commands=[];results=[]
def command(argv,label,timeout=10):
    left=180-(time.monotonic()-start)
    if left<=0:raise RuntimeError('180-second budget exhausted')
    q=subprocess.run([str(x) for x in argv],capture_output=True,text=True,timeout=min(timeout,left))
    (out/(label+'.stdout')).write_text(q.stdout);(out/(label+'.stderr')).write_text(q.stderr)
    commands.append({'argv':[str(x) for x in argv],'exit_code':q.returncode,'stdout_sha256':hashlib.sha256(q.stdout.encode()).hexdigest(),'stderr_sha256':hashlib.sha256(q.stderr.encode()).hexdigest()})
    return q

first=struct.pack('<HHBIII',19,6,1,11,12,13)+b'abcdefghijklmnop'+struct.pack('<III',14,15,16)
component=struct.pack('<HIII8II',2,21,22,23,*range(101,109),24)
fixed=bytes((i*17+5)%256 for i in range(3256))
base=first+component+struct.pack('<HH',3,4)+fixed
mh=lambda n:struct.pack('<Hi',1,n)
decl=lambda name,schema=0:struct.pack('<HHH',65535,schema,len(name))+name
housing=struct.pack('<HIHIII',1,101,1,102,103,104)
again=struct.pack('<HIHIII',1,201,1,202,203,204)
prefix=lambda n:base+mh(n)+decl(b'cHousingGoal')+housing
if len(prefix(1))!=3399:raise RuntimeError('independent source fixture size')
cases=[]
def add(name,tail,valid,kind=None,end=None,member=1,skipped=0,payload=0,nextcache=3,firstcount=1,error=None,compressed=False,parts=None):
    cases.append({'name':name,'plain':prefix(firstcount)+tail,'valid':valid,'kind':kind,'end':end,'member':member,'skipped':skipped,'payload':payload,'nextcache':nextcache,'error':error,'compressed':compressed,'parts':parts})
unknown=b'cSyntheticNext';declaration=decl(unknown)
add('next-member-new-class',mh(1)+declaration+b'UNQUALIFIED PAYLOAD',True,'new_class_declaration',3411+len(unknown))
add('next-member-object-reference',mh(1)+struct.pack('<H',2),True,'object_reference_no_payload',3407)
add('object-reference-ignores-unknown-payload-looking-tail',mh(1)+struct.pack('<H',2)+b'\x00'*40,True,'object_reference_no_payload',3407)
add('next-member-class-reference-reads-new-payload',mh(1)+struct.pack('<H',32769)+again,True,'class_reference_new_instance',3427,payload=20,nextcache=4)
add('same-member-more-than-two-reference',struct.pack('<H',2),True,'object_reference_no_payload',3401,member=0,firstcount=3)
add('same-member-object-reference',struct.pack('<H',2),True,'object_reference_no_payload',3401,member=0,firstcount=2)
add('same-member-class-reference',struct.pack('<H',32769)+again,True,'class_reference_new_instance',3421,member=0,payload=20,nextcache=4,firstcount=2)
add('same-member-new-class',declaration+b'UNKNOWN',True,'new_class_declaration',3405+len(unknown),member=0,firstcount=2)
add('skip-two-empty-members',mh(0)*2+mh(1)+declaration,True,'new_class_declaration',3423+len(unknown),member=3,skipped=2)
add('all-nine-following-empty',mh(0)*9+b'UNKNOWN NEXT ROOT',True,'no_item_all_members_empty',3453,member=10,skipped=9)
add('extended-object-reference',mh(1)+struct.pack('<HI',32767,2),True,'object_reference_no_payload',3411)
add('extended-class-reference',mh(1)+struct.pack('<HI',32767,2147483649)+again,True,'class_reference_new_instance',3431,payload=20,nextcache=4)
add('source-name63-at-last-member',mh(0)*8+mh(1)+decl(b'C'+b'a'*62)+b'TAIL',True,'new_class_declaration',3522,member=9,skipped=8)
add('wrapped-zlib-reference',mh(1)+struct.pack('<H',2),True,'object_reference_no_payload',3407,compressed=True)
whole=prefix(1)+mh(1)+declaration;cuts=[1,3,45,47,95,97,99,3355,3357,3361,3363,3365,3367,3379,3381,3385,3387,3391,3395,3399,3400,3401,3402,3403,3404,3405,3406,3407,3409,3411,len(whole)]
add('cross-block-next-header-and-declaration',mh(1)+declaration,True,'new_class_declaration',len(whole),parts=[whole[x:y] for x,y in zip([0]+cuts,cuts)])
for label,word,error in [('object-as-class',32770,'cache kind mismatch: class expected'),('class-as-object',1,'cache kind mismatch: object expected'),('unknown-object',3,'object reference has no qualified cache entry'),('unknown-class',32771,'class reference has no qualified cache entry'),('class-zero',32768,'class reference zero invalid'),('null-object',0,'positive member consumer requires nonnull')]:
    add(label,mh(1)+struct.pack('<H',word),False,error=error)
add('extended-internal-not-newclass',mh(1)+struct.pack('<HI',32767,2147516415)+declaration,False,error='class reference has no qualified cache entry')
add('negative-next-count',mh(-1),False,error='negative next member count')
add('wrong-next-version',struct.pack('<Hi',2,1)+declaration,False,error='next member version outside modern scope1')
add('class-reference-missing-payload',mh(1)+struct.pack('<H',32769),False,error='requires new Housing payload20')
add('class-reference-payload-last-byte-truncated',mh(1)+struct.pack('<H',32769)+again[:-1],False,error='requires new Housing payload20')
add('class-reference-base-version-unsupported',mh(1)+struct.pack('<H',32769)+struct.pack('<H',2)+again[2:],False,error='section versions outside modern scope1')
add('class-reference-derived-version-unsupported',mh(1)+struct.pack('<H',32769)+again[:6]+struct.pack('<H',2)+again[8:],False,error='section versions outside modern scope1')
for n in [0,1,2,3,4,5]:add('next-header-truncated-'+str(n),mh(1)[:n],False,error='next member header truncated')
add('next-tag-missing',mh(1),False,error='next object tag truncated')
add('next-tag-one-byte',mh(1)+b'\xff',False,error='next object tag truncated')
for n in [2,3,4,5]:add('extended-tag-truncated-'+str(n),mh(1)+struct.pack('<HI',32767,2)[:n],False,error='extended object tag truncated')
add('declaration-schema-missing',mh(1)+b'\xff\xff',False,error='schema/name length truncated')
add('declaration-name-last-byte-missing',mh(1)+declaration[:-1],False,error='name bytes truncated')
add('name64-source-rejected',mh(1)+decl(b'C'+b'a'*63),False,error='name length must be below64')
add('nameNUL-modern-scope',mh(1)+decl(b'C\x00X'),False,error='ASCII identifier scope')
add('input-sha',mh(1)+declaration,False)
add('decoded-sha',mh(1)+declaration,False)
add('bad-adler',mh(1)+declaration,False,compressed=True)
inputs=out/'self-authored-inputs';inputs.mkdir();prepared=[]
for case in cases:
    parts=case['parts'] or [case['plain']];data=struct.pack('<I',4275878570)
    for body in parts:
        if case['compressed']:
            encoded=zlib.compress(body)
            if case['name']=='bad-adler':encoded=encoded[:-1]+bytes([encoded[-1]^1])
            data+=struct.pack('<II',2882400255,len(encoded))+b'TEST'+encoded
        else:data+=struct.pack('<II',2863386095,len(body))+body
    path=inputs/(case['name']+'.bin');path.write_bytes(data);prepared.append((case,path,len(parts)))
before={x.name:sha(x) for x in inputs.iterdir()}
sdk=command(['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'],'sdk',30)
if sdk.returncode:raise RuntimeError('installed SDK unavailable')
for mode in ('release','ubsan'):
    exe=out/('next-member-tag-'+mode);flags=['-O2'] if mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined']
    q=command(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-isysroot',sdk.stdout.strip(),'-I',root/'file_io/include',root/'file_io/src/strict_zlib.cpp',root/'file_io/src/framed_reader.cpp',root/'file_io/tools/qualify_city19_next_member_tag.cpp','-lz','-o',exe],mode+'-build',60)
    if q.returncode:raise RuntimeError('self-authored next tag build failed')
    for case,path,blocks in prepared:
        q=command([exe,path,path.stat().st_size,262144,4915584,blocks,4275878570,2882400255,2863386095,36144,36144,'0'*64 if case['name']=='input-sha' else sha(path),'0'*64 if case['name']=='decoded-sha' else hashlib.sha256(case['plain']).hexdigest()],mode+'-'+case['name']);seen=json.loads(q.stdout) if q.stdout else None
        if case['valid']:
            passed=q.returncode==0 and seen is not None and seen['parsed_end']==case['end'] and seen['first_payload_frontier']==3399 and seen['next_tag_kind']==case['kind'] and seen['next_member_index']==case['member'] and seen['skipped_empty_members']==case['skipped'] and seen['next_payload_bytes_consumed']==case['payload'] and seen['conditional_cache_next_after_item']==case['nextcache'] and seen['cache_runtime_state_proved'] is False and seen['original_explicit_cache_kind_check_inferred'] is False and seen['mainpool_indices_interpreted'] is False and seen['object_field_values_emitted'] is False and seen['remaining_uninterpreted_decoded_bytes']==len(case['plain'])-case['end']
            expected_following='pending_class_payload' if case['kind']=='new_class_declaration' else 'next_root_consumer' if case['kind']=='no_item_all_members_empty' else 'next_object_tag_same_member' if case['name']=='same-member-more-than-two-reference' else 'next_member_header'
            passed=passed and seen['following_consumer_kind']==expected_following
            if case['payload']:passed=passed and seen['next_payload_sha256']==hashlib.sha256(again).hexdigest()
        else:passed=q.returncode==2 and not q.stdout and (not case['error'] or case['error'] in q.stderr)
        results.append({'mode':mode,'name':case['name'],'passed':passed,'observed':seen})
if {x.name:sha(x) for x in inputs.iterdir()}!=before:raise RuntimeError('self-authored input modified')
report={'passed':sum(x['passed'] for x in results),'failed':sum(not x['passed'] for x in results),'cases_per_mode':len(cases),'results':results,'commands':commands,'tool_source_sha256':sha(root/'file_io/tools/qualify_city19_next_member_tag.cpp'),'production_sources':ready['fixed_production_source_sha256'],'original_data_or_game_used':False,'total_budget_seconds':180}
(out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['passed','failed','cases_per_mode']}));raise SystemExit(bool(report['failed']))
