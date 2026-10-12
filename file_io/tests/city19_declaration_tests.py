#!/usr/bin/env python3
"""Bounded archive tag and class-declaration tests using self-authored bytes only."""
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
for p,h in ready['fixed_production_source_sha256'].items():
    if sha(root/p)!=h:raise RuntimeError('reviewed source mismatch: '+p)
out.mkdir(parents=True);begin=time.monotonic();commands=[];results=[]
def command(argv,label,timeout=10):
    remaining=180-(time.monotonic()-begin)
    if remaining<=0:raise RuntimeError('180-second total test budget exhausted')
    run=subprocess.run([str(x) for x in argv],capture_output=True,text=True,timeout=min(timeout,remaining))
    (out/(label+'.stdout')).write_text(run.stdout);(out/(label+'.stderr')).write_text(run.stderr)
    commands.append({'argv':[str(x) for x in argv],'exit_code':run.returncode,
                     'stdout_sha256':hashlib.sha256(run.stdout.encode()).hexdigest(),
                     'stderr_sha256':hashlib.sha256(run.stderr.encode()).hexdigest()})
    return run

first=struct.pack('<HHBIII',19,6,1,11,12,13)+b'abcdefghijklmnop'+struct.pack('<III',14,15,16)
second=struct.pack('<HIII8II',2,21,22,23,*range(101,109),24)
fixed=bytes((i*17+5)%256 for i in range(3256))
base=first+second+struct.pack('<HH',3,4)+fixed+struct.pack('<Hi',1,3)
if len(base)!=3361:raise RuntimeError('independent source prelude fixture length')
decl=lambda name,schema=0:struct.pack('<HHH',65535,schema,len(name))+name
name=b'cSyntheticGoal';body=b'UNQUALIFIED PAYLOAD MUST NOT BE READ AS FIELDS'
good=decl(name)+body
cases=[]
def add(label,tail,valid,kind=None,schema=None,identifier=None,compressed=False,error=None,chunks=None):
    cases.append({'name':label,'plain':base+tail,'valid':valid,'kind':kind,'schema':schema,'identifier':identifier,
                  'compressed':compressed,'error':error,'chunks':chunks})
add('new-class-declaration',good,True,'new_class',0,name)
add('new-class-u16-schema',decl(name,65535)+body,True,'new_class',65535,name)
add('source-name63',decl(b'C'+b'a'*62)+body,True,'new_class',0,b'C'+b'a'*62)
add('source-name1',decl(b'C')+body,True,'new_class',0,b'C')
add('unknown-static-class-remains-unresolved',decl(b'cUnknownType')+body,True,'new_class',0,b'cUnknownType')
add('null-compact',b'\x00\x00'+body,True,'null_object_reference')
add('null-extended',struct.pack('<HI',32767,0)+body,True,'null_object_reference')
add('wrapped-zlib-declaration',good,True,'new_class',0,name,compressed=True)
cuts=[1,3,45,47,95,97,99,3355,3357,3361,3362,3363,3364,3365,3366,3367,3367+len(name)]
plain=base+good;cuts+=[len(plain)]
add('across-block-tag-schema-length-name',good,True,'new_class',0,name,
    chunks=[plain[a:b] for a,b in zip([0]+cuts,cuts)])
add('class-reference-no-cache',struct.pack('<H',32769)+body,False,error='class reference has no qualified cache entry')
add('class-reference-zero',struct.pack('<H',32768),False,error='class reference index zero invalid')
add('object-reference-no-cache',struct.pack('<H',1),False,error='object reference has no qualified cache entry')
add('extended-object-reference',struct.pack('<HI',32767,1),False,error='object reference has no qualified cache entry')
add('extended-class-reference',struct.pack('<HI',32767,2147483649),False,error='class reference has no qualified cache entry')
add('extended-value-not-newclass',struct.pack('<HI',32767,2147516415)+decl(name),False,error='class reference has no qualified cache entry')
add('source-name64',decl(b'C'+b'a'*63),False,error='source class name length must be below64')
add('empty-name-modern-scope',decl(b''),False,error='empty class name outside qualification scope')
add('name-NUL-modern-scope',decl(b'C\x00X'),False,error='ASCII identifier scope')
add('name-digit-first-modern-scope',decl(b'1Class'),False,error='ASCII identifier scope')
add('name-nonASCII-modern-scope',decl(b'C\xff'),False,error='ASCII identifier scope')
add('name-shortread',struct.pack('<HHH',65535,0,10)+b'C12345678',False,error='class name bytes truncated')
for n in range(0,7):add('declaration-truncated-'+str(n),decl(name)[:n],False)
for n in range(2,6):add('extended-truncated-'+str(n),struct.pack('<HI',32767,0)[:n],False,error='extended object tag truncated')
add('name-last-byte-truncated',decl(name)[:-1],False,error='class name bytes truncated')
add('input-sha',good,False)
add('decoded-sha',good,False)
add('bad-adler',good,False,compressed=True)
inputs=out/'self-authored-inputs';inputs.mkdir();manifest=[]
for case in cases:
    chunks=case['chunks'] or [case['plain']]
    framed=struct.pack('<I',4275878570)
    for chunk in chunks:
        if case['compressed']:
            encoded=zlib.compress(chunk)
            if case['name']=='bad-adler':encoded=encoded[:-1]+bytes([encoded[-1]^1])
            framed+=struct.pack('<II',2882400255,len(encoded))+b'TEST'+encoded
        else:framed+=struct.pack('<II',2863386095,len(chunk))+chunk
    p=inputs/(case['name']+'.bin');p.write_bytes(framed);manifest.append((case,p,len(chunks)))
before={p.name:sha(p) for p in inputs.iterdir()}
sdk=command(['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'],'sdk',30)
if sdk.returncode:raise RuntimeError('installed SDK unavailable')
for mode in ('release','ubsan'):
    exe=out/('first-declaration-'+mode)
    flags=['-O2'] if mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined']
    build=command(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-isysroot',sdk.stdout.strip(),
                   '-I',root/'file_io/include',root/'file_io/src/strict_zlib.cpp',root/'file_io/src/framed_reader.cpp',
                   root/'file_io/tools/qualify_city19_first_declaration.cpp','-lz','-o',exe],mode+'-build',60)
    if build.returncode:raise RuntimeError('self-authored qualifier build failed')
    for case,p,count in manifest:
        run=command([exe,p,p.stat().st_size,262144,4915584,count,4275878570,2882400255,2863386095,36144,36144,
                     '0'*64 if case['name']=='input-sha' else sha(p),
                     '0'*64 if case['name']=='decoded-sha' else hashlib.sha256(case['plain']).hexdigest()],mode+'-'+case['name'])
        observed=json.loads(run.stdout) if run.stdout else None
        if case['valid']:
            length=len(case['identifier']) if case['identifier'] is not None else None
            end=3361+6+length if length is not None else 3361+(6 if case['name']=='null-extended' else 2)
            passed=run.returncode==0 and observed is not None and observed['parsed_end']==end and observed['member_header_frontier']==3361 and observed['archive_tag_kind']==case['kind'] and observed['declared_schema']==case['schema'] and observed['declared_class_name_bytes']==length and observed['declared_class_name_sha256']==(hashlib.sha256(case['identifier']).hexdigest() if length is not None else None) and observed['class_name_emitted'] is False and observed['actual_descriptor_resolved'] is False and observed['expected_descriptor_used_as_actual'] is False and observed['class_factory_or_schema_reader_executed'] is False and observed['cache_runtime_state_proved'] is False and observed['remaining_uninterpreted_decoded_bytes']==len(case['plain'])-end
        else:
            passed=run.returncode==2 and not run.stdout and (not case['error'] or case['error'] in run.stderr)
        results.append({'mode':mode,'name':case['name'],'passed':passed,'observed':observed})
if {p.name:sha(p) for p in inputs.iterdir()}!=before:raise RuntimeError('self-authored input changed')
report={'passed':sum(x['passed'] for x in results),'failed':sum(not x['passed'] for x in results),'cases_per_mode':len(cases),
        'results':results,'commands':commands,'tool_source_sha256':sha(root/'file_io/tools/qualify_city19_first_declaration.cpp'),
        'production_sources':ready['fixed_production_source_sha256'],'original_data_or_game_used':False,'total_budget_seconds':180}
(out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['passed','failed','cases_per_mode']}))
raise SystemExit(bool(report['failed']))
