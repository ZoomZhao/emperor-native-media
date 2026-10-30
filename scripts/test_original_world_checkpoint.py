#!/usr/bin/env python3
"""Build and exercise original-world disk checkpoints outside source/GameData.

Only explicitly named SAV inputs are read, through a per-process optional root.
This program never launches the original game or a GUI and never uses ASan.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--mode', choices=['release', 'undefined'], default='release')
    p.add_argument('--build-dir', type=Path, required=True)
    p.add_argument('--source', action='append', default=[])
    p.add_argument('--base-source', action='append', default=[])
    p.add_argument('--retired-source', action='append', default=[])
    p.add_argument('--legacy', action='store_true')
    a = p.parse_args()
    repo = Path(__file__).resolve().parents[1]
    build = a.build_dir.resolve()
    data = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if not a.build_dir.is_absolute() or build == repo or repo in build.parents:
        p.error('absolute, fresh build directory outside source required')
    if data and (build == Path(data).resolve() or Path(data).resolve() in build.parents):
        p.error('outputs must remain outside GameData')
    build.mkdir(parents=True, exist_ok=False)
    flags = ['-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror']
    if a.mode == 'undefined':
        flags += ['-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    env = dict(os.environ)
    env.pop('EMPEROR_GAMEDATA_ROOT', None)
    if a.mode == 'undefined':
        env['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
    jobs = []
    # Includes inline units and the exact frozen wire-schema, plus UI consumer.
    sources = [f for folder in ['game_lab', 'file_io/include'] for f in (repo / folder).rglob('*')
               if f.is_file() and f.suffix in ('.cpp', '.hpp', '.inc', '.mm', '.py', '.sh')]
    sources += [Path(__file__).resolve()]
    hashes = {str(f.relative_to(repo)): hashlib.sha256(f.read_bytes()).hexdigest() for f in sources}

    def run(argv, label, environment=env):
        start = datetime.now(timezone.utc).isoformat()
        with (build / (label + '.stdout')).open('x') as out, (build / (label + '.stderr')).open('x') as err:
            result = subprocess.run(list(map(str, argv)), cwd=repo, env=environment, stdout=out, stderr=err, timeout=1200)
        jobs.append(dict(name=label, argv=list(map(str, argv)), exit=result.returncode,
                         started_utc=start, finished_utc=datetime.now(timezone.utc).isoformat()))
        print(json.dumps({'name': label, 'exit': result.returncode}), flush=True)
        if result.returncode:
            raise RuntimeError(label + ': ' + (build / (label + '.stderr')).read_text()[-3000:])

    def compile_file(name):
        obj = build / (name.replace('/', '_') + '.o')
        run(['/usr/bin/clang++', *flags, '-c', repo / 'game_lab' / (name + '.cpp'), '-o', obj], name.replace('/', '_') + '-compile')
        return obj

    error = None
    helpers = ['original_map', 'original_terrain', 'original_ground', 'original_save', 'original_pak',
               'resource_root', 'sg3', 'map_reference', 'app/map_surface', 'app/map_surface_preview', 'app/city_playback']
    tests = ['original_world_checkpoint_codec_tests', 'original_world_checkpoint_io_tests', 'original_world_checkpoint_composition_tests', 'app/original_world_checkpoint_tests']
    if a.legacy:
        tests += ['original_kernel_contract_tests', 'original_market_base_codec_tests', 'original_building_update_tests',
                  'original_market_transfer_tests', 'original_house_goods_contract_tests', 'native_city_context_tests',
                  'native_population_wire_tests', 'native_city_contract_tests', 'native_session_tests', 'original_figure_tests',
                  'app/city_notifications_tests', 'app/map_surface_tests', 'original_food_contract_composition_tests']
    try:
        # Compare declarations obtained from current source, not merely encode
        # and decode visitors agreeing with each other about omitted members.
        inspect = build / 'coverage.cpp'
        inspect.write_text('#include "' + str(repo / 'game_lab/original_world_checkpoint_codec.hpp') + '"\n')
        run(['/usr/bin/clang++', '-std=c++17', '-fsyntax-only', '-Xclang', '-ast-dump', '-Xclang', '-ast-dump-filter=game_lab', inspect], 'declarations')
        records, stack, identities = {}, [], {}
        for line in (build / 'declarations.stdout').open():
            m = re.match(r'^([ |`-]*)([A-Za-z][A-Za-z0-9_]*)\s+(.*)$', line)
            if not m:
                continue
            depth, kind, body = len(m[1]), m[2], m[3].rstrip()
            while stack and depth <= stack[-1]['depth']:
                stack.pop()
            if kind == 'CXXRecordDecl' and ' definition' in body:
                n = re.search(r'\b(struct|class|union)\s+([\w:]+)\s+definition', body)
                name = n[2] if n else '<anonymous>'
                full = '::'.join([v['name'] for v in stack if v['name'] != '<anonymous>'] + [name])
                parent = re.search(r' parent (0x[0-9a-f]+)', body)
                if parent and parent[1] in identities:
                    full = identities[parent[1]] + '::' + name
                identities[body.split()[0]] = full
                record = dict(name=name, depth=depth, fields=[])
                stack.append(record)
                if name != '<anonymous>' and ' implicit ' not in body:
                    records[full] = record
            elif kind == 'FieldDecl' and stack:
                q = re.search(r"\s'([^']*)'", body)
                if q:
                    name = body[:q.start()].split()[-1]
                    if re.match(r'^\w+$', name):
                        stack[-1]['fields'].append(name)
        schema = (repo / 'game_lab/original_world_checkpoint_fields.inc').read_text()
        expected = {m[1]: re.findall(r'v.field\(\d+,x\.(\w+)\)', m[2]) for m in re.finditer(r'worldFields\(V& v,([\w:]+)& x\)\{(.*?)\n\}', schema, re.S)}
        mismatches = {n: dict(expected=f, actual=records.get(n, {}).get('fields')) for n, f in expected.items() if records.get(n, {}).get('fields') != f}
        implementation = (repo / 'game_lab/original_world_checkpoint_codec.cpp').read_text()
        remainder = re.search(r'worldFields\(V& v,Remainder& x\)\{(.*?)\nstruct Writer', implementation, re.S)
        remainder_fields = re.findall(r'v.field\(\d+,x.f\.(\w+)\)', remainder[1]) if remainder else []
        if remainder_fields != [f for f in expected['OriginalFigureUpdateContext'] if f != 'figures']:
            mismatches['section5-remainder'] = dict(actual=remainder_fields)
        coverage = dict(records=len(expected), direct_fields=sum(map(len, expected.values())), mismatches=mismatches)
        (build / 'coverage.json').write_text(json.dumps(coverage, indent=2) + '\n')
        if mismatches or not expected:
            raise RuntimeError('current declarations differ from the reviewed persistent wire schema')
        with ThreadPoolExecutor(max_workers=2) as pool:
            objects = dict(zip(helpers + tests, pool.map(compile_file, helpers + tests)))
        for name in tests:
            label = name.replace('/', '_')
            binary = build / label
            run(['/usr/bin/clang++', *flags, objects[name], *[objects[h] for h in helpers], '-framework', 'CoreGraphics', '-framework', 'CoreFoundation', '-lz', '-o', binary], label + '-link')
            argv = [binary]
            if name in ['original_world_checkpoint_io_tests', 'original_world_checkpoint_composition_tests', 'app/map_surface_tests']:
                argv.append(build / (label + '-output'))
            run(argv, label)
        if a.legacy:
            run([build / 'original_figure_tests', '--timeline-mode', 'combined', '--city-clocks', '0,0,0,0,0,0,0,0,0,0'], 'combined-authored')
        if data and (a.source or a.base_source or a.retired_source):
            real_env = dict(env, EMPEROR_GAMEDATA_ROOT=data)
            if a.source:
                run([build / 'app_original_world_checkpoint_tests', build / 'real-world', *a.source], 'real-world', real_env)
            for kind, names in [('base', a.base_source), ('retired', a.retired_source)]:
                if names:
                    run([build / 'app_original_world_checkpoint_tests', build / ('real-' + kind), '--' + kind, *names], 'real-' + kind, real_env)
        else:
            print(json.dumps({'SKIP': 'EMPEROR_GAMEDATA_ROOT unset or no explicit source paths'}), flush=True)
    except (OSError, RuntimeError, subprocess.SubprocessError) as e:
        error = str(e)
    stable = all((repo / f).is_file() and hashlib.sha256((repo / f).read_bytes()).hexdigest() == h for f, h in hashes.items())
    report = dict(mode=a.mode, passed=error is None and stable, error=error, source_stable=stable,
                  source_sha256=hashes, jobs=jobs, original_game_executed=False, GUI_executed=False, ASan=False)
    (build / 'verification.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
    print(json.dumps({'passed': report['passed'], 'error': error, 'report': str(build / 'verification.json')}), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
