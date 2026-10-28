#!/usr/bin/env python3
"""Build and verify the explicit C1 context contract, without running the original game."""
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
    parser.add_argument('--source', action='append')
    parser.add_argument('--skip-real', action='store_true')
    parser.add_argument('--build-dir', type=Path)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    if args.build_dir:
        if not args.build_dir.is_absolute():
            parser.error('--build-dir must be absolute and new')
        args.build_dir.mkdir(parents=True, exist_ok=False)
        output = args.build_dir
    else:
        evidence_root = Path('/tmp/house-calendar-c1-20261027')
        evidence_root.mkdir(parents=True, exist_ok=True)
        output = Path(tempfile.mkdtemp(prefix='native-city-' + args.mode + '-', dir=evidence_root))
    flags = ['-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-I' + str(repo / 'game_lab')]
    sanitizer = ['-fsanitize=undefined', '-fno-sanitize-recover=all'] if args.mode == 'undefined' else []
    names = subprocess.check_output(['git', 'ls-files', '--cached', '--others', '--exclude-standard'], cwd=repo, text=True).splitlines()
    before = {name: hashlib.sha256((repo / name).read_bytes()).hexdigest() for name in names
              if (repo / name).is_file() and Path(name).suffix in {'.hpp', '.h', '.inc', '.cpp', '.mm', '.py'}}
    jobs, dependencies, real = [], {}, []

    def run(command, label, *, env=None):
        with (output / (label + '.stdout')).open('x') as out, (output / (label + '.stderr')).open('x') as err:
            try:
                code = subprocess.run(command, stdout=out, stderr=err, env=env, timeout=900).returncode
            except subprocess.TimeoutExpired:
                code = 'timeout900'
        jobs.append({'name': label, 'argv': command, 'exit': code})
        print(json.dumps({'name': label, 'exit': code}), flush=True)
        if code != 0:
            raise RuntimeError(label + ' failed; retained logs in ' + str(output))

    def compile_source(name, label):
        obj, dep = output / (label + '.o'), output / (label + '.d')
        run(['/usr/bin/clang++', *flags, *sanitizer, '-MMD', '-MF', str(dep), '-c', str(repo / name), '-o', str(obj)], label + '-compile')
        for token in shlex.split(dep.read_text().replace('\\\n', ' '))[1:]:
            path = Path(token)
            if path.is_file() and repo in path.resolve().parents:
                dependencies[str(path.relative_to(repo))] = hashlib.sha256(path.read_bytes()).hexdigest()
        return obj

    def executable(name, label, helpers=()):
        obj = compile_source('game_lab/' + name, label)
        exe = output / label
        run(['/usr/bin/clang++', *sanitizer, str(obj), *map(str, helpers), *(['-lz'] if helpers else []), '-o', str(exe)], label + '-link')
        return exe

    skipped, error = None, None
    try:
        for file, label in [('native_city_context_tests.cpp', 'context-codec'),
                            ('original_house_goods_contract_tests.cpp', 'goods-contract'),
                            ('native_population_wire_tests.cpp', 'B-wire-compat')]:
            run([str(executable(file, label))], label)
        helpers = [compile_source('game_lab/' + name + '.cpp', name)
                   for name in ['original_map', 'original_terrain']]
        run([str(executable('native_city_contract_tests.cpp', 'city-contract', helpers))], 'city-contract')
        extra = [compile_source('game_lab/' + name + '.cpp', name)
                 for name in ['original_ground', 'original_save', 'original_pak', 'resource_root', 'sg3']]
        helpers += extra
        clean_env = dict(os.environ)
        clean_env.pop('EMPEROR_GAMEDATA_ROOT', None)
        run([str(executable('native_session_tests.cpp', 'legacy-core', helpers))], 'legacy-core', env=clean_env)
        data_root = os.environ.get('EMPEROR_GAMEDATA_ROOT', '')
        if args.skip_real or not data_root:
            skipped = 'explicit --skip-real' if args.skip_real else 'EMPEROR_GAMEDATA_ROOT unset'
            print(json.dumps({'SKIP': skipped, 'scope': 'real C1 source/model integration'}), flush=True)
        else:
            if not Path(data_root).is_absolute():
                raise RuntimeError('EMPEROR_GAMEDATA_ROOT must be absolute')
            sources = args.source or ['Save/2026102301/历史战役自动保存_1.sav']
            exe = executable('native_city_runtime_tests.cpp', 'city-runtime', helpers)
            for index, source in enumerate(sources):
                if Path(source).is_absolute() or '..' in Path(source).parts:
                    raise RuntimeError('source must be a relative child of the configured root')
                stage = output / ('source-' + str(index))
                stage.mkdir()
                run([str(exe), source, str(stage)], 'city-source-' + str(index))
                real.append(source)
    except (OSError, RuntimeError) as exc:
        error = str(exc)
    stable = all((repo / name).is_file() and hashlib.sha256((repo / name).read_bytes()).hexdigest() == value
                 for name, value in before.items())
    report = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'mode': args.mode,
              'passed': error is None and stable, 'source_stable': stable, 'source_sha256': before,
              'compiled_dependency_sha256': dependencies, 'jobs': jobs, 'real_sources_ran': real,
              'SKIP': skipped, 'error': error, 'GUI_executed': False, 'original_game_executed': False,
              'ASan': False, 'push': False}
    (output / 'verification.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
    print(json.dumps({'report': str(output / 'verification.json'), 'passed': report['passed']}), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
