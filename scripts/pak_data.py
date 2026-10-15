"""Build native PAK authored tests or read-only original aggregate metrics."""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def run(args, build):
    build = build.resolve()
    selected = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if selected and Path(selected).is_absolute():
        data_root = Path(selected).resolve()
        if build == data_root or data_root in build.parents:
            raise ValueError('build directory must stay outside GameData')
    build.mkdir(parents=True, exist_ok=True)
    binary = build / ('pak-' + args.task + '-' + args.mode)
    flags = ['-O2'] if args.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    sources = ['game_lab/original_map.cpp', 'game_lab/original_pak.cpp', 'game_lab/resource_root.cpp',
               'game_lab/original_pak_tests.cpp' if args.task == 'test' else 'game_lab/original_pak_metrics.cpp']
    frameworks = [] if args.task == 'test' else ['-framework', 'CoreGraphics', '-framework', 'CoreFoundation']
    subprocess.run(['/usr/bin/clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror', *flags,
                    *[str(ROOT / x) for x in sources], *frameworks, '-lz', '-o', str(binary)], check=True)
    return subprocess.run([str(binary)], check=False).returncode

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('task', choices=['test', 'metrics'])
    p.add_argument('--mode', choices=['release', 'undefined'], default='release')
    p.add_argument('--build-dir', type=Path)
    a = p.parse_args()
    if a.task == 'metrics' and not os.environ.get('EMPEROR_GAMEDATA_ROOT'):
        print('{"skipped":true,"reason":"EMPEROR_GAMEDATA_ROOT unset or empty"}')
        return 0
    if a.build_dir:
        if not a.build_dir.is_absolute():
            p.error('--build-dir must be absolute')
        return run(a, a.build_dir)
    with tempfile.TemporaryDirectory(prefix='emperor-native-pak-') as directory:
        return run(a, Path(directory))

if __name__ == '__main__':
    raise SystemExit(main())
