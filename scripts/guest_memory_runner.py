"""Standalone authored native memory tests on 64-bit macOS; no guest execution."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import platform
import signal
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
SOURCES = [
    'scripts/guest_memory_runner.py',
    'runtime_core/tests/guest_memory_native_tests.c',
    'runtime_core/tests/guest_memory_test_hooks.h',
    'legacy/src/stage79/tools/guest_memory.c',
    'legacy/src/stage79/tools/guest_memory.h',
]

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def save(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n')

def verify(report):
    result = json.loads((report/'result.json').read_text())
    assert result['passed'] and result['source_stable']
    manifest = json.loads((report/'source-manifest.json').read_text())['sources_sha256']
    assert set(manifest) == set(SOURCES)
    assert all(sha(ROOT/name) == value and sha(report/'source'/name) == value for name, value in manifest.items())
    assert all(sha(report/name) == value for name, value in json.loads((report/'result-manifest.json').read_text()).items())
    inputs = json.loads((report/'input-manifest.json').read_text())
    assert inputs['self_authored'] is True
    assert set(inputs['actual_inputs']) == {'fixtures/full.bin', 'fixtures/short.bin', 'inputs.jsonl'}
    assert all(sha(report/name) == record['sha256'] and (report/name).stat().st_size == record['bytes']
               for name, record in inputs['actual_inputs'].items())
    build = json.loads((report/'build-manifest.json').read_text())
    assert set(build['binaries_sha256']) == {'guest_memory.o', 'guest-tests.o', 'guest-tests'}
    assert all(sha(report/name) == value for name, value in build['binaries_sha256'].items())
    assert sha(report/'source-manifest.json') == build['source_manifest_sha256']
    assert all(p['exit_code'] == 0 and not p['timed_out'] and p['elapsed_seconds'] < 60 and p['timeout_seconds'] == 60
               for p in json.loads((report/'processes.json').read_text()))
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=['release', 'undefined'], default='undefined')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--verify', type=Path)
    args = parser.parse_args()
    if args.verify:
        if not args.verify.is_absolute():
            parser.error('--verify requires an absolute report path')
        verify(args.verify)
        print(json.dumps({'verified': True, 'report': str(args.verify)}))
        return 0
    if not args.output or not args.output.is_absolute():
        parser.error('--output requires an absolute directory')
    if platform.system() != 'Darwin' or sys.maxsize <= 2**32:
        parser.error('native mapping/protection inspection requires 64-bit macOS')
    args.output.mkdir(parents=True, exist_ok=True)
    report = Path(tempfile.mkdtemp(prefix='guest-memory-'+args.mode+'-', dir=args.output))
    snapshot = report/'source'
    records = {}
    for name in SOURCES:
        path = ROOT/name
        if path.is_symlink() or not path.is_file():
            raise RuntimeError('Missing or unexpected source: '+name)
        content = path.read_bytes()
        records[name] = hashlib.sha256(content).hexdigest()
        dest = snapshot/name
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(content)
    save(report/'source-manifest.json', {'sources_sha256': records, 'mode': args.mode})
    (report/'tmp').mkdir()
    (report/'fixtures').mkdir()
    fixture = bytes((0x20+i) % 256 for i in range(8192))
    (report/'fixtures/full.bin').write_bytes(fixture)
    (report/'fixtures/short.bin').write_bytes(fixture[:31])
    environment = {'PATH': '/usr/bin:/bin:/usr/sbin:/sbin', 'LANG': 'en_US.UTF-8', 'TMPDIR': str(report/'tmp')}
    processes = []
    print('REPORT '+str(report), flush=True)

    def command(label, argv):
        entry = {'label': label, 'argv': [str(v) for v in argv], 'timeout_seconds': 60}
        start = time.monotonic()
        with (report/(label+'.stdout')).open('wb') as out, (report/(label+'.stderr')).open('wb') as err:
            p = subprocess.Popen(entry['argv'], cwd=report, env=environment, stdout=out, stderr=err, start_new_session=True)
            try:
                entry['exit_code'] = p.wait(timeout=60)
                entry['timed_out'] = False
            except subprocess.TimeoutExpired:
                os.killpg(p.pid, signal.SIGKILL)
                p.wait(timeout=5)
                entry['exit_code'] = p.returncode
                entry['timed_out'] = True
        entry['elapsed_seconds'] = time.monotonic()-start
        processes.append(entry)
        save(report/'processes.json', processes)
        print(label+' exit='+str(entry['exit_code']), flush=True)
        if entry['exit_code'] or entry['timed_out']:
            raise RuntimeError(label+': '+(report/(label+'.stderr')).read_text(errors='replace')[-3000:])

    flags = ['-O2'] if args.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    common = ['/usr/bin/clang', '-std=c11', '-fno-modules', '-Wall', '-Wextra', '-Werror', *flags,
              '-I', snapshot/'legacy/src/stage79/tools']
    passed, error, tests = False, None, {}
    try:
        command('compiler-version', ['/usr/bin/clang', '--version'])
        command('compile-provider', [*common, '-Wno-logical-op-parentheses',
                '-include', snapshot/'runtime_core/tests/guest_memory_test_hooks.h',
                '-c', snapshot/'legacy/src/stage79/tools/guest_memory.c', '-o', report/'guest_memory.o'])
        command('compile-test', [*common, '-c', snapshot/'runtime_core/tests/guest_memory_native_tests.c',
                                '-o', report/'guest-tests.o'])
        command('link-self-test', ['/usr/bin/clang', *flags, report/'guest_memory.o', report/'guest-tests.o',
                                   '-o', report/'guest-tests'])
        command('native-self-test', [report/'guest-tests'])
        tests = json.loads((report/'native-self-test.stdout').read_text())
        for key in ['passed', 'cross_region', 'native_permissions_and_nx', 'overflow', 'failure_cleanup', 'repeated_lifecycle']:
            if tests.get(key) is not True:
                raise RuntimeError('Missing successful contract: '+key)
        if tests.get('cycles') != 64 or any(tests.get(key) is not False for key in ['GameData','checkpoint','guest_code','ui','audio_devices']):
            raise RuntimeError('Unexpected test scope')
        passed = True
    except Exception as caught:
        error = str(caught)
    stable = all(sha(ROOT/name) == value and sha(snapshot/name) == value for name, value in records.items())
    inputs = {p.relative_to(report).as_posix(): {'sha256': sha(p), 'bytes': p.stat().st_size}
              for p in [report/'fixtures/full.bin', report/'fixtures/short.bin', report/'inputs.jsonl'] if p.is_file()}
    save(report/'input-manifest.json', {'self_authored': True, 'actual_inputs': inputs})
    binaries = {p.name: sha(p) for p in [report/'guest_memory.o', report/'guest-tests.o', report/'guest-tests'] if p.is_file()}
    save(report/'build-manifest.json', {'binaries_sha256': binaries, 'source_manifest_sha256': sha(report/'source-manifest.json')})
    save(report/'result.json', {'passed': passed and stable, 'source_stable': stable, 'mode': args.mode, 'tests': tests,
                               'error': error, 'native_os_mapping': True, 'fault_injection': True,
                               'GameData': False, 'checkpoint': False, 'original_guest_code': False, 'ui': False, 'audio_devices': False})
    save(report/'result-manifest.json', {p.name: sha(p) for p in report.iterdir() if p.is_file() and p.name != 'result-manifest.json'})
    if passed and stable:
        verify(report)
    print(json.dumps({'report': str(report), 'passed': passed and stable, 'tests': tests, 'error': error}), flush=True)
    return 0 if passed and stable else 1

if __name__ == '__main__':
    raise SystemExit(main())
