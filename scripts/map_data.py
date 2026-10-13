"""Test authored MAP decoding or report aggregate original-data terrain metrics."""
from pathlib import Path
import argparse
import os
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('task', choices=['test', 'metrics'])
    parser.add_argument('--mode', choices=['release', 'undefined'], default='release')
    parser.add_argument('--build-dir', type=Path, default=ROOT / '.local' / 'map-build')
    args = parser.parse_args()
    if args.task == 'metrics' and not os.environ.get('EMPEROR_GAMEDATA_ROOT'):
        print('{"skipped":true,"reason":"EMPEROR_GAMEDATA_ROOT unset"}')
        return 0
    build = args.build_dir.resolve()
    build.mkdir(parents=True, exist_ok=True)
    binary = build / ('map-' + args.task + '-' + args.mode)
    flags = ['-O2'] if args.mode == 'release' else [
        '-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    sources = ['game_lab/original_map.cpp']
    frameworks = []
    if args.task == 'test':
        sources += ['game_lab/original_map_tests.cpp']
    else:
        sources += ['game_lab/original_map_metrics.cpp', 'game_lab/resource_root.cpp']
        frameworks = ['-framework', 'CoreGraphics', '-framework', 'CoreFoundation']
    subprocess.run(['/usr/bin/clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    *flags, *[str(ROOT / p) for p in sources], *frameworks,
                    '-lz', '-o', str(binary)], check=True)
    # This process owns its child and waits normally; no forced termination.
    return subprocess.run([str(binary)], check=False).returncode


if __name__ == '__main__':
    raise SystemExit(main())
