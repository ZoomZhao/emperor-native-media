#!/usr/bin/env python3
"""Bounded self-authored synthetic tests. Never uses GameData or caller file inputs."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import time

HEADER, COMPRESSED, RAW = 0xfedcbaaa, 0xabcdefff, 0xaaabcdef
def word(n): return struct.pack('<I', n)
def digest(b): return hashlib.sha256(b).hexdigest()
def stored(data):
    # Independent RFC1950/RFC1951 fixture: one final stored block + explicit Adler32.
    a, b = 1, 0
    for c in data: a = (a+c)%65521; b = (b+a)%65521
    n = len(data)
    return b'\x78\x01\x01' + struct.pack('<HH',n,n^0xffff) + data + struct.pack('>I',(b<<16)|a)
def frame(data, compressed=False, tail=b''):
    payload = stored(data)+tail if compressed else data
    return word(COMPRESSED if compressed else RAW) + word(len(payload)) + (word(0x12345678) if compressed else b'') + payload

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-root', type=Path, required=True, help='repository root containing file_io and docs')
parser.add_argument('--output', type=Path, required=True, help='fresh directory; synthetic inputs written only here')
args = parser.parse_args()
if sys.flags.optimize: parser.error('test runner requires assertions enabled')
args.source_root, args.output = args.source_root.resolve(), args.output.resolve()
if args.output.exists(): parser.error('fresh output required')
args.output.mkdir(parents=True)
begin = time.monotonic()
commands, checks = [], []
MAX_SECONDS = 240

def command(argv, label, timeout=30, env=None):
    if time.monotonic()-begin > MAX_SECONDS: raise RuntimeError('240-second total test budget exhausted')
    start=time.monotonic()
    r=subprocess.run([str(a) for a in argv],capture_output=True,text=True,timeout=min(timeout,MAX_SECONDS-(start-begin)),env=env)
    stdout, stderr = args.output/(label+'.stdout'), args.output/(label+'.stderr')
    stdout.write_text(r.stdout); stderr.write_text(r.stderr)
    commands.append(dict(argv=[str(a) for a in argv],exit_code=r.returncode,elapsed_seconds=time.monotonic()-start,
                         stdout=stdout.name,stdout_sha256=digest(r.stdout.encode()),stderr=stderr.name,stderr_sha256=digest(r.stderr.encode()),
                         PYTHONOPTIMIZE=(env or os.environ).get('PYTHONOPTIMIZE')))
    return r
def check(name, passed, source, observed=None):
    checks.append(dict(name=name,passed=bool(passed),expectation_source=source,observed=observed))

sdk=command(['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'],'sdk')
if sdk.returncode: raise RuntimeError('installed SDK lookup failed; stop')
source=args.source_root/'file_io'
payload=b'Independent stored-block fixture\x00\xff'; prefix=b'prefix\x00'
multi=word(HEADER)+frame(prefix)+frame(b'')+frame(payload,True)+frame(b'',True)
cases={
    'success-mixed-empty': (multi,1048576,64),
    'success-exact-output-cap': (word(HEADER)+frame(payload,True),len(payload),1),
    'compressed-unused-tail': (word(HEADER)+frame(payload,True,b'TAIL'),1048576,64),
    'bad-Adler-private-prefix': (word(HEADER)+frame(prefix)+frame(payload,True)[:-1]+bytes([stored(payload)[-1]^1]),1048576,64),
    'compressed-output-cap': (word(HEADER)+frame(payload,True),10,64),
    'raw-output-cap': (word(HEADER)+frame(payload),10,64),
    'unknown-marker-metadata': (word(HEADER)+frame(prefix)+b'????HIDE-not-a-frame',1048576,64),
    'unsupported-header-metadata': (b'????HIDE',1048576,64),
    'block-limit-metadata': (word(HEADER)+frame(prefix),1048576,0),
    'truncated-length-metadata': (word(HEADER)+word(RAW)+b'\x03\x00',1048576,64),
    'truncated-payload': (word(HEADER)+word(RAW)+word(20)+b'abc',1048576,64),
    'encoded-cap-before-payload': (word(HEADER)+word(COMPRESSED)+word(36145)+word(0),1048576,64),
    'modern-empty-container': (word(HEADER),0,0),
    'zero-decoded-budget': (word(HEADER)+frame(payload,True),0,64),
    'empty-compressed-zero-budget': (word(HEADER)+frame(b'',True),0,1),
}
inputs=args.output/'synthetic-inputs';inputs.mkdir()
for name,(data,_,_) in cases.items(): (inputs/(name+'.bin')).write_bytes(data)
baseline={name:digest(data) for name,(data,_,_) in cases.items()}
for mode in ('release','ubsan'):
    exe=args.output/('qualifier-'+mode)
    flags=['-O2'] if mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined']
    argv=['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-isysroot',sdk.stdout.strip(),
          '-I',source/'include',source/'src/strict_zlib.cpp',source/'src/framed_reader.cpp',source/'tools/qualify_data_file.cpp','-lz','-o',exe]
    build=command(argv,'build-'+mode,60)
    if build.returncode: raise RuntimeError('self-authored qualifier build failed; stop')
    stats={}
    for name,(data,budget,blocks) in cases.items():
        p=inputs/(name+'.bin')
        run=command([exe,p,len(data),8192,budget,blocks,HEADER,COMPRESSED,RAW,36144,36144],mode+'-'+name)
        if run.returncode: raise RuntimeError('synthetic native run failed: '+name)
        s=json.loads(run.stdout); stats[name]=s
        (args.output/(mode+'-'+name+'.json')).write_text(json.dumps(s,indent=2)+'\n')
    check(mode+' synthetic full-input SHA and read-only identity',all(
        s['input_bytes']==len(cases[name][0]) and s['input_sha256']==baseline[name] and
        s['read_only'] and s['input_metadata_unchanged'] and not s['raw_or_decoded_bytes_exported'] and
        not s['whole_original_file_or_game_accepted'] and s['canonical_path']==str(inputs/(name+'.bin'))
        for name,s in stats.items()),'known synthetic bytes + Python hashlib; before/after input SHA compared below')
    s=stats['success-mixed-empty']
    expected=prefix+payload
    check(mode+' mixed raw/compressed/empty hash chain',s['termination']=='end' and s['validated_blocks']==4 and
          s['validated_output_bytes']==len(expected) and s['validated_output_prefix_sha256']==digest(expected) and
          [e['output_sha256'] for e in s['events'][:-1]]==[digest(b) for b in (prefix,b'',payload,b'')] and
          s['unconsumed_file_bytes']==0 and s['unconsumed_file_sha256']==digest(b''),
          'known raw bytes + independently constructed final stored zlib blocks + SHA256')
    s=stats['success-exact-output-cap']; e=s['events'][0]
    check(mode+' exact output cap accepts terminal block',s['termination']=='end' and s['validated_blocks']==1 and
          e['terminal_and_Adler_validated'] and e['codec_consumed']==len(stored(payload)) and
          e['codec_private_produced']==e['codec_published']==len(payload) and e['codec_unused_payload']==0,
          'strict_zlib cap includes one-byte private probe; valid terminal output equals cap')
    s=stats['compressed-unused-tail'];e=s['events'][0]
    check(mode+' unused codec tail distinct from framing EOF',s['termination']=='end' and e['codec_unused_payload']==4 and
          e['codec_consumed']==len(stored(payload)) and e['declared_payload_N']==len(stored(payload))+4 and
          s['unconsumed_file_bytes']==0,'existing strict_zlib first-member tail reporting; envelope consumes declared N')
    s=stats['bad-Adler-private-prefix'];e=s['events'][-1]
    check(mode+' checksum failure preserves prior published prefix',s['termination']=='codec_error' and
          s['validated_blocks']==1 and s['validated_output_bytes']==len(prefix) and
          s['validated_output_prefix_sha256']==digest(prefix) and e['codec_private_produced']==len(payload) and
          e['codec_published']==e['published_output_bytes']==0 and e['output_sha256']=='' and
          not e['terminal_and_Adler_validated'],'RFC1950 manual Adler corruption + existing transactional publication contract')
    for name,private in (('compressed-output-cap',11),('zero-decoded-budget',1)):
        s=stats[name];e=s['events'][-1]
        check(mode+' '+name,s['termination']=='output_limit' and s['validated_output_bytes']==0 and
              s['validated_output_prefix_sha256']==digest(b'') and e['codec_private_produced']==private and
              e['codec_published']==e['published_output_bytes']==0 and e['output_sha256']=='',
              'existing one-byte private overflow probe and zero publication on limit')
    s=stats['raw-output-cap'];e=s['events'][-1]
    check(mode+' raw cap rejects before reading payload',s['termination']=='output_limit' and e['input_offset']==12 and
          e['published_output_bytes']==0 and s['unconsumed_file_bytes']==len(payload),
          'framed_reader raw cap check precedes allocation/read')
    # These four checks are deliberate regressions: absent/unparsed N must not be reported as a declared length.
    for name,status in (('unknown-marker-metadata','unknown_marker'),('unsupported-header-metadata','unsupported_header'),
                        ('block-limit-metadata','block_limit'),('truncated-length-metadata','truncated')):
        s=stats[name];e=s['events'][-1]
        check(mode+' '+name,s['termination']==status and e.get('declared_payload_N') is None,
              'framed_reader did not consume a recognized complete length; hash/count-only diagnostics must not reinterpret unparsed bytes',
              dict(status=s['termination'],frame_start=e['frame_start'],input_offset=e['input_offset'],declared_payload_N=e.get('declared_payload_N')))
    s=stats['unknown-marker-metadata'];e=s['events'][-1]
    check(mode+' unknown marker retains unparsed-tail hash',s['unconsumed_file_sha256']==digest(b'HIDE-not-a-frame') and
          e['input_offset']==e['frame_start']+4 and s['validated_output_prefix_sha256']==digest(prefix),
          'unknown marker consumes only four-byte marker; remainder is an opaque hash-only tail')
    s=stats['truncated-payload'];e=s['events'][-1]
    check(mode+' known truncated N remains a declared field',s['termination']=='truncated' and e['declared_payload_N']==20 and
          e['published_output_bytes']==0 and s['unconsumed_file_bytes']==0,
          'recognized raw marker and complete LE32 N were consumed; payload ends early')
    s=stats['encoded-cap-before-payload'];e=s['events'][-1]
    check(mode+' encoded cap preserves N and stops before opaque',s['termination']=='input_limit' and
          e['declared_payload_N']==36145 and e['input_offset']==12 and s['unconsumed_file_bytes']==4,
          'framed_reader per-block encoded cap precedes opaque/payload consumption')
    for name,blocks in (('modern-empty-container',0),('empty-compressed-zero-budget',1)):
        s=stats[name]
        check(mode+' '+name,s['termination']=='end' and s['validated_blocks']==blocks and
              s['validated_output_bytes']==0 and s['validated_output_prefix_sha256']==digest(b''),
              'existing modern complete container/zero-output block policy; not original city acceptance')
    base_args=[exe,inputs/'success-mixed-empty.bin',len(multi),8192,1048576,64,HEADER,COMPRESSED,RAW,36144,36144]
    for name,index,value in (('input-hard-cap',3,8193),('output-hard-cap',4,1048577),('blocks-hard-cap',5,65),
                             ('encoded-hard-cap',9,36145),('decoded-hard-cap',10,36145),('exact-size-mismatch',2,len(multi)+1)):
        argv=list(base_args);argv[index]=value
        run=command(argv,mode+'-'+name)
        check(mode+' '+name,run.returncode==2 and not run.stdout.strip(),
              'C++ qualifier explicit hard limits/regular-file exact size checked before JSON/materialization')
    symlink=inputs/('link-'+mode);symlink.symlink_to(inputs/'success-mixed-empty.bin')
    argv=list(base_args);argv[1]=symlink
    run=command(argv,mode+'-symlink-rejection')
    check(mode+' noncanonical symlink rejected',run.returncode==2 and not run.stdout.strip(),
          'realpath fixed canonical path + O_NOFOLLOW; only self-authored link used')
check('all synthetic input bytes unchanged',all(digest((inputs/(n+'.bin')).read_bytes())==h for n,h in baseline.items()),
      'before/after exact SHA256 of all self-authored inputs')

# Provenance gate fault: deliberately wrong expected core hash, never change a core implementation.
gate=args.output/'gate-source'
ready=json.loads((args.source_root/'docs/original-profile-owned-readiness.json').read_text())
# Copy only named self-authored modern sources/metadata, even when run from a full repository.
relatives=list(ready['fixed_production_source_sha256']) + ['file_io/qualify_data_file.py',
    'file_io/tools/qualify_data_file.cpp','docs/original-profile-owned-readiness.json','docs/original-file-outer-profile.json']
for relative in relatives:
    destination=gate/relative;destination.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(args.source_root/relative,destination)
ready_path=gate/'docs/original-profile-owned-readiness.json'
ready['fixed_production_source_sha256']['file_io/src/strict_zlib.cpp']='0'*64
ready_path.write_text(json.dumps(ready,indent=2)+'\n')
normal_env=dict(os.environ);normal_env.pop('PYTHONOPTIMIZE',None)
cli=[gate/'file_io/qualify_data_file.py','--input',inputs/'success-mixed-empty.bin','--expected-bytes',len(multi),
     '--max-input',8192,'--max-output',1048576,'--max-blocks',64]
for name,flags,env in (('normal-invalid-source',[],normal_env),('optimized-invalid-source',['-O'],normal_env),
                       ('env-optimized-invalid-source',[],dict(normal_env,PYTHONOPTIMIZE='1'))):
    out=args.output/(name+'-output')
    run=command([sys.executable,'-B',*flags,*cli,'--output',out],name,120,env)
    check(name+' rejected before qualification/build',run.returncode!=0 and not out.exists(),
          'documented production/profile source SHA must match before build or file qualification; optimization must not bypass a safety/provenance gate',
          dict(exit_code=run.returncode,output_exists=out.exists(),stdout=run.stdout.strip()))
result=dict(elapsed_seconds=time.monotonic()-begin,passed=sum(c['passed'] for c in checks),
            failed=sum(not c['passed'] for c in checks),checks=checks,commands=commands,
            mode_counts={m:dict(passed=sum(c['passed'] for c in checks if c['name'].startswith(m+' ')),
                               failed=sum(not c['passed'] for c in checks if c['name'].startswith(m+' '))) for m in ('release','ubsan')},
            ASan_run=False,original_data_used=False,original_program_executed=False,network_or_push=False,
            total_budget_seconds=MAX_SECONDS,child_timeout_seconds=30,compile_timeout_seconds=60)
(args.output/'results.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
print(json.dumps({k:result[k] for k in ('passed','failed','elapsed_seconds','mode_counts')}))
raise SystemExit(bool(result['failed']))
