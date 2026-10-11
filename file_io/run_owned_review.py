#!/usr/bin/env python3
"""Self-authored ownership tests only, not the independent reviewer suite."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
out = args.output
if not out.is_absolute() or out.exists(): parser.error('new absolute output required')
out.mkdir(parents=True)
sdk = subprocess.check_output(['/usr/bin/xcrun', '--sdk', 'macosx', '--show-sdk-path'], text=True).strip()
commands, results = [], []
for mode, flags in [('release', ['-O2']), ('undefined', ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all'])]:
    exe = out / ('owned-' + mode)
    argv = ['/usr/bin/clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-isysroot', sdk,
            '-I', str(root/'include'), str(root/'src/strict_zlib.cpp'), str(root/'src/framed_reader.cpp'),
            str(root/'tests/owned_reader_tests.cpp'), *flags, '-lz', '-o', str(exe)]
    build = subprocess.run(argv, capture_output=True, text=True, timeout=60)
    commands.append({'argv': argv, 'exit_code': build.returncode, 'stderr': build.stderr})
    assert build.returncode == 0, build.stderr
    run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=15)
    commands.append({'argv': [str(exe)], 'exit_code': run.returncode, 'stderr': run.stderr})
    rows = [json.loads(line) for line in run.stdout.splitlines()]
    results.append({'mode': mode, 'checks': rows[:-1], 'summary': rows[-1]})
    assert run.returncode == 0 and not run.stderr and rows[-1]['passed'] == 6 and rows[-1]['failed'] == 0
receipt = {'scope': 'ownership wrapper author regressions; not independent review or original file/game acceptance',
           'results': results, 'commands': commands, 'ASan_run': False,
           'source_hashes': {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
               for p in [root/'include/owned_reader.hpp', root/'include/framed_reader.hpp',
                         root/'tests/owned_reader_tests.cpp', root/'src/strict_zlib.cpp', root/'src/framed_reader.cpp',
                         Path(__file__).resolve()]}}
(out/'results.json').write_text(json.dumps(receipt, ensure_ascii=False, indent=2)+'\n')
print(json.dumps([r['summary'] for r in results]))
