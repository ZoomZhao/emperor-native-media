#!/usr/bin/env python3
"""Run authored warehouse training against a pinned published source snapshot; no App or devices."""
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
    parser.add_argument('--source-root', type=Path, required=True, help='Published self-authored repository text snapshot')
    parser.add_argument('--expected-core-sha256', required=True)
    parser.add_argument('--output', type=Path, required=True, help='New absolute task-owned evidence directory')
    parser.add_argument('--mode', choices=['release', 'undefined'], default='release')
    args = parser.parse_args()
    if not args.output.is_absolute() or args.output.exists():
        parser.error('--output must be a new absolute directory')
    source = args.source_root.resolve()
    if sha(source / 'game_lab/city_simulation.hpp') != args.expected_core_sha256:
        parser.error('Published self-authored core does not match explicit source pin')
    compiler = shutil.which('clang++')
    if not compiler:
        parser.error('Existing clang++ unavailable; no software installation')
    root = Path(__file__).resolve().parents[1]
    own = ['game_lab/app/city_warehouse_training.hpp', 'game_lab/city_warehouse_training_tests.cpp']
    dependencies = ['game_lab/city_simulation.hpp', 'game_lab/city_state_io.hpp', 'game_lab/city_game_state.hpp',
                    'game_lab/emperor_assets.hpp', 'game_lab/asset_limits.hpp', 'game_lab/map_geometry.hpp',
                    'game_lab/app/city_goal_feedback.hpp', 'game_lab/app/city_goal_feedback_adapter.hpp',
                    'game_lab/app/city_scenario.hpp', 'game_lab/app/city_scenario_checkpoint.hpp']
    inputs = {path: root / path for path in own}
    inputs.update({path: source / path for path in dependencies})
    content = {path: file.read_bytes() for path, file in inputs.items()}
    for path, file in inputs.items():
        if file.read_bytes() != content[path]:
            parser.error('Source changed while capturing snapshot: ' + path)
    args.output.mkdir(parents=True)
    snapshot = args.output / 'sources'
    rows = []
    for path, data in content.items():
        target = snapshot / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        rows.append({'path': path, 'sha256': sha(target)})
    flags = ['-std=c++17', '-Wall', '-Wextra', '-Werror', '-pedantic']
    flags += ['-O2'] if args.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    report = {'mode': args.mode, 'expected_core_sha256': args.expected_core_sha256, 'source_files': rows,
              'runner_sha256': sha(Path(__file__).resolve()), 'per_process_timeout_seconds': 60,
              'fixture_scope': 'Only deterministic authored empty dry-map builds, snapshots and corruption bytes in test source.',
              'steps': []}

    def run(command, name):
        start = time.monotonic()
        try:
            result = subprocess.run(command, text=True, capture_output=True, timeout=60)
            output, code = result.stdout + result.stderr, result.returncode
        except subprocess.TimeoutExpired as exc:
            output = 'TIMEOUT after 60 seconds\n' + (exc.stdout or b'').decode() + (exc.stderr or b'').decode()
            code = 124
        log = args.output / (name + '.txt')
        log.write_text(output)
        report['steps'].append({'name': name, 'command': command, 'exit_code': code,
                                'seconds': round(time.monotonic() - start, 3), 'log_sha256': sha(log)})
        print(output, end='')
        return code == 0

    standalone = snapshot / 'standalone.cpp'
    standalone.write_text('#include <app/city_warehouse_training.hpp>\nint main(){return game_lab::selfAuthoredWarehouseTraining().spec.id == 4 ? 0 : 1;}\n')
    report['standalone_sha256'] = sha(standalone)
    ok = run([compiler, *flags, '-I', str(snapshot / 'game_lab'), '-fsyntax-only', str(standalone)], 'standalone-compile')
    binary = args.output / 'city-warehouse-training-tests'
    if ok:
        ok = run([compiler, *flags, '-I', str(snapshot / 'game_lab'), str(snapshot / own[1]), '-o', str(binary)], 'compile')
    if ok:
        report['binary_sha256'] = sha(binary)
        ok = run([str(binary)], 'tests')
    report['passed'] = ok
    (args.output / 'result.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
    print(('PASS' if ok else 'FAIL') + ' receipt: ' + str(args.output / 'result.json'))
    return 0 if ok else 1


if __name__ == '__main__':
    raise SystemExit(main())
