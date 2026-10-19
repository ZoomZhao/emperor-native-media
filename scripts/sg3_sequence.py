"""Build/test explicit SG3 asset sequences; no inferred chains or game timing."""
import os
import sys
if len(sys.argv) > 1 and sys.argv[1] == 'metrics' and not os.environ.get('EMPEROR_GAMEDATA_ROOT'):
    print('{"skipped":true,"reason":"EMPEROR_GAMEDATA_ROOT unset or empty"}')
    raise SystemExit(0)
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('task', choices=('test', 'metrics'))
    parser.add_argument('--mode', choices=('release', 'undefined'), default='release')
    parser.add_argument('--manifest', type=Path)
    parser.add_argument('--build-dir', type=Path)
    args = parser.parse_args()
    build = args.build_dir or Path(tempfile.mkdtemp(prefix='emperor-sequence-'))
    if not build.is_absolute():parser.error('absolute build directory required')
    root = Path(os.environ['EMPEROR_GAMEDATA_ROOT']).resolve() if os.environ.get('EMPEROR_GAMEDATA_ROOT') else None
    if root and (build.resolve() == root or root in build.resolve().parents):parser.error('build must stay outside GameData')
    build.mkdir(parents=True, exist_ok=True)
    binary = build / ('sg3-sequence-' + args.task + '-' + args.mode)
    flags = ['-O2'] if args.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    sources = ['game_lab/sg3_sequence_tests.cpp' if args.task == 'test' else 'game_lab/sg3_sequence_metrics.cpp',
               'game_lab/sg3.cpp', 'game_lab/asset_pixels.cpp', 'game_lab/resource_root.cpp', 'surface_lab/surface.cpp']
    frameworks = [] if args.task == 'test' else ['-framework', 'CoreGraphics', '-framework', 'CoreFoundation']
    subprocess.run(['/usr/bin/clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror', *flags,
                    *[str(ROOT / s) for s in sources], *frameworks, '-o', str(binary)], check=True)
    if args.task == 'test':return subprocess.run([str(binary)]).returncode
    if not args.manifest or not args.manifest.is_absolute() or args.manifest.stat().st_size > 65536:parser.error('bounded absolute local manifest required')
    entries = json.loads(args.manifest.read_text()).get('sequences')
    if not isinstance(entries, list) or not 1 <= len(entries) <= 6:parser.error('1..6 explicit sample sequences required')
    before = {}
    for e in entries:
        if not isinstance(e.get('archive'), str) or not isinstance(e.get('physical_ids'), list) or not 1 <= len(e['physical_ids']) <= 256 or any(type(i) is not int or not 1 <= i <= 0xffffffff for i in e['physical_ids']):parser.error('explicit physical IDs required')
        for name in (Path(e['archive']), Path(e['archive']).with_suffix('.555')):
            source = (root / name).resolve()
            if name.is_absolute() or '..' in name.parts or root not in source.parents or not source.is_file():parser.error('source or companion outside root/missing')
            before[source] = hashlib.sha256(source.read_bytes()).digest()
    reports = []
    for e in entries:
        q = subprocess.run([str(binary), e['archive'], ','.join(map(str, e['physical_ids']))], capture_output=True, text=True, check=True)
        reports.append(json.loads(q.stdout))
    for path, digest in before.items():
        if hashlib.sha256(path.read_bytes()).digest() != digest:raise ValueError('source changed across sequence audit')
    print(json.dumps({'skipped': False, 'sequences': len(reports), 'frames': sum(r['frames'] for r in reports),
                      'native_images': sum(r['native_images'] for r in reports), 'exact_source_frames': sum(r['exact_source_frames'] for r in reports),
                      'input_files_unchanged': [len(before), len(before)], 'records': reports,
                      'original_timing_verified': False, 'automatic_frame_chain': False}))
    return 0

if __name__ == '__main__':raise SystemExit(main())
