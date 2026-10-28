#!/usr/bin/env python3
"""Reproduce the bounded native population contract; never writes game data."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=['release', 'undefined'], default='release')
    parser.add_argument('--source', action='append', help='repeat for relative admitted SAV paths')
    parser.add_argument('--skip-real', action='store_true')
    parser.add_argument('--build-dir', type=Path, help='new output directory; must not exist')
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    if args.build_dir:
        if not args.build_dir.is_absolute():
            parser.error('--build-dir must be absolute')
        args.build_dir.mkdir(parents=True, exist_ok=False)
        output = args.build_dir
    else:
        output = Path(tempfile.mkdtemp(prefix='emperor-native-population-'))
    flags = ['-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-I' + str(repo / 'game_lab')]
    sanitizer = ['-fsanitize=undefined', '-fno-sanitize-recover=all'] if args.mode == 'undefined' else []
    rows = []
    hashes = {}

    def run(command, label):
        with (output / (label + '.stdout')).open('x') as out, (output / (label + '.stderr')).open('x') as err:
            try:
                result = subprocess.run(command, stdout=out, stderr=err, timeout=900)
                code = result.returncode
            except subprocess.TimeoutExpired:
                code = 'timeout900'
        rows.append({'name': label, 'argv': command, 'exit': code})
        print(json.dumps({'name': label, 'exit': code}), flush=True)
        if code != 0:
            raise RuntimeError(label + ' failed; see ' + str(output))

    def compile_object(source, name):
        obj = output / (name + '.o')
        dependencies = output / (name + '.d')
        run(['/usr/bin/clang++', *flags, *sanitizer, '-MMD', '-MF', str(dependencies), '-c', str(repo / source), '-o', str(obj)], name + '-compile')
        for token in shlex.split(dependencies.read_text().replace('\\\n', ' '))[1:]:
            path = Path(token)
            if path.is_file() and repo in path.resolve().parents:
                hashes[str(path.relative_to(repo))] = hashlib.sha256(path.read_bytes()).hexdigest()
        return obj

    def executable(source, name, helpers=()):
        obj = compile_object(source, name)
        exe = output / name
        run(['/usr/bin/clang++', *sanitizer, str(obj), *map(str, helpers), *(['-lz'] if helpers else []), '-o', str(exe)], name + '-link')
        return exe

    skipped = None
    real = []
    error = None
    try:
        for file, label in [('native_immigrant_codec_tests.cpp', 'immigrant-codec'),
                            ('native_immigrant_arrival_tests.cpp', 'arrival-proof'),
                            ('native_population_wire_tests.cpp', 'population-wire')]:
            exe = executable('game_lab/' + file, label)
            run([str(exe)], label)
        root = os.environ.get('EMPEROR_GAMEDATA_ROOT', '')
        if args.skip_real or not root:
            skipped = 'real integration explicitly skipped' if args.skip_real else 'EMPEROR_GAMEDATA_ROOT unset'
            print(json.dumps({'SKIP': skipped, 'scope': 'real source integration'}), flush=True)
        else:
            if not Path(root).is_absolute():
                raise RuntimeError('EMPEROR_GAMEDATA_ROOT must be absolute')
            sources = args.source or ['Save/2026102301/历史战役自动保存_1.sav']
            for source in sources:
                if Path(source).is_absolute() or '..' in Path(source).parts:
                    raise RuntimeError('source must be a relative child of the configured data root')
            helpers = [compile_object('game_lab/' + name + '.cpp', name) for name in
                       ['original_map', 'original_terrain', 'original_ground', 'original_save', 'original_pak', 'resource_root', 'sg3']]
            exe = executable('game_lab/native_population_runtime_tests.cpp', 'population-runtime', helpers)
            for index, source in enumerate(sources):
                run([str(exe), source], 'population-source-' + str(index))
                real.append(source)
    except (OSError, RuntimeError) as exc:
        error = str(exc)
    stable = all(hashlib.sha256((repo / f).read_bytes()).hexdigest() == digest for f, digest in hashes.items())
    report = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'mode': args.mode,
              'passed': error is None and stable, 'source_stable': stable, 'source_sha256': hashes,
              'commands': rows, 'real_sources_ran': real, 'SKIP': skipped, 'error': error,
              'GUI_executed': False, 'original_game_executed': False, 'ASan': False}
    (output / 'verification.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'report': str(output / 'verification.json'), 'passed': report['passed']}), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
