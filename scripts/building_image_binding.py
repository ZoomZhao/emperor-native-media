"""Exe-cited Well static-base binding and contained scene-preview checks.

Original-data checks read only EMPEROR_GAMEDATA_ROOT; unset/empty succeeds SKIP.
No pixels, hashes, source listings or original assets are written.
"""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def run(args, build):
    build = build.resolve()
    selected = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if selected:
        data = Path(selected).resolve()
        if build == data or data in build.parents:
            raise ValueError('build directory must stay outside GameData')
    build.mkdir(parents=True, exist_ok=True)
    binary = build / ('building-image-' + args.mode)
    flags = ['-O2'] if args.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    sources = ['original_building_image_tests.cpp', 'original_map.cpp', 'original_save.cpp',
               'original_pak.cpp', 'original_terrain.cpp', 'resource_root.cpp', 'sg3.cpp']
    subprocess.run(['/usr/bin/clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror', *flags,
                    *[str(ROOT / 'game_lab' / x) for x in sources], '-framework', 'CoreGraphics',
                    '-framework', 'CoreFoundation', '-lz', '-o', str(binary)], check=True)
    return subprocess.run([str(binary)], check=False).returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=['release', 'undefined'], default='release')
    parser.add_argument('--build-dir', type=Path)
    args = parser.parse_args()
    if args.build_dir:
        if not args.build_dir.is_absolute():
            parser.error('--build-dir must be absolute')
        return run(args, args.build_dir)
    with tempfile.TemporaryDirectory(prefix='emperor-building-image-') as directory:
        return run(args, Path(directory))


if __name__ == '__main__':
    raise SystemExit(main())
