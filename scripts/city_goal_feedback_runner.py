#!/usr/bin/env python3
"""Compile/run only self-authored goal-feedback fixtures with existing clang++ (60s per process)."""
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
    parser.add_argument('--output', type=Path, required=True, help='New absolute evidence directory')
    parser.add_argument('--mode', choices=['release', 'undefined'], default='release')
    parser.add_argument('--harvest-rule', choices=['integer-v1', 'services-v1', 'bb00e6b'], required=True)
    parser.add_argument('--core-directory', type=Path, help='Directory with authored city_simulation.hpp; default game_lab')
    args = parser.parse_args()
    if not args.output.is_absolute() or args.output.exists():
        parser.error('--output must be a new absolute directory')
    root = Path(__file__).resolve().parents[1]
    core = args.core_directory.resolve() if args.core_directory else root / 'game_lab'
    expected_core = {'services-v1': '4a21f495f3353594cc34f20806d19ead8e851e71b874ab2f2a0586834b34874d', 'integer-v1': '9b5647534450e3a2c8c9b4818003093500bf7c6afc78cb0409126bd44658d3b4',
                     'bb00e6b': '14d30ff19edffc099d2de9c1880fb6d7806eee1552365d9a58c66fd9981769c5'}[args.harvest_rule]
    accepted_cores = [expected_core]
    if args.harvest_rule == 'integer-v1':
        accepted_cores.append('4a21f495f3353594cc34f20806d19ead8e851e71b874ab2f2a0586834b34874d') # Legacy ruleset still fully supported on this core.
    if args.harvest_rule in ('integer-v1', 'services-v1'):
        accepted_cores.append('d60e45ade98531edacfbfb4ac1e3ad3e03dd8e8ae1a08cdd811b97a6a1502ce0') # Optional v4 retains both published v1/v3 contracts.
    expected_core = sha(core / 'city_simulation.hpp')
    if expected_core not in accepted_cores:
        parser.error('Core SHA-256 does not match the explicitly selected tested rule; provide the correct frozen --core-directory')
    compiler = shutil.which('clang++')
    if not compiler:
        parser.error('Existing clang++ is unavailable; no software will be installed')
    # Exact text source allowlist. No media, original binary, GameData, GUI, or device code.
    files = {
        'game_lab/app/city_goal_feedback.hpp': root / 'game_lab/app/city_goal_feedback.hpp',
        'game_lab/app/city_goal_feedback_adapter.hpp': root / 'game_lab/app/city_goal_feedback_adapter.hpp',
        'game_lab/city_goal_feedback_tests.cpp': root / 'game_lab/city_goal_feedback_tests.cpp',
        **{f'game_lab/{name}': core / name for name in
           ['city_simulation.hpp', 'emperor_assets.hpp', 'asset_limits.hpp', 'map_geometry.hpp']},
    }
    contents = {name: source.read_bytes() for name, source in files.items()}
    args.output.mkdir(parents=True)
    sources = args.output / 'sources'
    rows = []
    for name, content in contents.items():
        dest = sources / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(content)
        rows.append({'path': name, 'sha256': sha(dest)})
    binary = args.output / 'city-goal-feedback-tests'
    flags = ['-std=c++17', '-Wall', '-Wextra', '-Werror', '-pedantic']
    flags += ['-O2'] if args.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    if args.harvest_rule == 'bb00e6b':
        flags += ['-DCITY_GOAL_TEST_LEGACY=1']
    report = {'mode': args.mode, 'harvest_rule': args.harvest_rule, 'sources': rows,
              'expected_core_sha256': expected_core,
              'runner_sha256': sha(Path(__file__).resolve()),
              'fixtures': 'Explicit self-authored snapshots and an empty authored city; test source contains all inputs.',
              'per_process_timeout_seconds': 60, 'steps': []}

    def run(command, name):
        started = time.monotonic()
        try:
            result = subprocess.run(command, capture_output=True, text=True, timeout=60)
            text = result.stdout + result.stderr
            code = result.returncode
        except subprocess.TimeoutExpired as exc:
            text = 'TIMEOUT after 60 seconds\n' + (exc.stdout or b'').decode() + (exc.stderr or b'').decode()
            code = 124
        log = args.output / f'{name}.txt'
        log.write_text(text)
        report['steps'].append({'name': name, 'command': command, 'exit_code': code,
                                'seconds': round(time.monotonic() - started, 3), 'log_sha256': sha(log)})
        print(text, end='')
        return code == 0

    if args.harvest_rule == 'services-v1':
        flags += ['-DCITY_GOAL_TEST_SERVICES_V1=1']

    # Verify the pure header compiles alone: it must not need the city model or frameworks.
    standalone = sources / 'standalone.cpp'
    standalone.write_text('#include "game_lab/app/city_goal_feedback.hpp"\nint main() { return game_lab::evaluateCityGoals({}).valid ? 0 : 1; }\n')
    report['standalone_fixture_sha256'] = sha(standalone)
    ok = run([compiler, *flags, '-I', str(sources), '-fsyntax-only', str(standalone)], 'standalone-compile')
    if ok:
        ok = run([compiler, *flags, '-I', str(sources / 'game_lab'),
                  str(sources / 'game_lab/city_goal_feedback_tests.cpp'), '-o', str(binary)], 'compile')
    if ok:
        report['binary_sha256'] = sha(binary)
        ok = run([str(binary)], 'tests')
    report['passed'] = ok
    (args.output / 'result.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
    print(f"{'PASS' if ok else 'FAIL'} receipt: {args.output / 'result.json'}")
    return 0 if ok else 1


if __name__ == '__main__':
    raise SystemExit(main())
