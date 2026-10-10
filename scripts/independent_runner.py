"""Portable, explicit self-authored sources; synthetic inputs; 60s per process."""
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
CONTROL = ['scripts/independent_runner.py', 'scripts/self_fixture_evidence.hpp']
DATA = [
    'runtime_core/src/heap_native.c', 'runtime_core/include/heap_native.h',
    'runtime_core/include/memory.hpp', 'runtime_core/include/thread_state.hpp',
    'runtime_core/tests/boundary_tests.cpp', 'runtime_core/state_codec.py',
    'runtime_core/tests/state_codec_tests.py', 'runtime_core/run_boundaries.sh',
]
MEDIA = [
    'game_lab/pure_boundary_tests.mm', 'game_lab/emperor_assets.mm', 'game_lab/emperor_assets.hpp',
    'game_lab/asset_bytes.hpp', 'game_lab/asset_limits.hpp', 'game_lab/asset_pixels.cpp',
    'game_lab/wav_pcm.cpp', 'game_lab/resource_root.cpp', 'game_lab/resource_root.hpp',
    'game_lab/audio_policy.hpp', 'surface_lab/surface.cpp', 'surface_lab/surface.hpp',
    'game_lab/run_boundaries.sh',
]
APP = [
    'game_lab/app/main.mm', 'game_lab/app/build.sh', 'game_lab/app/build_provenance.py',
    'game_lab/app/Info.plist', 'game_lab/app/av_audio_backend.hpp', 'game_lab/app/av_audio_backend.mm',
    'game_lab/app_media.hpp', 'game_lab/app_media.cpp', 'game_lab/image_presentation.hpp', 'game_lab/app_media_integration_tests.mm',
    'game_lab/emperor_assets.hpp', 'game_lab/emperor_assets.mm',
    'game_lab/asset_bytes.hpp', 'game_lab/asset_limits.hpp', 'game_lab/asset_pixels.cpp',
    'game_lab/wav_pcm.cpp', 'game_lab/resource_root.hpp', 'game_lab/resource_root.cpp',
    'game_lab/audio_policy.hpp', 'surface_lab/surface.hpp', 'surface_lab/surface.cpp',
    'game_lab/run_app_integration.sh',
]
SUITES = {'data': DATA, 'media': MEDIA, 'integration': APP, 'app': APP}


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        while block := stream.read(1024 * 1024):
            digest.update(block)
    return digest.hexdigest()


def save(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n')


def read(path):
    return json.loads(path.read_text())


def verify(report, root):
    result = read(report / 'result.json')
    if not result['passed']:
        raise RuntimeError('Run did not pass')
    for name, expected in read(report / 'result-manifest.json').items():
        if sha(report / name) != expected:
            raise RuntimeError('Changed report: ' + name)
    sources = read(report / 'source-manifest.json')['sources_sha256']
    expected_names = set(CONTROL + SUITES[result['suite']])
    if set(sources) != expected_names:
        raise RuntimeError('Unexpected source set')
    for name, expected in sources.items():
        if sha(root / name) != expected or sha(report / 'source' / name) != expected:
            raise RuntimeError('Changed source: ' + name)
    for name, entry in read(report / 'input-manifest.json')['actual_inputs'].items():
        path = report / name
        if 'symlink_target' in entry:
            if not path.is_symlink() or os.readlink(path) != entry['symlink_target']:
                raise RuntimeError('Changed fixture symlink: ' + name)
        elif path.is_symlink() or sha(path) != entry['sha256'] or path.stat().st_size != entry['bytes']:
            raise RuntimeError('Changed fixture: ' + name)
    for name, expected in read(report / 'build-manifest.json')['binaries_sha256'].items():
        if sha(report / name) != expected:
            raise RuntimeError('Changed binary: ' + name)
    for process in read(report / 'processes.json'):
        if process['exit_code'] != 0 or process['timed_out'] or process['timeout_seconds'] != 60 or process['elapsed_seconds'] >= 60:
            raise RuntimeError('Process not accepted')
    if result['suite'] == 'app':
        app = report / 'build/EmperorApp.app'
        package = read(report / 'package-manifest.json')
        if package['source_manifest_sha256'] != sha(report / 'source-manifest.json'):
            raise RuntimeError('Package/source binding mismatch')
        for name, expected in package['files_sha256'].items():
            if sha(app / name) != expected:
                raise RuntimeError('Changed package file: ' + name)
        embedded = read(app / 'Contents/Resources/source-manifest.json')
        if any(sources.get(name) != expected for name, expected in embedded['sources_sha256'].items()):
            raise RuntimeError('Embedded production source mismatch')
    return result


def run(suite, mode, output):
    output.mkdir(parents=True, exist_ok=True)
    report = Path(tempfile.mkdtemp(prefix=suite + '-' + mode + '-', dir=output))
    snapshot = report / 'source'
    records = {}
    for name in sorted(set(CONTROL + SUITES[suite])):
        original = ROOT / name
        if original.is_symlink() or not original.is_file():
            raise RuntimeError('Unreviewed/missing source: ' + name)
        content = original.read_bytes()
        records[name] = hashlib.sha256(content).hexdigest()
        copy = snapshot / name
        copy.parent.mkdir(parents=True, exist_ok=True)
        copy.write_bytes(content)
    save(report / 'source-manifest.json', {'suite': suite, 'mode': mode, 'sources_sha256': records})
    temporary = report / 'tmp'
    temporary.mkdir()
    environment = {
        'PATH': '/usr/bin:/bin:/usr/sbin:/sbin', 'TMPDIR': str(temporary), 'LANG': 'en_US.UTF-8',
        'SELF_FIXTURE_DIR': str(report / 'fixtures'), 'SELF_INPUT_TRACE': str(report / 'inputs.jsonl'),
        'EMPEROR_BUILD_DIR': str(report / 'build'),
    }
    processes = []
    print('REPORT ' + str(report), flush=True)

    def command(label, argv):
        entry = {'label': label, 'argv': [str(value) for value in argv], 'timeout_seconds': 60}
        start = time.monotonic()
        with (report / (label + '.stdout')).open('wb') as stdout, (report / (label + '.stderr')).open('wb') as stderr:
            process = subprocess.Popen(entry['argv'], cwd=report, env=environment, stdout=stdout,
                                       stderr=stderr, start_new_session=True)
            try:
                entry['exit_code'] = process.wait(timeout=60)
                entry['timed_out'] = False
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait(timeout=5)
                entry['exit_code'] = process.returncode
                entry['timed_out'] = True
        entry['elapsed_seconds'] = time.monotonic() - start
        processes.append(entry)
        save(report / 'processes.json', processes)
        print(label + ' exit=' + str(entry['exit_code']) + ' elapsed=' + str(round(entry['elapsed_seconds'], 3)), flush=True)
        if entry['exit_code'] or entry['timed_out']:
            raise RuntimeError(label + ': ' + (report / (label + '.stderr')).read_text(errors='replace')[-4000:])

    passed, error, results, binaries = False, None, {}, {}
    flags = ['-O2'] if mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    try:
        command('compiler-version', ['/usr/bin/clang++', '--version'])
        if suite == 'data':
            command('compile-c', ['/usr/bin/clang', '-std=c11', '-Wall', '-Wextra', '-Werror', *flags,
                                 '-I', snapshot / 'runtime_core/include', '-c', snapshot / 'runtime_core/src/heap_native.c', '-o', report / 'heap_native.o'])
            command('compile-cpp', ['/usr/bin/clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror', *flags,
                                   '-I', snapshot / 'runtime_core/include', snapshot / 'runtime_core/tests/boundary_tests.cpp', report / 'heap_native.o', '-o', report / 'data-tests'])
            binaries = {name: sha(report / name) for name in ['heap_native.o', 'data-tests']}
            command('data-test', [report / 'data-tests'])
            command('tlv-test', ['/usr/bin/python3', '-I', '-B', snapshot / 'runtime_core/tests/state_codec_tests.py'])
            results = {'data': read(report / 'data-test.stdout'), 'tlv': read(report / 'tlv-test.stdout')}
        elif suite in ('media', 'integration'):
            compiled = ['game_lab/emperor_assets.mm', 'game_lab/wav_pcm.cpp', 'game_lab/asset_pixels.cpp',
                        'game_lab/resource_root.cpp', 'surface_lab/surface.cpp']
            if suite == 'media':
                compiled.insert(0, 'game_lab/pure_boundary_tests.mm')
            else:
                compiled[:0] = ['game_lab/app_media_integration_tests.mm', 'game_lab/app_media.cpp']
            command('compile-test', ['/usr/bin/clang++', '-std=c++17', '-fobjc-arc', '-fno-modules',
                                     '-Wall', '-Wextra', '-Werror', *flags, *[snapshot / name for name in compiled],
                                     '-framework', 'Foundation', '-framework', 'ImageIO', '-framework', 'CoreGraphics',
                                     '-lz', '-o', report / 'self-tests'])
            binaries = {'self-tests': sha(report / 'self-tests')}
            command('self-test', [report / 'self-tests'])
            results = {suite: read(report / 'self-test.stdout')}
        else:
            command('app-build', ['/bin/sh', snapshot / 'game_lab/app/build.sh'])
            app = report / 'build/EmperorApp.app'
            command('signature-verify', ['/usr/bin/codesign', '--verify', '--strict', '--verbose=2', app])
            command('linked-libraries', ['/usr/bin/otool', '-L', app / 'Contents/MacOS/EmperorApp'])
            libraries = [line.strip().split(' (')[0] for line in (report / 'linked-libraries.stdout').read_text().splitlines()[1:]]
            if not libraries or any(not name.startswith(('/System/Library/', '/usr/lib/')) for name in libraries):
                raise RuntimeError('Unexpected linked library')
            embedded = read(app / 'Contents/Resources/source-manifest.json')
            if any(records.get(name) != expected for name, expected in embedded['sources_sha256'].items()):
                raise RuntimeError('Embedded source mismatch')
            package = {path.relative_to(app).as_posix(): sha(path) for path in app.rglob('*') if path.is_file() and not path.is_symlink()}
            save(report / 'package-manifest.json', {'files_sha256': package, 'source_manifest_sha256': sha(report / 'source-manifest.json'), 'app_launched': False})
            binaries = {'build/EmperorApp.app/Contents/MacOS/EmperorApp': package['Contents/MacOS/EmperorApp']}
            results = {'app': {'passed': True, 'signature_verified': True, 'linked_libraries': libraries, 'app_launched': False}}
        if any(value.get('passed') is not True for value in results.values()):
            raise RuntimeError('Required test result failed')
        if any(sha(ROOT / name) != expected or sha(snapshot / name) != expected for name, expected in records.items()):
            raise RuntimeError('Sources changed during validation')
        passed = True
    except Exception as caught:
        error = str(caught)
    inputs = {}
    for name in ['fixtures', 'self-root']:
        if (report / name).exists():
            for path in (report / name).rglob('*'):
                key = path.relative_to(report).as_posix()
                if path.is_symlink():
                    inputs[key] = {'symlink_target': os.readlink(path)}
                elif path.is_file():
                    inputs[key] = {'sha256': sha(path), 'bytes': path.stat().st_size}
    for name in ['inputs.jsonl', 'self-outside.wav']:
        if (report / name).is_file():
            inputs[name] = {'sha256': sha(report / name), 'bytes': (report / name).stat().st_size}
    save(report / 'input-manifest.json', {'actual_inputs': inputs, 'self_authored': True})
    save(report / 'build-manifest.json', {'binaries_sha256': binaries, 'source_manifest_sha256': sha(report / 'source-manifest.json')})
    save(report / 'result.json', {'passed': passed, 'suite': suite, 'mode': mode, 'results': results, 'error': error,
                                 'ui': False, 'audio_devices': False, 'GameData': False, 'legacy': False, 'app_launched': False})
    save(report / 'result-manifest.json', {path.name: sha(path) for path in report.iterdir() if path.is_file() and path.name != 'result-manifest.json'})
    if passed:
        verify(report, ROOT)
    print(json.dumps({'passed': passed, 'report': str(report), 'results': results, 'error': error}, ensure_ascii=False), flush=True)
    return 0 if passed else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('suite', choices=[*SUITES, 'verify'])
    parser.add_argument('--mode', choices=['release', 'undefined'], default='release')
    parser.add_argument('--output', type=Path, default=ROOT / '.local-results/independent')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    if args.suite == 'verify':
        if args.report is None or not args.report.is_absolute():
            parser.error('verify requires --report /absolute/path')
        result = verify(args.report, ROOT)
        print(json.dumps({'verified': True, 'suite': result['suite'], 'report': str(args.report)}))
        return 0
    if not args.output.is_absolute():
        parser.error('--output must be absolute')
    if args.suite == 'app' and args.mode != 'release':
        parser.error('compile-only App packaging supports release mode')
    return run(args.suite, args.mode, args.output)


if __name__ == '__main__':
    raise SystemExit(main())
