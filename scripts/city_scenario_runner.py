#!/usr/bin/env python3
"""Build/run authored scenarios without GUI, devices, media, or original-game inputs."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--core-directory', type=Path, required=True, help='Authored game_lab directory with published goal headers in app/')
    parser.add_argument('--expected-core-sha256', help='Optional explicit source pin; checked before compiling')
    parser.add_argument('--output', type=Path, required=True, help='New absolute evidence directory')
    parser.add_argument('--mode', choices=['release', 'undefined'], default='release')
    args = parser.parse_args()
    if not args.output.is_absolute() or args.output.exists():
        parser.error('--output must be a new absolute directory')
    core = args.core_directory.resolve()
    if args.expected_core_sha256 and sha(core / 'city_simulation.hpp') != args.expected_core_sha256:
        parser.error('Self-authored core source SHA-256 does not match requested pin')
    compiler = shutil.which('clang++')
    if not compiler:
        parser.error('Existing clang++ unavailable; no software will be installed')
    root = Path(__file__).resolve().parents[1]
    own = ['game_lab/app/city_scenario.hpp', 'game_lab/app/city_scenario_checkpoint.hpp', 'game_lab/city_scenario_tests.cpp']
    dependencies = ['city_simulation.hpp', 'emperor_assets.hpp', 'asset_limits.hpp', 'map_geometry.hpp',
                    'app/city_goal_feedback.hpp', 'app/city_goal_feedback_adapter.hpp']
    files = {path: root / path for path in own}
    files.update({f'game_lab/{path}': core / path for path in dependencies})
    contents = {path: source.read_bytes() for path, source in files.items()}
    args.output.mkdir(parents=True)
    sources = args.output / 'sources'
    rows = []
    for path, content in contents.items():
        dest = sources / path
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(content)
        rows.append({'path': path, 'sha256': sha(dest)})
    flags = ['-std=c++17', '-Wall', '-Wextra', '-Werror', '-pedantic']
    flags += ['-O2'] if args.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    report = {'mode': args.mode, 'expected_core_sha256': args.expected_core_sha256,
              'sources': rows, 'runner_sha256': sha(Path(__file__).resolve()),
              'fixtures': 'All self-authored clocks, metadata bytes, input snapshots and deterministic empty-city recipes are in test source.',
              'per_process_timeout_seconds': 60, 'steps': []}

    def run(command, name):
        start = time.monotonic()
        try:
            result = subprocess.run(command, text=True, capture_output=True, timeout=60)
            output, code = result.stdout + result.stderr, result.returncode
        except subprocess.TimeoutExpired as exc:
            output = 'TIMEOUT after 60 seconds\n' + (exc.stdout or b'').decode() + (exc.stderr or b'').decode()
            code = 124
        log = args.output / f'{name}.txt'
        log.write_text(output)
        report['steps'].append({'name': name, 'command': command, 'exit_code': code,
                                'seconds': round(time.monotonic() - start, 3), 'log_sha256': sha(log)})
        print(output, end='')
        return code == 0

    standalone = sources / 'standalone.cpp'
    standalone.write_text('#include "game_lab/app/city_scenario_checkpoint.hpp"\nint main(){game_lab::CityScenarioSession s;return s.active()?1:0;}\n')
    report['standalone_sha256'] = sha(standalone)
    ok = run([compiler, *flags, '-I', str(sources), '-fsyntax-only', str(standalone)], 'standalone-compile')
    binary = args.output / 'city-scenario-tests'
    if ok:
        ok = run([compiler, *flags, '-I', str(sources / 'game_lab'), str(sources / 'game_lab/city_scenario_tests.cpp'),
                  '-o', str(binary)], 'compile')
    if ok:
        report['binary_sha256'] = sha(binary)
        ok = run([str(binary)], 'tests')
    report['passed'] = ok
    (args.output / 'result.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
    print(f"{'PASS' if ok else 'FAIL'} receipt: {args.output / 'result.json'}")
    return 0 if ok else 1


if __name__ == '__main__':
    raise SystemExit(main())
