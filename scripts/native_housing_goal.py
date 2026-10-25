#!/usr/bin/env python3
"""Build/test the isolated native housing interval rule; never starts an App.

Real saves are read only through EMPEROR_GAMEDATA_ROOT with explicit --save
relative paths and a separately qualified --threshold. Outputs stay outside
this repository and include compiler-derived source/binary hash bindings.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--suite', choices=['interval', 'diagnostics'], default='interval')
    parser.add_argument('--mode', choices=['release', 'undefined'], default='release')
    parser.add_argument('--output-dir', type=Path)
    parser.add_argument('--threshold', type=int)
    parser.add_argument('--save', action='append', default=[])
    args = parser.parse_args()
    if args.save and (args.threshold is None or args.threshold <= 0):
        parser.error('--save requires a separately qualified positive --threshold')
    output = (args.output_dir or Path(tempfile.mkdtemp(prefix='emperor-housing-'))).resolve()
    if output == ROOT or ROOT in output.parents:
        parser.error('build outputs must stay outside the repository')
    output.mkdir(parents=True, exist_ok=True)
    flags = ['-std=c++17', '-Wall', '-Wextra', '-Werror', '-I' + str(ROOT / 'game_lab')]
    flags += ['-O2'] if args.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    test_source = 'native_housing_goal_tests' if args.suite == 'interval' else 'app/city_goal_diagnostics_tests'
    sources = [test_source, 'original_map', 'original_terrain', 'original_ground', 'original_save', 'original_pak', 'sg3', 'resource_root']
    commands, bindings, objects = [], {}, []
    for source in sources:
        stem = source.replace('/', '_')
        obj, dep = output / (stem + '.o'), output / (stem + '.d')
        cmd = ['/usr/bin/clang++', *flags, '-MMD', '-MF', str(dep), '-c', str(ROOT / 'game_lab' / (source + '.cpp')), '-o', str(obj)]
        subprocess.run(cmd, check=True); commands.append(cmd); objects.append(obj)
        dependencies = shlex.split(dep.read_text().replace('\\\n', ' ').split(':', 1)[1])
        for name in dependencies:
            path = Path(name).resolve()
            if ROOT in path.parents:
                bindings[str(path.relative_to(ROOT))] = sha(path)
    binary = output / ('housing-goal-tests' if args.suite == 'interval' else 'city-goal-diagnostics-tests')
    link = ['/usr/bin/clang++', *flags, *map(str, objects), '-framework', 'CoreGraphics', '-framework', 'CoreFoundation', '-lz', '-o', str(binary)]
    subprocess.run(link, check=True); commands.append(link)
    runs = []
    def run(argv, env=None):
        result = subprocess.run([str(binary), *argv], env=env, check=True, capture_output=True, text=True)
        if result.stderr:
            raise RuntimeError(result.stderr)
        rows = [json.loads(line) for line in result.stdout.splitlines()]
        runs.append({'arguments': argv, 'results': rows})
        for row in rows:
            print(json.dumps(row, ensure_ascii=False), flush=True)
    run([])
    no_data = dict(os.environ); no_data.pop('EMPEROR_GAMEDATA_ROOT', None)
    run(['--real'], no_data)
    assert 'SKIP' in runs[-1]['results'][0]
    inputs = {}
    root = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if args.save:
        if root:
            data_root = Path(root).resolve()
            for name in args.save:
                path = (data_root / name).resolve()
                if Path(name).is_absolute() or data_root not in path.parents:
                    raise ValueError('real save must be contained in the supplied root')
                inputs[name] = sha(path)
        run(['--real', str(args.threshold), *args.save])
        if root:
            assert all(sha(Path(root) / name) == digest for name, digest in inputs.items())
    assert all(sha(ROOT / name) == digest for name, digest in bindings.items()), 'source changed during verification'
    manifest = {'suite': args.suite, 'driver_sha256': sha(__file__), 'mode': args.mode, 'compiler': subprocess.check_output(['/usr/bin/clang++', '--version'], text=True),
                'commands': commands, 'sources_sha256': bindings,
                'objects_sha256': {obj.name: sha(obj) for obj in objects},
                'binary_sha256': sha(binary), 'runs': runs, 'real_inputs_sha256': inputs,
                'real_inputs_read_only_verified': bool(inputs), 'App_executed': False}
    (output / 'verification.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n')
    print('Bindings: ' + str(output / 'verification.json'))

if __name__ == '__main__':
    main()
