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
    'game_lab/pure_boundary_tests.mm', 'game_lab/sg3.cpp', 'game_lab/emperor_assets.mm', 'game_lab/emperor_assets.hpp',
    'game_lab/asset_bytes.hpp', 'game_lab/asset_limits.hpp', 'game_lab/asset_pixels.cpp',
    'game_lab/wav_pcm.cpp', 'game_lab/resource_root.cpp', 'game_lab/resource_root.hpp',
    'game_lab/audio_policy.hpp', 'surface_lab/surface.cpp', 'surface_lab/surface.hpp',
    'game_lab/run_boundaries.sh',
]
APP = [
    'LICENSE', 'THIRD_PARTY_NOTICES.md', 'README.md', 'game_lab/city_simulation.hpp', 'game_lab/city_state_io.hpp',
    'game_lab/city_game_state.hpp', 'game_lab/app/city_scenario.hpp', 'game_lab/app/city_scenario_checkpoint.hpp',
    'game_lab/app/city_warehouse_training.hpp', 'game_lab/app/city_warehouse_transfer.hpp', 'game_lab/app/city_warehouse_routes.hpp',
    'game_lab/app/main.mm', 'game_lab/app/city_goal_feedback.hpp', 'game_lab/app/city_goal_feedback_adapter.hpp',
    'game_lab/app/build.sh', 'game_lab/app/build_provenance.py',
    'game_lab/app/Info.plist', 'game_lab/app/av_audio_backend.hpp', 'game_lab/app/av_audio_backend.mm',
    'game_lab/app_media.hpp', 'game_lab/app_media.cpp',
    'game_lab/movie_session.hpp', 'game_lab/movie_session.mm',
    'game_lab/image_presentation.hpp', 'game_lab/cg_image_conversion.hpp', 'game_lab/app_launch.hpp', 'game_lab/map_geometry.hpp', 'game_lab/map_viewport.hpp', 'game_lab/app_media_integration_tests.mm',
    'game_lab/emperor_assets.hpp', 'game_lab/emperor_assets.mm',
    'game_lab/asset_bytes.hpp', 'game_lab/asset_limits.hpp', 'game_lab/asset_pixels.cpp',
    'game_lab/wav_pcm.cpp', 'game_lab/resource_root.hpp', 'game_lab/resource_root.cpp',
    'game_lab/audio_policy.hpp', 'surface_lab/surface.hpp', 'surface_lab/surface.cpp',
    'stream_lab/movie_io.hpp', 'stream_lab/movie_io.mm',
    'stream_lab/presenter.hpp', 'stream_lab/presenter.cpp',
    'video_lab/stream.hpp', 'video_lab/stream.cpp', 'video_lab/video.hpp',
    'metal_lab/backend.hpp', 'metal_lab/backend.mm',
    'audio_lab/audio.hpp', 'audio_lab/audio.cpp',
    'media_demo/host_clock.hpp',
    'media_demo/controller.hpp', 'media_demo/controller.cpp',
    'media_demo/device_monitor.hpp',
    'game_lab/run_app_integration.sh',
    'game_lab/candidate_import.hpp',
    'game_lab/candidate_import.cpp', 'game_lab/sg3.cpp',
    'file_io/include/strict_zlib.h',
    'file_io/include/framed_reader.hpp',
    'file_io/include/owned_reader.hpp',
    'file_io/include/city19_goal_archive.hpp',
    'file_io/src/strict_zlib.cpp',
    'file_io/src/framed_reader.cpp',
]
CITY = ['game_lab/city_simulation.hpp', 'game_lab/city_state_io.hpp',
        'game_lab/city_simulation_tests.cpp', 'game_lab/emperor_assets.hpp',
        'game_lab/asset_limits.hpp', 'game_lab/map_geometry.hpp']
SERVICE = [name for name in CITY if name != 'game_lab/city_simulation_tests.cpp'] + [
    'game_lab/service_labor_tests.cpp', 'game_lab/app/city_goal_feedback.hpp', 'game_lab/app/city_goal_feedback_adapter.hpp']
SCENARIO_STATE = [name for name in SERVICE if name != 'game_lab/service_labor_tests.cpp'] + [
    'game_lab/city_game_state.hpp', 'game_lab/app/city_scenario.hpp',
    'game_lab/app/city_scenario_checkpoint.hpp', 'game_lab/app/city_warehouse_training.hpp', 'game_lab/app/city_warehouse_transfer.hpp', 'game_lab/app/city_warehouse_routes.hpp',
    'game_lab/city_game_state_tests.cpp']
WAREHOUSE_TRAINING_STATE = [name for name in SCENARIO_STATE if name != 'game_lab/city_game_state_tests.cpp'] + ['game_lab/warehouse_training_state_tests.cpp']
WAREHOUSE = [name for name in SCENARIO_STATE if name != 'game_lab/city_game_state_tests.cpp'] + ['game_lab/warehouse_roads_tests.cpp']
WAREHOUSE_TRANSFER = [name for name in WAREHOUSE_TRAINING_STATE if name != 'game_lab/warehouse_training_state_tests.cpp'] + ['game_lab/warehouse_transfer_tests.cpp']
WAREHOUSE_ROUTES = [name for name in WAREHOUSE_TRANSFER if name != 'game_lab/warehouse_transfer_tests.cpp'] + ['game_lab/warehouse_route_tests.cpp']
VIEWPORT = ['game_lab/map_viewport.hpp','game_lab/map_viewport_tests.cpp']
LAUNCH = ['game_lab/app_launch.hpp','game_lab/resource_root.hpp','game_lab/resource_root.cpp','game_lab/app_launch_tests.cpp']
CANDIDATE_IMPORT = ['game_lab/candidate_import.hpp', 'game_lab/candidate_import.cpp', 'game_lab/sg3.cpp', 'file_io/include/strict_zlib.h', 'file_io/include/framed_reader.hpp', 'file_io/include/owned_reader.hpp', 'file_io/include/city19_goal_archive.hpp', 'file_io/src/strict_zlib.cpp', 'file_io/src/framed_reader.cpp', 'game_lab/candidate_import_tests.cpp', 'game_lab/candidate_import_allocation_test_hooks.cpp']
TEXT_FEEDBACK = [name for name in APP if name != 'game_lab/app_media_integration_tests.mm'] + [
    'game_lab/text_load_feedback_tests.mm']
IMAGE_CONVERSION = ['game_lab/asset_limits.hpp', 'game_lab/cg_image_conversion.hpp',
                    'game_lab/cg_image_conversion_tests.cpp']
SURFACE = ['surface_lab/surface.hpp', 'surface_lab/surface.cpp',
           'surface_lab/oracle.hpp', 'surface_lab/oracle.cpp', 'surface_lab/tests.cpp']
METAL = [name for name in SURFACE if name != 'surface_lab/tests.cpp'] + [
    'metal_lab/backend.hpp', 'metal_lab/backend.mm', 'metal_lab/tests.mm']
SUITES = {'candidate-import': CANDIDATE_IMPORT, 'text-feedback': TEXT_FEEDBACK, 'image-conversion': IMAGE_CONVERSION, 'viewport': VIEWPORT, 'launch': LAUNCH, 'surface': SURFACE, 'metal': METAL,
          'city': CITY, 'service': SERVICE, 'scenario-state': SCENARIO_STATE, 'warehouse': WAREHOUSE,
          'warehouse-routes': WAREHOUSE_ROUTES, 'warehouse-transfer': WAREHOUSE_TRANSFER, 'warehouse-training-state': WAREHOUSE_TRAINING_STATE, 'data': DATA, 'media': MEDIA, 'integration': APP, 'app': APP,
          'movie-async': APP + ['game_lab/movie_session_async_tests.mm'],
          'movie-termination': APP + ['game_lab/movie_termination_tests.mm']}


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
    suite_sources = list(SUITES[suite])
    for name in sorted(set(CONTROL + suite_sources)):
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
        if suite in ('surface', 'metal'):
            compiled = ['surface_lab/surface.cpp', 'surface_lab/oracle.cpp']
            extra = []
            if suite == 'surface':
                compiled.append('surface_lab/tests.cpp')
            else:
                compiled += ['metal_lab/backend.mm', 'metal_lab/tests.mm']
                extra = ['-fobjc-arc', '-fno-modules', '-framework', 'Metal', '-framework', 'Foundation']
            command('compile-test', ['/usr/bin/clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                                     *flags, *extra, *[snapshot / name for name in compiled],
                                     '-o', report / 'graphics-tests'])
            binaries = {'graphics-tests': sha(report / 'graphics-tests')}
            fixtures = report / 'graphics-fixtures'
            fixtures.mkdir()
            command('graphics-test', [report / 'graphics-tests', fixtures])
            results = {suite: read(fixtures / 'summary.json')}
        elif suite in ('city', 'service', 'scenario-state', 'warehouse', 'warehouse-training-state', 'warehouse-transfer', 'warehouse-routes'):
            test_file = {'service':'game_lab/service_labor_tests.cpp', 'city':'game_lab/city_simulation_tests.cpp',
                         'scenario-state':'game_lab/city_game_state_tests.cpp', 'warehouse':'game_lab/warehouse_roads_tests.cpp',
                         'warehouse-training-state':'game_lab/warehouse_training_state_tests.cpp', 'warehouse-transfer':'game_lab/warehouse_transfer_tests.cpp','warehouse-routes':'game_lab/warehouse_route_tests.cpp'}[suite]
            command('compile-test', ['/usr/bin/clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                                     *flags, '-I', snapshot, snapshot / test_file,
                                     '-o', report / 'city-tests'])
            binaries = {'city-tests': sha(report / 'city-tests')}
            command('city-test', [report / 'city-tests', report / 'city-fixtures'])
            results = {suite: read(report / 'city-test.stdout')}
        elif suite == 'candidate-import':
            command('compile-test', ['/usr/bin/clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                                     *flags, snapshot / 'game_lab/candidate_import_tests.cpp',
                                     snapshot / 'game_lab/candidate_import.cpp', 'game_lab/sg3.cpp', snapshot / 'game_lab/candidate_import_allocation_test_hooks.cpp',
                                     snapshot / 'file_io/src/strict_zlib.cpp', snapshot / 'file_io/src/framed_reader.cpp',
                                     '-I', snapshot / 'file_io/include', '-lz', '-o', report / 'candidate-tests'])
            binaries = {'candidate-tests': sha(report / 'candidate-tests')}
            command('candidate-test', [report / 'candidate-tests'])
            results = {'candidate-import': read(report / 'candidate-test.stdout')}
        elif suite == 'image-conversion':
            command('compile-test', ['/usr/bin/clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                                     *flags, snapshot / 'game_lab/cg_image_conversion_tests.cpp',
                                     '-framework', 'CoreGraphics', '-framework', 'CoreFoundation',
                                     '-o', report / 'image-tests'])
            binaries = {'image-tests': sha(report / 'image-tests')}
            command('image-test', [report / 'image-tests'])
            results = {'image-conversion': read(report / 'image-test.stdout')}
        elif suite == 'viewport':
            command('compile-test', ['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,
                                     '-I',snapshot,snapshot/'game_lab/map_viewport_tests.cpp','-o',report/'viewport-tests'])
            binaries={'viewport-tests':sha(report/'viewport-tests')}
            command('viewport-test',[report/'viewport-tests'])
            results={'viewport':read(report/'viewport-test.stdout')}
        elif suite == 'launch':
            command('compile-test', ['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,
                                     '-I',snapshot,snapshot/'game_lab/app_launch_tests.cpp',
                                     snapshot/'game_lab/resource_root.cpp','-o',report/'launch-tests'])
            binaries={'launch-tests':sha(report/'launch-tests')}
            command('launch-test',[report/'launch-tests',report/'launch-fixtures'])
            results={'launch':read(report/'launch-test.stdout')}
        elif suite == 'data':
            command('compile-c', ['/usr/bin/clang', '-std=c11', '-Wall', '-Wextra', '-Werror', *flags,
                                 '-I', snapshot / 'runtime_core/include', '-c', snapshot / 'runtime_core/src/heap_native.c', '-o', report / 'heap_native.o'])
            command('compile-cpp', ['/usr/bin/clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror', *flags,
                                   '-I', snapshot / 'runtime_core/include', snapshot / 'runtime_core/tests/boundary_tests.cpp', report / 'heap_native.o', '-o', report / 'data-tests'])
            binaries = {name: sha(report / name) for name in ['heap_native.o', 'data-tests']}
            command('data-test', [report / 'data-tests'])
            command('tlv-test', ['/usr/bin/python3', '-I', '-B', snapshot / 'runtime_core/tests/state_codec_tests.py'])
            results = {'data': read(report / 'data-test.stdout'), 'tlv': read(report / 'tlv-test.stdout')}
        elif suite in ('media', 'integration', 'text-feedback', 'movie-async', 'movie-termination'):
            compiled = ['game_lab/sg3.cpp', 'game_lab/emperor_assets.mm', 'game_lab/wav_pcm.cpp', 'game_lab/asset_pixels.cpp',
                        'game_lab/resource_root.cpp', 'surface_lab/surface.cpp']
            extra_frameworks = []
            if suite == 'media':
                compiled.insert(0, 'game_lab/pure_boundary_tests.mm')
            else:
                compiled[:0] = [
                    ('game_lab/text_load_feedback_tests.mm' if suite == 'text-feedback' else
                     'game_lab/movie_session_async_tests.mm' if suite == 'movie-async' else
                     'game_lab/movie_termination_tests.mm' if suite == 'movie-termination' else
                     'game_lab/app_media_integration_tests.mm'), 'game_lab/app_media.cpp',
                    'game_lab/movie_session.mm',
                    'stream_lab/movie_io.mm', 'stream_lab/presenter.cpp',
                    'video_lab/stream.cpp', 'metal_lab/backend.mm',
                    'audio_lab/audio.cpp',
                    'media_demo/controller.cpp',
                ]
                extra_frameworks = [
                    '-framework', 'AVFoundation', '-framework', 'AudioToolbox',
                    '-framework', 'CoreMedia', '-framework', 'CoreVideo',
                    '-framework', 'Metal',
                ]
            command('compile-test', ['/usr/bin/clang++', '-std=c++17', '-fobjc-arc', '-fno-modules',
                                     '-Wall', '-Wextra', '-Werror', '-I', snapshot, '-I', snapshot / 'media_demo',
                                     *flags, *[snapshot / name for name in compiled],
                                     '-framework', 'Foundation', *extra_frameworks,
                                     '-framework', 'ImageIO', '-framework', 'CoreGraphics',
                                     '-lz', '-o', report / 'self-tests'])
            binaries = {'self-tests': sha(report / 'self-tests')}
            command('self-test', [report / 'self-tests', report / 'async-fixtures'] if suite in ('movie-async', 'movie-termination')
                    else [report / 'self-tests'])
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
    for name in ['fixtures', 'self-root', 'async-fixtures', 'city-fixtures', 'graphics-fixtures']:
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
