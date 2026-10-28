#!/usr/bin/env python3
"""Authored Market transfer/figure regressions. Never reads original game data."""
from pathlib import Path
import argparse
import datetime
import hashlib
import json
import os
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--mode', choices=('release', 'undefined'), default='release')
    p.add_argument('--output-dir', type=Path, required=True)
    a = p.parse_args()
    out = a.output_dir.resolve()
    data = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    data_root = Path(data).resolve() if data else None
    if (not a.output_dir.is_absolute() or out.exists() or out == ROOT or ROOT in out.parents
            or (data_root and (out == data_root or data_root in out.parents))):
        p.error('new absolute output outside repository and original data required')
    out.mkdir(parents=True)
    report = {'suite': 'market-transfer-conservation', 'mode': a.mode, 'passed': False,
              'started_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'driver_sha256': sha(__file__), 'commands': [], 'sources_sha256': {},
              'jobs': [], 'original_data': False, 'original_capacity_claim': False,
              'GUI': False, 'ASan': False}
    env = dict(os.environ)
    env.pop('EMPEROR_GAMEDATA_ROOT', None)
    env['PYTHONDONTWRITEBYTECODE'] = '1'

    def save():
        (out/'verification.json').write_text(json.dumps(report, indent=2)+'\n')

    def run(command, label, timeout=300):
        report['commands'].append(command)
        save()
        with (out/(label+'.stdout')).open('xb') as stdout, (out/(label+'.stderr')).open('xb') as stderr:
            result = subprocess.run(command, cwd=ROOT, env=env, stdout=stdout,
                                    stderr=stderr, timeout=timeout)
        report['jobs'].append({'label': label, 'exit': result.returncode})
        save()
        print(json.dumps({'label': label, 'exit': result.returncode}), flush=True)
        if result.returncode:
            raise RuntimeError(label+' failed; logs retained')

    flags = ['-std=c++17', '-Wall', '-Wextra', '-Werror', '-I'+str(ROOT/'game_lab')]
    flags += ['-O2'] if a.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    helpers = ['app/map_surface', 'map_reference', 'original_map', 'original_save',
               'original_pak', 'original_terrain', 'original_ground', 'resource_root', 'sg3']
    tests = ['original_market_transfer_tests', 'original_house_goods_contract_tests', 'original_figure_tests']
    objects = {}
    try:
        for name in tests+helpers:
            label = name.replace('/', '-')
            obj, dep = out/(label+'.o'), out/(label+'.d')
            run(['/usr/bin/clang++', *flags, '-MMD', '-MF', str(dep), '-c',
                 str(ROOT/'game_lab'/(name+'.cpp')), '-o', str(obj)], 'compile-'+label)
            objects[name] = obj
            for token in shlex.split(dep.read_text().replace('\\\n', ' ').split(':', 1)[1]):
                path = Path(token).resolve()
                if ROOT in path.parents:
                    rel = path.relative_to(ROOT).as_posix()
                    digest = sha(path)
                    if rel in report['sources_sha256'] and report['sources_sha256'][rel] != digest:
                        raise RuntimeError('source changed during compilation: '+rel)
                    report['sources_sha256'][rel] = digest
        for name in tests:
            exe = out/name
            run(['/usr/bin/clang++', *flags, str(objects[name]),
                 *[str(objects[n]) for n in helpers], '-framework', 'CoreGraphics',
                 '-framework', 'CoreFoundation', '-lz', '-o', str(exe)], 'link-'+name)
            args = ['--timeline-mode', 'combined', '--city-clocks', ','.join(['0']*10)] if name == 'original_figure_tests' else []
            run([str(exe), *args], 'run-'+name, timeout=900)
            report.setdefault('binaries_sha256', {})[name] = sha(exe)
        report['objects_sha256'] = {name: sha(path) for name, path in objects.items()}
        report['passed'] = True
    finally:
        report['source_stable'] = all(sha(ROOT/f) == h for f, h in report['sources_sha256'].items())
        report['passed'] = report['passed'] and report['source_stable']
        report['finished_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        save()
    print(json.dumps({'passed': report['passed'], 'report': str(out/'verification.json')}))


if __name__ == '__main__':
    main()
