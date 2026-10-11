#!/usr/bin/env python3
"""Read a separately authorized small data file; emit counts/hashes, never bytes."""
import sys
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--input', type=Path, required=True)
parser.add_argument('--expected-bytes', type=int, required=True)
parser.add_argument('--max-input', type=int, required=True)
parser.add_argument('--max-output', type=int, required=True)
parser.add_argument('--max-blocks', type=int, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
if sys.flags.optimize:
    parser.error("Python optimization disables mandatory validation checks; use unoptimized Python")
root = Path(__file__).resolve().parent
repo = root.parent
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
if not args.input.is_absolute() or args.input.resolve() != args.input:
    parser.error('fixed absolute canonical nonsymlink input required')
if not args.output.is_absolute() or args.output.exists(): parser.error('fresh absolute output required')
if not (4 <= args.expected_bytes <= args.max_input <= 8192 and 0 <= args.max_output <= 1048576 and 0 <= args.max_blocks <= 64):
    parser.error('explicit budgets exceed tool supported domain')
ready = json.loads((repo/'docs/original-profile-owned-readiness.json').read_text())
for relative, digest in ready['fixed_production_source_sha256'].items():
    if sha(repo/relative) != digest:
        raise RuntimeError('reviewed production source SHA mismatch: '+relative)
profile_path = repo/'docs/original-file-outer-profile.json'
if sha(profile_path) != ready['source_profile_sha256']:
    raise RuntimeError('reviewed source profile SHA mismatch')
profile = json.loads(profile_path.read_text())
markers = profile['marker_parameters']
args.output.mkdir(parents=True)
commands = []
def command(argv, label, timeout):
    begin = time.monotonic()
    result = subprocess.run([str(x) for x in argv], capture_output=True, text=True, timeout=timeout)
    stdout, stderr = args.output/(label+'.stdout'), args.output/(label+'.stderr')
    stdout.write_text(result.stdout); stderr.write_text(result.stderr)
    commands.append({'argv':[str(x) for x in argv], 'exit_code':result.returncode,
                     'elapsed_seconds':time.monotonic()-begin, 'stdout':stdout.name,
                     'stdout_sha256':sha(stdout), 'stderr':stderr.name, 'stderr_sha256':sha(stderr)})
    return result
try:
    sdk = command(['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'],'sdk',30)
    if sdk.returncode: raise RuntimeError('installed SDK lookup failed; no alternative attempted')
    exe = args.output/'qualify-data-file'
    build = command(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror','-O2','-isysroot',sdk.stdout.strip(),
                     '-I',root/'include',root/'src/strict_zlib.cpp',root/'src/framed_reader.cpp',
                     root/'tools/qualify_data_file.cpp','-lz','-o',exe],'build',60)
    if build.returncode: raise RuntimeError('self-authored tool build failed; no alternative attempted')
    run = command([exe,args.input,args.expected_bytes,args.max_input,args.max_output,args.max_blocks,
                   markers['container_marker']['value_u32'],markers['compressed_marker']['value_u32'],
                   markers['raw_marker']['value_u32'],36144,36144],'qualification',30)
    if run.returncode: raise RuntimeError('fixed-path readonly qualification failed; consult stderr; no retry or alternative path')
    statistics = json.loads(run.stdout)
    if statistics['canonical_path'] != str(args.input) or statistics['raw_or_decoded_bytes_exported']:
        raise RuntimeError('qualification path/export result mismatch')
    (args.output/'statistics.json').write_text(json.dumps(statistics,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps({key:statistics[key] for key in ('input_bytes','input_sha256','termination',
          'validated_blocks','validated_output_bytes','unconsumed_file_bytes')},ensure_ascii=False))
finally:
    (args.output/'command-receipt.json').write_text(json.dumps({'commands':commands,
        'tool_source_sha256':sha(root/'tools/qualify_data_file.cpp'),
        'production_source_sha256':ready['fixed_production_source_sha256'],
        'input_path_not_switched':True,'only_selfauthored_modern_component_and_system_zlib':True,
        'original_program_or_codec_executed':False,'input_written':False,
        'raw_or_decoded_bytes_exported':False,'network_or_upload':False},ensure_ascii=False,indent=2)+'\n')
