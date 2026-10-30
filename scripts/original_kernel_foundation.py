#!/usr/bin/env python3
"""Build kernel foundation/legacy regressions outside the repo; original data is process-local and optional."""
import argparse
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--mode', choices=['release', 'undefined'], default='release')
    p.add_argument('--build-dir', type=Path, required=True)
    p.add_argument('--real-playback', action='store_true')
    p.add_argument('--source', action='append', default=[], help='Explicit relative SAV for App playback')
    p.add_argument('--market-source', action='append', default=[], help='Explicit relative SAV for known-base roundtrip')
    a = p.parse_args()
    if a.real_playback and not a.source:
        p.error('--real-playback requires explicit --source entries')
    root = Path(__file__).resolve().parents[1]
    build = a.build_dir
    if not build.is_absolute() or root == build or root in build.parents:
        p.error('absolute build directory outside source required')
    data = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if data and (Path(data).resolve() == build.resolve() or Path(data).resolve() in build.resolve().parents):
        p.error('build must stay outside GameData')
    build.mkdir(parents=True, exist_ok=False)
    flags = ['-std=c++17', '-Wall', '-Wextra', '-Werror', '-O2']
    if a.mode == 'undefined':
        flags += ['-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    jobs = []
    sources = sorted(root.glob('game_lab/**/*.hpp')) + sorted(root.glob('game_lab/**/*.cpp')) + sorted(root.glob('file_io/include/*.hpp'))
    hashes = {str(f.relative_to(root)): hashlib.sha256(f.read_bytes()).hexdigest() for f in sources}
    env = dict(os.environ)
    env.pop('EMPEROR_GAMEDATA_ROOT', None)

    def run(argv, label, environment=env):
        started = datetime.now(timezone.utc).isoformat()
        with (build / (label + '.stdout')).open('x') as out, (build / (label + '.stderr')).open('x') as err:
            r = subprocess.run(list(map(str, argv)), env=environment, stdout=out, stderr=err, timeout=1200)
        jobs.append({'name': label, 'argv': list(map(str, argv)), 'exit': r.returncode, 'started_utc': started,
                     'finished_utc': datetime.now(timezone.utc).isoformat()})
        print(json.dumps({'name': label, 'exit': r.returncode}), flush=True)
        if r.returncode:
            raise RuntimeError(label + ' failed: ' + (build / (label + '.stderr')).read_text()[-2000:])

    def compile_file(name):
        obj = build / (name.replace('/', '_') + '.o')
        run(['/usr/bin/clang++', *flags, '-c', root / 'game_lab' / (name + '.cpp'), '-o', obj], name.replace('/', '_') + '-compile')
        return obj

    helpers = ['original_map', 'original_terrain', 'original_ground', 'original_save', 'original_pak', 'resource_root', 'sg3', 'map_reference',
               'app/map_surface', 'app/map_surface_preview', 'app/city_playback']
    tests = ['original_kernel_contract_tests', 'original_market_base_codec_tests', 'original_building_update_tests', 'original_market_transfer_tests',
             'original_house_goods_contract_tests', 'native_city_context_tests', 'native_population_wire_tests', 'native_city_contract_tests',
             'native_session_tests', 'original_figure_tests', 'app/city_notifications_tests', 'app/map_surface_tests']
    error = None
    try:
        with ThreadPoolExecutor(max_workers=2) as pool:
            objects = dict(zip(helpers + tests, pool.map(compile_file, helpers + tests)))
        for name in tests:
            label = name.replace('/', '_')
            binary = build / label
            run(['/usr/bin/clang++', *flags, objects[name], *[objects[h] for h in helpers], '-framework', 'CoreGraphics', '-framework', 'CoreFoundation', '-lz', '-o', binary], label + '-link')
            argv = [binary]
            if name == 'app/map_surface_tests':
                fixture = build / 'authored-fixture'
                fixture.mkdir()
                argv.append(fixture)
            run(argv, label)
        run([build / 'original_figure_tests', '--timeline-mode', 'combined', '--city-clocks', '0,0,0,0,0,0,0,0,0,0'], 'combined-authored')
        # Existing v1..v6/default regression assertions run above, never rewritten.
        if a.real_playback and data:
            run([build / 'app_map_surface_tests', '--playback', *a.source], 'real-App-playback', dict(os.environ))
        else:
            print(json.dumps({'SKIP': 'real App playback not requested or EMPEROR_GAMEDATA_ROOT unset'}), flush=True)
        if a.market_source and data:
            run([build / 'original_market_base_codec_tests', *a.market_source], 'real-market-base', dict(os.environ))
    except (OSError, RuntimeError, subprocess.SubprocessError) as e:
        error = str(e)
    stable = all((root / f).is_file() and hashlib.sha256((root / f).read_bytes()).hexdigest() == h for f, h in hashes.items())
    report = {'mode': a.mode, 'passed': error is None and stable, 'error': error, 'source_stable': stable, 'source_sha256': hashes, 'jobs': jobs,
              'real_App_requested': a.real_playback, 'real_App_ran': any(j['name'] == 'real-App-playback' for j in jobs),
              'original_game_executed': False, 'GUI_executed': False, 'ASan': False}
    (build / 'verification.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
    print(json.dumps({'report': str(build / 'verification.json'), 'passed': report['passed'], 'error': error}), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
