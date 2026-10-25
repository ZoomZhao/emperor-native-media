#!/usr/bin/env python3
"""Native v6 labor tests. Real data only via EMPEROR_GAMEDATA_ROOT; unset is SKIP.
Builds outside the repository and binds every repository source and real input by SHA-256.
"""
from pathlib import Path
import argparse, hashlib, json, os, shlex, subprocess
ROOT = Path(__file__).resolve().parents[1]
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--mode', choices=['release', 'undefined'], default='release')
    p.add_argument('--output-dir', type=Path, required=True)
    p.add_argument('--save', action='append', default=[], help='relative SAV under EMPEROR_GAMEDATA_ROOT')
    a = p.parse_args(); out = a.output_dir.resolve()
    if out == ROOT or ROOT in out.parents: p.error('output must be outside repository')
    out.mkdir(parents=True, exist_ok=True)
    flags = ['-std=c++17', '-Wall', '-Wextra', '-Werror', '-I' + str(ROOT / 'game_lab')]
    flags += ['-O2'] if a.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    shared = ['original_map', 'original_terrain', 'original_ground', 'original_save', 'original_pak', 'sg3', 'resource_root']
    bindings, binaries, commands = {}, {}, []
    def compile_unit(name):
        obj = out / (name + '.o'); dep = out / (name + '.d')
        cmd = ['/usr/bin/clang++', *flags, '-MMD', '-MF', str(dep), '-c', str(ROOT / 'game_lab' / (name + '.cpp')), '-o', str(obj)]
        subprocess.run(cmd, check=True); commands.append(cmd)
        for item in shlex.split(dep.read_text().replace('\\\n', ' ').split(':', 1)[1]):
            f = Path(item).resolve()
            if ROOT in f.parents: bindings[str(f.relative_to(ROOT))] = sha(f)
        return obj
    objects = [compile_unit(n) for n in shared]
    for test in ['native_labor_tests', 'native_v6_rules_tests']:
        binary = out / test.replace('_', '-')
        cmd = ['/usr/bin/clang++', *flags, str(compile_unit(test)), *map(str, objects), '-framework', 'CoreGraphics', '-framework', 'CoreFoundation', '-lz', '-o', str(binary)]
        subprocess.run(cmd, check=True); commands.append(cmd); binaries[test] = binary
    runs = []
    def run(binary, args, env=None):
        x = subprocess.run([str(binary), *args], env=env, capture_output=True, text=True)
        rows = [json.loads(line) for line in x.stdout.splitlines() if line.startswith('{')]
        runs.append({'binary': binary.name, 'arguments': args, 'exit': x.returncode, 'stderr': x.stderr, 'results': rows})
        print(x.stdout, end='', flush=True)
        if x.returncode or x.stderr: raise SystemExit(f'{binary.name} {args} failed: exit {x.returncode} {x.stderr}')
    run(binaries['native_labor_tests'], []); run(binaries['native_v6_rules_tests'], [])
    clean = dict(os.environ); clean.pop('EMPEROR_GAMEDATA_ROOT', None)
    run(binaries['native_labor_tests'], ['--real', *a.save], clean); assert 'SKIP' in runs[-1]['results'][0]
    root = os.environ.get('EMPEROR_GAMEDATA_ROOT'); inputs = {}
    if root and a.save:
        names = ['Model/EmperorBuildingModels.txt', *a.save]
        inputs = {n: sha(Path(root) / n) for n in names}
        run(binaries['native_labor_tests'], ['--real', *a.save])
        assert all(sha(Path(root) / n) == v for n, v in inputs.items()), 'real input changed'
    assert all(sha(ROOT / n) == v for n, v in bindings.items()), 'source changed during run'
    result = {'suite': 'native-v6-labor', 'mode': a.mode, 'driver_sha256': sha(__file__), 'sources_sha256': bindings,
              'binaries_sha256': {k: sha(v) for k, v in binaries.items()}, 'commands': commands, 'runs': runs,
              'real_inputs_sha256': inputs, 'App_executed': False}
    (out / 'verification.json').write_text(json.dumps(result, indent=2, ensure_ascii=False) + '\n')
if __name__ == '__main__': main()
