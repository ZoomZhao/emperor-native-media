"""Building update pass tests; real inputs read only EMPEROR_GAMEDATA_ROOT."""
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
    parser.add_argument('--bite', action='store_true')
    parser.add_argument('--bite-kind', choices=['crop', 'industry', 'road', 'accounting'], default='crop')
    args = parser.parse_args()
    if args.build_dir and not args.build_dir.is_absolute():
        parser.error('absolute build directory required')
    build = (args.build_dir or Path(tempfile.mkdtemp(prefix='b-building-update-', dir=TEMP))).resolve()
    configured = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if configured and (build == Path(configured).resolve() or Path(configured).resolve() in build.parents):
        parser.error('build directory must stay outside GameData')
    build.mkdir(parents=True, exist_ok=True)
    sources = ['original_building_update_tests.cpp', 'original_map.cpp', 'original_save.cpp',
               'original_pak.cpp', 'original_terrain.cpp', 'original_ground.cpp', 'map_reference.cpp', 'resource_root.cpp', 'sg3.cpp']
    flags = ['-O2'] if args.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    test_source = ROOT / 'game_lab' / sources[0]
    if args.bite:
        # Mutate only private authored source. Original bytes/listings never copied.
        copied = build / 'bite'
        copied.mkdir(exist_ok=True)
        header = (ROOT / 'game_lab/original_building_update.hpp').read_text()
        mutations = {
            'crop': ('low>32?', 'low>=32?', 'literal per-type growth uses LOW byte32 threshold'),
            'industry': ('?5:10)', '?5:9)', 'industry workforce100 percent produces10 with ordinary divisor10'),
            'road': ('if(r<bestRank)', 'if(r<=bestRank)', 'equal network rank preserves clockwise first candidate'),
            'accounting': ('if(r.priority>5)r.priority=3', 'if(r.priority>5)r.priority=4', 'out-of-range positive priority normalizes to3 before demand test'),
        }
        before, after, assertion = mutations[args.bite_kind]
        assert header.count(before) == 1
        (copied / 'original_building_update.hpp').write_text(header.replace(before, after, 1))
        test_source = copied / sources[0]
        test_source.write_text((ROOT / 'game_lab' / sources[0]).read_text())
    binary = build / ('building-update-' + args.mode + ('-bite' if args.bite else ''))
    subprocess.run(['/usr/bin/clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror', *flags,
                    '-I', str(ROOT / 'game_lab'), str(test_source),
                     *[str(ROOT / 'game_lab' / s) for s in sources[1:]], '-framework', 'CoreGraphics', '-framework', 'CoreFoundation', '-lz', '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)], check=False, capture_output=args.bite, text=args.bite)
    if args.bite:
        if result.returncode == 0:
            raise RuntimeError('bite mutation survived')
        if assertion not in result.stderr:
            raise RuntimeError('bite failed outside the intended assertion: ' + result.stderr)
        print(result.stderr.strip())
        print('{"bite_detected":true,"mutation":"' + args.bite_kind + '"}')
        return 0
    return result.returncode


if __name__ == '__main__':
    raise SystemExit(main())
