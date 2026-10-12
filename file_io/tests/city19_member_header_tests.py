#!/usr/bin/env python3
"""Source-bound fixed fields and empty/positive member-header frontier. Self-authored data only."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import time
import zlib

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-root', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
if sys.flags.optimize:
    parser.error('optimized test invocation unsupported')
root = args.source_root.resolve()
out = args.output.resolve()
if out.exists():
    parser.error('fresh output required')
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
ready = json.loads((root/'docs/original-profile-owned-readiness.json').read_text())
for name, expected in ready['fixed_production_source_sha256'].items():
    if sha(root/name) != expected:
        raise RuntimeError('reviewed production source mismatch: '+name)
out.mkdir(parents=True)
started = time.monotonic()
commands = []
results = []

def command(argv, label, timeout=10):
    remaining = 180-(time.monotonic()-started)
    if remaining <= 0:
        raise RuntimeError('180-second total test budget exhausted')
    result = subprocess.run([str(x) for x in argv], capture_output=True, text=True,
                            timeout=min(timeout, remaining))
    (out/(label+'.stdout')).write_text(result.stdout)
    (out/(label+'.stderr')).write_text(result.stderr)
    commands.append({'argv':[str(x) for x in argv], 'exit_code':result.returncode,
                     'stdout_sha256':hashlib.sha256(result.stdout.encode()).hexdigest(),
                     'stderr_sha256':hashlib.sha256(result.stderr.encode()).hexdigest()})
    return result

# Independently spell out the source widths; never import parsing implementation.
opaque = b'abcdefghijklmnop'
first = struct.pack('<HHBIII',19,6,1,11,12,13)+opaque+struct.pack('<III',14,15,16)
second = struct.pack('<HIII8II',2,21,22,23,*range(101,109),24)
fixed = bytes((i*17+5)%256 for i in range(1760+88+88+1320))
base = first+second+struct.pack('<HH',3,4)+fixed
if len(first)!=45 or len(second)!=50 or len(base)!=3355:
    raise RuntimeError('independent source fixture sizes')
header = lambda n: struct.pack('<Hi',1,n)
positive = base+header(3)+b'UNQUALIFIED OBJECT BYTES'
empty = base+header(0)*10+b'NEXT OWNER UNKNOWN'
after_two = base+header(0)*2+header(1)+b'UNQUALIFIED'
splits = [1,3,45,46,47,59,91,95,96,97,98,99,1859,1947,2035,3355,3356,3357,3358,3359,3360,3361,len(positive)]
parts = [positive[a:b] for a,b in zip([0]+splits,splits)]
cases = [
    ('positive-first',positive,True,0,True,False,[positive]),
    ('all-ten-empty',empty,True,10,False,False,[empty]),
    ('two-empty-then-positive',after_two,True,2,True,False,[after_two]),
    ('cross-block-tags-count-and-fixed-fields',positive,True,0,True,False,parts),
    ('wrapped-zlib-positive',positive,True,0,True,True,[positive]),
    ('largest-positive-no-allocation',base+header(2147483647)+b'X',True,0,True,False,None),
    ('positive-payload-looks-like-wrong-next-version',base+header(1)+b'\xff\xff\xff\xff\xff\xff',True,0,True,False,None),
    ('negative-count-qualification-unsupported',base+header(-1),False,0,False,False,None),
    ('wrong-input-sha',positive,False,0,True,False,None),
    ('wrong-decoded-sha',positive,False,0,True,False,None),
    ('input-budget',positive,False,0,True,False,None),
    ('output-budget',positive,False,0,True,False,None),
    ('unframed-tail',positive,False,0,True,False,None),
    ('bad-adler',positive,False,0,True,True,None),
]
for name,offset,value in [('city19',0,18),('section6',2,5),('component2',45,1),
                          ('helper3',95,2),('owner4',97,3),('member1',3355,2)]:
    changed = positive[:offset]+struct.pack('<H',value)+positive[offset+2:]
    cases.append(('wrong-'+name,changed,False,0,True,False,None))
for n in [95,96,97,98,99,1858,1859,1946,1947,2034,2035,3354,3355,3356,3357,3358,3359,3360]:
    cases.append(('truncated-at-'+str(n),positive[:n],False,0,True,False,None))
cases += [('second-empty-header-truncated',empty[:3366],False,0,False,False,None),
          ('tenth-empty-header-truncated',empty[:3414],False,0,False,False,None)]
inputs = out/'self-authored-inputs'
inputs.mkdir()
manifest = []
for name,plain,valid,zeros,nonempty,compressed,chunks in cases:
    chunks = chunks or [plain]
    frame = struct.pack('<I',0xfedcbaaa)
    for chunk in chunks:
        if compressed:
            encoded = zlib.compress(chunk)
            if name=='bad-adler':
                encoded = encoded[:-1]+bytes([encoded[-1]^1])
            frame += struct.pack('<II',0xabcdefff,len(encoded))+b'TEST'+encoded
        else:
            frame += struct.pack('<II',0xaaabcdef,len(chunk))+chunk
    if name=='unframed-tail':
        frame += b'TAIL'
    path = inputs/(name+'.bin')
    path.write_bytes(frame)
    manifest.append((name,path,plain,valid,zeros,nonempty,len(chunks)))
before = {p.name:sha(p) for p in inputs.iterdir()}
sdk = command(['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'],'sdk',30)
if sdk.returncode:
    raise RuntimeError('installed SDK unavailable')
for mode in ['release','ubsan']:
    exe = out/('member-headers-'+mode)
    flags = ['-O2'] if mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined']
    build = command(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,
                     '-isysroot',sdk.stdout.strip(),'-I',root/'file_io/include',
                     root/'file_io/src/strict_zlib.cpp',root/'file_io/src/framed_reader.cpp',
                     root/'file_io/tools/qualify_city19_member_headers.cpp','-lz','-o',exe],mode+'-build',60)
    if build.returncode:
        raise RuntimeError('self-authored qualifier build failed')
    for name,path,plain,valid,zeros,nonempty,count in manifest:
        run = command([exe,path,path.stat().st_size,
                       path.stat().st_size-1 if name=='input-budget' else 262144,
                       3359 if name=='output-budget' else 4915584,count,
                       0xfedcbaaa,0xabcdefff,0xaaabcdef,36144,36144,
                       '0'*64 if name=='wrong-input-sha' else sha(path),
                       '0'*64 if name=='wrong-decoded-sha' else hashlib.sha256(plain).hexdigest()],mode+'-'+name)
        seen = json.loads(run.stdout) if run.stdout else None
        expected_end = 3355+6*(zeros+int(nonempty))
        if valid:
            allowed = seen is not None and run.returncode==0
            passed = allowed and seen['parsed_end']==expected_end and seen['completed_empty_members']==zeros and seen['positive_member_count_observed']==nonempty and seen['member_group_complete_without_objects']==(not nonempty) and seen['fixed_owner_payload_sha256']==hashlib.sha256(fixed).hexdigest() and seen['remaining_uninterpreted_decoded_bytes']==len(plain)-expected_end and seen['retained_prefix_bytes']<=3415 and seen['objects_deserialized'] is False and seen['member_count_or_object_values_emitted'] is False and seen['live_original_receiver_type_proved'] is False
        else:
            passed = run.returncode==2 and not run.stdout
        results.append({'mode':mode,'name':name,'passed':passed,'observed':seen})
if {p.name:sha(p) for p in inputs.iterdir()}!=before:
    raise RuntimeError('self-authored input changed')
report = {'passed':sum(x['passed'] for x in results),'failed':sum(not x['passed'] for x in results),
          'results':results,'commands':commands,'tool_source_sha256':sha(root/'file_io/tools/qualify_city19_member_headers.cpp'),
          'production_sources':ready['fixed_production_source_sha256'],'original_data_or_game_used':False,
          'total_budget_seconds':180,'raw_or_decoded_original_bytes_saved':False}
(out/'results.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({'passed':report['passed'],'failed':report['failed'],'per_mode':len(cases)}))
raise SystemExit(bool(report['failed']))
