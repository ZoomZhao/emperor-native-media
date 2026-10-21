"""City modulo clock tests (0x529990); authored only, no game data required."""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TEMP = Path('/private/var/folders/5n/nsp1d6sn0_76p_0f_j_dmj_c0000gn/T/opencode')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=['release', 'undefined'], default='release')
    parser.add_argument('--build-dir', type=Path)
    args = parser.parse_args()
    if args.build_dir and not args.build_dir.is_absolute():
        parser.error('absolute build directory required')
    build = (args.build_dir or Path(tempfile.mkdtemp(prefix='b-city-clocks-', dir=TEMP))).resolve()
    configured = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if configured and (build == Path(configured).resolve() or Path(configured).resolve() in build.parents):
        parser.error('build directory must stay outside GameData')
    build.mkdir(parents=True, exist_ok=True)
    flags = ['-O2'] if args.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    binary = build / ('city-clocks-' + args.mode)
    subprocess.run(['/usr/bin/clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror', *flags,
                    '-I', str(ROOT / 'game_lab'),
                    str(ROOT / 'game_lab' / 'original_city_clocks_tests.cpp'), '-o', str(binary)], check=True)
    return subprocess.run([str(binary)], check=False).returncode


if __name__ == '__main__':
    raise SystemExit(main())
