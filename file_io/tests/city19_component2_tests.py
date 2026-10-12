#!/usr/bin/env python3
"""Synthetic component2 layout, identity and truncation checks. Never opens GameData."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import time

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-root',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
if sys.flags.optimize:parser.error('optimized test invocation unsupported')
root=args.source_root.resolve();out=args.output.resolve()
if out.exists():parser.error('fresh output required')
ready=json.loads((root/'docs/original-profile-owned-readiness.json').read_text())
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for p,h in ready['fixed_production_source_sha256'].items():
    if sha(root/p)!=h:raise RuntimeError('reviewed source mismatch: '+p)
out.mkdir(parents=True);begin=time.monotonic();commands=[];results=[]
def command(argv,label,timeout=10):
    remaining=180-(time.monotonic()-begin)
    if remaining<=0:raise RuntimeError('180-second total test budget exhausted')
    result=subprocess.run([str(x) for x in argv],capture_output=True,text=True,timeout=min(timeout,remaining))
    (out/(label+'.stdout')).write_text(result.stdout);(out/(label+'.stderr')).write_text(result.stderr)
    commands.append({'argv':[str(x) for x in argv],'exit_code':result.returncode,
                     'stdout_sha256':hashlib.sha256(result.stdout.encode()).hexdigest(),
                     'stderr_sha256':hashlib.sha256(result.stderr.encode()).hexdigest()})
    return result
sdk=command(['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'],'sdk',30)
if sdk.returncode:raise RuntimeError('installed SDK unavailable')
opaque=b'abcdefghijklmnop'
payload=struct.pack('<HHBIII',19,6,1,0x01020304,0x05060708,0x090a0b0c)+opaque+struct.pack('<III',0x11121314,0x15161718,0x191a1b1c)
array32=struct.pack('<8I',*range(101,109))
payload+=struct.pack('<HIII',2,21,22,23)+array32+struct.pack('<I',24)
if len(payload)!=95:raise RuntimeError('independent source-layout fixture construction')
cases=[('unaligned-fixed-layout',[payload],True),
       ('versions-across-raw-blocks',[payload[:1],payload[1:3],payload[3:5],payload[5:17],payload[17:33],payload[33:45],payload[45:46],payload[46:47],payload[47:59],payload[59:91],payload[91:]],True),
       ('uninterpreted-tail',[payload+b'TAIL'],True),
       ('wrong-city19',[struct.pack('<H',18)+payload[2:]],False),
       ('wrong-section6',[payload[:2]+struct.pack('<H',5)+payload[4:]],False),
       ('wrong-component2',[payload[:45]+struct.pack('<H',1)+payload[47:]],False)]
for n in [44,45,46,47,51,55,59,91,94]:cases.append(('truncated-at-'+str(n),[payload[:n]],False))
cases += [('wrong-input-sha',[payload],False),('wrong-decoded-sha',[payload],False)]
inputs=out/'self-authored-inputs';inputs.mkdir()
for name,parts,_ in cases:
    (inputs/(name+'.bin')).write_bytes(struct.pack('<I',0xfedcbaaa)+b''.join(struct.pack('<II',0xaaabcdef,len(x))+x for x in parts))
before={p.name:sha(p) for p in inputs.iterdir()}
for mode in ('release','ubsan'):
    exe=out/('component2-'+mode)
    flags=['-O2'] if mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined']
    build=command(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-isysroot',sdk.stdout.strip(),
                   '-I',root/'file_io/include',root/'file_io/src/strict_zlib.cpp',root/'file_io/src/framed_reader.cpp',
                   root/'file_io/tools/qualify_city19_component2.cpp','-lz','-o',exe],mode+'-build',60)
    if build.returncode:raise RuntimeError('self-authored tool build failed')
    for name,parts,valid in cases:
        p=inputs/(name+'.bin');plain=b''.join(parts)
        run=command([exe,p,p.stat().st_size,262144,4915584,len(parts),0xfedcbaaa,0xabcdefff,0xaaabcdef,36144,36144,
                     '0'*64 if name=='wrong-input-sha' else sha(p),
                     '0'*64 if name=='wrong-decoded-sha' else hashlib.sha256(plain).hexdigest()],mode+'-'+name)
        observed=json.loads(run.stdout) if run.stdout else None
        if valid:
            passed=run.returncode==0 and observed['section_end']==45 and observed['component_end']==95 and observed['component_payload_sha256']==hashlib.sha256(payload[47:95]).hexdigest() and observed['component_array32_sha256']==hashlib.sha256(array32).hexdigest() and observed['section_payload_sha256']==hashlib.sha256(payload[4:45]).hexdigest() and observed['opaque16_sha256']==hashlib.sha256(opaque).hexdigest() and observed['remaining_uninterpreted_decoded_bytes']==len(plain)-95
        else:passed=run.returncode==2 and not run.stdout
        results.append({'mode':mode,'name':name,'passed':passed,'observed':observed,
                        'expectation_source':'independent compact firstsection plus component2 source requests4+4+4+32+4; version at45 andend95; no runtime-type inference'})
if {p.name:sha(p) for p in inputs.iterdir()}!=before:raise RuntimeError('synthetic input changed')
report={'passed':sum(x['passed'] for x in results),'failed':sum(not x['passed'] for x in results),'results':results,'commands':commands,
        'tool_source_sha256':sha(root/'file_io/tools/qualify_city19_component2.cpp'),'production_sources':ready['fixed_production_source_sha256'],
        'original_data_or_game_used':False,'raw_or_decoded_original_bytes_saved':False,'total_budget_seconds':180}
(out/'results.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:report[k] for k in ['passed','failed']}))
raise SystemExit(bool(report['failed']))
