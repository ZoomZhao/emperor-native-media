#!/usr/bin/env python3
"""Build only the independent, authored food API composition suite.

No CMake changes, assets, original game, network or saved-game reads.
All generated artifacts must be outside the repository.
"""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--mode', choices=('release', 'undefined', 'both'), default='both')
    parser.add_argument('--compiler', default='clang++')
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    if output == repo or repo in output.parents:
        parser.error('generated artifacts must be outside the repository')
    output.mkdir(parents=True, exist_ok=True)
    source = repo / 'game_lab/original_food_contract_composition_tests.cpp'
    common = [args.compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-I' + str(repo / 'game_lab')]
    modes = ('release', 'undefined') if args.mode == 'both' else (args.mode,)
    all_ok = True
    for mode in modes:
        directory = output / mode
        directory.mkdir(exist_ok=True)
        log = {'started_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
               'mode': mode, 'commands': [], 'authored_conditional_api_composition_only': True,
               'original_gameplay_tested': False, 'passed': False}
        def run(command, label, env=None):
            log['commands'].append(command)
            result = subprocess.run(command, cwd=repo, env=env, capture_output=True, text=True, timeout=180)
            (directory / (label + '.stdout')).write_text(result.stdout)
            (directory / (label + '.stderr')).write_text(result.stderr)
            if result.returncode:
                raise RuntimeError(f'{label} exit {result.returncode}; see {directory}')
            return result
        try:
            # A preflight include graph gives hashes BEFORE compiling, rather than
            # retrospectively binding whatever shared headers exist afterwards.
            dep = directory / 'preflight.d'
            run(common + ['-MM', '-MT', 'food_contract', '-MF', str(dep), str(source)], 'dependencies')
            names = shlex.split(dep.read_text().replace('\\\n', '').partition(':')[2])
            inputs = {str((repo / name).resolve()): digest((repo / name).resolve()) for name in names}
            for extra in [Path(__file__).resolve(), repo / 'docs/FOOD-CONTRACT-TESTS.md']:
                inputs[str(extra)] = digest(extra)
            flags = ['-O2'] if mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
            binary = directory / 'original_food_contract_composition_tests'
            run(common + flags + [str(source), '-o', str(binary)], 'build')
            env = os.environ.copy()
            env.pop('EMPEROR_GAMEDATA_ROOT', None)
            if mode == 'undefined':
                env['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
            result = run([str(binary)], 'run', env)
            payload = json.loads(result.stdout)
            changed = [path for path, old in inputs.items() if digest(path) != old]
            log.update(source_hashes=inputs, source_stable=not changed, changed_inputs=changed,
                       binary_sha256=digest(binary), result=payload)
            if changed:
                raise RuntimeError('source changed during compile/run; do not claim a stable binding')
            if not payload.get('passed'):
                raise RuntimeError('suite did not report success')
            log['passed'] = True
        except (OSError, RuntimeError, subprocess.TimeoutExpired, ValueError) as error:
            all_ok = False
            log['error'] = str(error)
        log['finished_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        (directory / 'verification.json').write_text(json.dumps(log, indent=2) + '\n')
        print(json.dumps({'mode': mode, 'passed': log['passed'], 'result': log.get('result'),
                          'error': log.get('error'), 'verification': str(directory / 'verification.json')}), flush=True)
    return 0 if all_ok else 1


if __name__ == '__main__':
    sys.exit(main())
