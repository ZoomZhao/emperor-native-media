"""Saved figure playback and scored forward selection; corpus uses EMPEROR_GAMEDATA_ROOT."""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--mode', choices=['release', 'undefined'], default='release')
    p.add_argument('--build-dir', type=Path)
    p.add_argument('--timeline-horizons', help='Comma-separated positive horizons, at most 10000 (default: regression corpus)')
    p.add_argument('--timeline-audio', choices=['off', 'on', 'both'], default='both')
    a = p.parse_args()
    horizons = None
    if a.timeline_horizons:
        try:
            horizons = sorted(set(int(value) for value in a.timeline_horizons.split(',')))
        except ValueError:
            p.error('timeline horizons must be comma-separated integers')
        if not horizons or horizons[0] < 1 or horizons[-1] > 10000:
            p.error('timeline horizons must be in 1..10000')
    if a.build_dir and not a.build_dir.is_absolute():
        p.error('absolute build directory required')
    build = (a.build_dir or Path(tempfile.mkdtemp(prefix='emperor-figure-scene-'))).resolve()
    configured = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if configured and (build == Path(configured).resolve() or Path(configured).resolve() in build.parents):
        p.error('build directory must stay outside GameData')
    build.mkdir(parents=True, exist_ok=True)
    binary = build / ('figure-scene-' + a.mode)
    flags = ['-O2'] if a.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    sources = ['original_figure_tests.cpp', 'app/map_surface.cpp', 'map_reference.cpp',
               'original_map.cpp', 'original_save.cpp', 'original_pak.cpp', 'original_terrain.cpp',
               'original_ground.cpp', 'resource_root.cpp', 'sg3.cpp']
    subprocess.run(['/usr/bin/clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror', *flags,
                    *[str(ROOT / 'game_lab' / s) for s in sources], '-framework', 'CoreGraphics',
                    '-framework', 'CoreFoundation', '-lz', '-o', str(binary)], check=True)
    command = [str(binary), '--timeline-audio', a.timeline_audio]
    if horizons:
        command += ['--timeline-horizons', ','.join(map(str, horizons))]
    return subprocess.run(command, check=False).returncode


if __name__ == '__main__':
    raise SystemExit(main())
