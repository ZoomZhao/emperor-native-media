"""SAV19 seeded ground reconstruction; only aggregate original-data metrics."""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=['release', 'undefined'], default='release')
    parser.add_argument('--build-dir', type=Path)
    args = parser.parse_args()
    if args.build_dir and not args.build_dir.is_absolute():
        parser.error('absolute build directory required')
    build = (args.build_dir or Path(tempfile.mkdtemp(prefix='emperor-sav-ground-'))).resolve()
    configured = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if configured and (build == Path(configured).resolve() or Path(configured).resolve() in build.parents):
        parser.error('build must stay outside GameData')
    build.mkdir(parents=True, exist_ok=True)
    binary = build / ('saved-ground-' + args.mode)
    flags = ['-O2'] if args.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    sources = ['original_saved_ground_tests.cpp', 'app/map_surface.cpp', 'map_reference.cpp',
               'original_map.cpp', 'original_save.cpp', 'original_pak.cpp', 'original_terrain.cpp',
               'original_ground.cpp', 'resource_root.cpp', 'sg3.cpp']
    subprocess.run(['/usr/bin/clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror', *flags,
                    *[str(ROOT / 'game_lab' / source) for source in sources], '-framework', 'CoreGraphics',
                    '-framework', 'CoreFoundation', '-lz', '-o', str(binary)], check=True)
    return subprocess.run([str(binary)], check=False).returncode


if __name__ == '__main__':
    raise SystemExit(main())
