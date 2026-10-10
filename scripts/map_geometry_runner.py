"""Shared coordinate component only; synthetic inputs; 60 seconds per process."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import signal
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['game_lab/map_geometry.hpp', 'game_lab/map_geometry_tests.cpp',
           'scripts/map_geometry_runner.py']


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n')


def read(path):
    return json.loads(path.read_text())


def verify(report):
    result = read(report / 'result.json')
    if result.get('passed') is not True:
        raise RuntimeError('Run did not pass')
    for name, expected in read(report / 'receipt-manifest.json').items():
        if sha(report / name) != expected:
            raise RuntimeError('Changed receipt: ' + name)
    sources = read(report / 'source-manifest.json')
    if set(sources) != set(SOURCES):
        raise RuntimeError('Unexpected source set')
    for name, expected in sources.items():
        if sha(ROOT / name) != expected or sha(report / 'source' / name) != expected:
            raise RuntimeError('Changed source: ' + name)
    if sha(report / 'map-input-tests') != read(report / 'binary-manifest.json')['sha256']:
        raise RuntimeError('Changed binary')
    for process in read(report / 'processes.json'):
        if process['exit_code'] != 0 or process['timed_out'] or process['timeout_seconds'] != 60:
            raise RuntimeError('Process not accepted')
    return result


def run(mode, output):
    output.mkdir(parents=True, exist_ok=True)
    report = Path(tempfile.mkdtemp(prefix='map-input-' + mode + '-', dir=output))
    sources = {}
    for name in SOURCES:
        original = ROOT / name
        if original.is_symlink() or not original.is_file():
            raise RuntimeError('Missing or unreviewed source: ' + name)
        copy = report / 'source' / name
        copy.parent.mkdir(parents=True, exist_ok=True)
        copy.write_bytes(original.read_bytes())
        sources[name] = sha(copy)
    save(report / 'source-manifest.json', sources)
    temporary = report / 'tmp'
    temporary.mkdir()
    environment = {'PATH': '/usr/bin:/bin:/usr/sbin:/sbin', 'TMPDIR': str(temporary),
                   'LANG': 'en_US.UTF-8'}
    processes = []

    def command(label, argv):
        start = time.monotonic()
        with (report / (label + '.stdout')).open('wb') as stdout, (report / (label + '.stderr')).open('wb') as stderr:
            process = subprocess.Popen(argv, cwd=report, env=environment, stdout=stdout,
                                       stderr=stderr, start_new_session=True)
            timed_out = False
            try:
                process.wait(timeout=60)
            except subprocess.TimeoutExpired:
                timed_out = True
                os.killpg(process.pid, signal.SIGKILL)
                process.wait(timeout=5)
        entry = {'label': label, 'argv': argv, 'exit_code': process.returncode,
                 'timed_out': timed_out, 'timeout_seconds': 60,
                 'elapsed_seconds': time.monotonic() - start}
        processes.append(entry)
        save(report / 'processes.json', processes)
        if timed_out or process.returncode:
            raise RuntimeError(label + ': ' + (report / (label + '.stderr')).read_text(errors='replace')[-2000:])

    flags = ['-O2'] if mode == 'release' else ['-O1', '-g', '-fsanitize=undefined,float-cast-overflow', '-fno-sanitize-recover=all']
    passed, error, test = False, None, None
    try:
        command('compile', ['/usr/bin/clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror', *flags,
                            '-I', str(report / 'source/game_lab'),
                            str(report / 'source/game_lab/map_geometry_tests.cpp'),
                            '-o', str(report / 'map-input-tests')])
        save(report / 'binary-manifest.json', {'sha256': sha(report / 'map-input-tests')})
        command('test', [str(report / 'map-input-tests')])
        test = read(report / 'test.stdout')
        if test.get('passed') is not True:
            raise RuntimeError('Required test result failed')
        if any(sha(ROOT / name) != expected for name, expected in sources.items()):
            raise RuntimeError('Sources changed during validation')
        passed = True
    except Exception as caught:
        error = str(caught)
    save(report / 'result.json', {'passed': passed, 'mode': mode, 'test': test, 'error': error,
                                 'GameData': False, 'city_model_executed': False,
                                 'ui': False, 'audio_devices': False})
    save(report / 'receipt-manifest.json', {p.name: sha(p) for p in report.iterdir()
                                          if p.is_file() and p.name != 'receipt-manifest.json'})
    if passed:
        verify(report)
    print(json.dumps({'passed': passed, 'report': str(report), 'test': test, 'error': error}))
    return 0 if passed else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=['release', 'undefined'], default='release')
    parser.add_argument('--output', type=Path, default=ROOT / '.local-results/map-input')
    parser.add_argument('--verify', type=Path)
    args = parser.parse_args()
    if args.verify:
        if not args.verify.is_absolute():
            parser.error('--verify requires an absolute report directory')
        result = verify(args.verify)
        print(json.dumps({'verified': True, 'report': str(args.verify), 'mode': result['mode']}))
        return 0
    if not args.output.is_absolute():
        parser.error('--output must be absolute')
    return run(args.mode, args.output)


if __name__ == '__main__':
    raise SystemExit(main())
