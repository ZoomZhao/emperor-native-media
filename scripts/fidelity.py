"""Fidelity check: run an earlier original save forward and compare with a later one.

Reads original data only through EMPEROR_GAMEDATA_ROOT and exits 0 with SKIP when
that is unset or empty. The build directory is always outside the data root.
"""
import argparse
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCES = [
    'game_lab/app/fidelity_tests.cpp',
    'game_lab/app/map_surface.cpp', 'game_lab/app/map_surface_preview.cpp',
    'game_lab/app/city_playback.cpp', 'game_lab/map_reference.cpp',
    'game_lab/original_map.cpp', 'game_lab/original_terrain.cpp', 'game_lab/original_ground.cpp',
    'game_lab/original_save.cpp', 'game_lab/original_pak.cpp', 'game_lab/sg3.cpp',
    'game_lab/resource_root.cpp',
]

def main():
    if not os.environ.get('EMPEROR_GAMEDATA_ROOT'):
        print('{"skipped":true,"reason":"EMPEROR_GAMEDATA_ROOT unset or empty"}')
        return 0
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=['release', 'undefined'], default='release')
    parser.add_argument('--build-dir', type=Path)
    args = parser.parse_args()
    data = Path(os.environ['EMPEROR_GAMEDATA_ROOT']).resolve()
    build = (args.build_dir or Path(tempfile.mkdtemp(prefix='emperor-fidelity-'))).resolve()
    if build.is_relative_to(data):
        parser.error('--build-dir must be outside the data root')
    build.mkdir(parents=True, exist_ok=True)
    binary = build / ('fidelity-' + args.mode)
    flags = ['-O2'] if args.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    subprocess.run(['/usr/bin/clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror', *flags,
                    *[str(ROOT / s) for s in SOURCES],
                    '-framework', 'CoreGraphics', '-framework', 'CoreFoundation', '-framework', 'Security',
                    '-lz', '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)], check=False)
    return result.returncode

if __name__ == '__main__':
    raise SystemExit(main())
