#!/usr/bin/env python3
"""Independent preventive-Inspector tests; originals stay read-only.

Builds and binds one stable source revision, tests authored contracts and the
two explicitly admitted original source files. No App/original GUI/model call.
"""
from datetime import datetime, timezone
from pathlib import Path
import argparse
import hashlib
import json
import os
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ('Save/2026102301/历史战役自动保存_1.sav',
           'Save/2026102401/历史战役自动保存_1.sav')
ASSETS = ('Model/EmperorBuildingModels.txt', 'Model/EmperorFigureModels.txt', 'EmperorText.eng',
          'DATA/China_General.sg3', 'DATA/China_Terrain.sg3', 'DATA/China_Elevation.sg3',
          'DATA/SprAmbient.sg3', 'DATA/SprMain.sg3', 'DATA/SprMain2.sg3', 'DATA/SprMain3.sg3')


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def utc():
    return datetime.now(timezone.utc).isoformat()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=('release', 'undefined'), default='release')
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--source', choices=SOURCES, action='append')
    parser.add_argument('--max-steps', type=int, default=10000)
    parser.add_argument('--run-timeout', type=int, default=1200)
    parser.add_argument('--reuse-build', type=Path)
    parser.add_argument('--checkpoint', type=Path, help='real paused pre-first-patrol input for exactly one source')
    args = parser.parse_args()
    out = args.output_dir.resolve()
    if not args.output_dir.is_absolute() or out == ROOT or ROOT in out.parents or out.exists():
        parser.error('a new absolute output directory outside the repository is required')
    if not 0 < args.max_steps <= 10000 or not 0 < args.run_timeout <= 7200:
        parser.error('max steps1..10000; timeout1..7200 seconds')
    data = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    root = Path(data).resolve() if data else None
    if root and (out == root or root in out.parents):
        parser.error('output must be outside original data')
    if args.checkpoint:
        if len(args.source or ()) != 1 or not args.checkpoint.is_absolute() or not args.checkpoint.is_file():
            parser.error('checkpoint requires one explicit source and an existing absolute input')
        if root and (args.checkpoint.resolve() == root or root in args.checkpoint.resolve().parents):
            parser.error('native preparation must be outside original data root')
    out.mkdir(parents=True)
    result = {'suite': 'native-preventive-inspector', 'mode': args.mode, 'started_utc': utc(),
              'passed': False, 'driver_sha256': sha(__file__), 'commands': [], 'jobs': [],
              'dependency_hashes': {}, 'input_hashes': {}, 'sources': [],
              'App_executed': False, 'original_game_executed': False, 'ASan_executed': False,
              'external_models': False}
    bindings, originals = result['dependency_hashes'], result['input_hashes']
    if args.checkpoint:
        originals[str(args.checkpoint.resolve())] = sha(args.checkpoint)
        result['real_preparation_checkpoint'] = str(args.checkpoint.resolve())

    def save():
        (out/'verification.json').write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n')

    def command(argv, label, timeout=180, env=None):
        result['commands'].append(argv)
        stdout, stderr = out/(label+'.stdout'), out/(label+'.stderr')
        save()
        started = utc()
        with stdout.open('xb') as so, stderr.open('xb') as se:
            process = subprocess.Popen(argv, cwd=ROOT, env=env, stdout=so, stderr=se)
            try:
                code = process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                # Only this runner's test/compiler child, never a user service.
                process.terminate()
                process.wait(timeout=15)
                code = 'timeout'
        job = {'label': label, 'started_utc': started, 'finished_utc': utc(), 'returncode': code,
               'stdout': str(stdout), 'stderr': str(stderr)}
        result['jobs'].append(job)
        save()
        print(json.dumps({'label': label, 'exit': code}), flush=True)
        if code != 0:
            raise RuntimeError(label+' failed; exact logs retained at '+str(out))
        if args.mode == 'undefined' and 'runtime error:' in stderr.read_text():
            raise RuntimeError(label+' UBSan diagnostic')
        return stdout

    try:
        flags = ['-std=c++17', '-Wall', '-Wextra', '-Werror', '-I'+str(ROOT/'game_lab')]
        flags += ['-O2'] if args.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
        frozen = {str(p.resolve()): sha(p) for base in ('game_lab', 'file_io/include')
                  for p in (ROOT/base).rglob('*') if p.is_file() and p.suffix in ('.h', '.hpp', '.cpp', '.inc', '.mm')}
        if args.reuse_build:
            prior = json.loads((args.reuse_build/'verification.json').read_text())
            if prior['mode'] != args.mode:
                raise ValueError('reuse mode mismatch')
            for name, digest in prior['dependency_hashes'].items():
                if sha(name) != digest:
                    raise ValueError('reused source changed: '+name)
            binary, golden = Path(prior['binary']), Path(prior['golden_binary'])
            if sha(binary) != prior['binary_sha256'] or sha(golden) != prior['golden_binary_sha256']:
                raise ValueError('reused binary changed')
            bindings.update(prior['dependency_hashes'])
            result['reused_build'] = str(args.reuse_build)
        else:
            objects = []
            for name in ('native_inspector_tests', 'original_map', 'original_terrain', 'original_ground',
                         'original_save', 'original_pak', 'sg3', 'resource_root'):
                obj, dep = out/(name+'.o'), out/(name+'.d')
                command(['/usr/bin/clang++', *flags, '-MMD', '-MF', str(dep), '-c',
                         str(ROOT/'game_lab'/(name+'.cpp')), '-o', str(obj)], 'compile-'+name)
                objects.append(obj)
                for token in shlex.split(dep.read_text().replace(chr(92)+chr(10), ' ').split(':', 1)[1]):
                    path = Path(token).resolve()
                    if ROOT in path.parents:
                        bindings[str(path)] = frozen.get(str(path), sha(path))
            binary = out/'native-inspector-tests'
            command(['/usr/bin/clang++', *flags, *map(str, objects), '-framework', 'CoreGraphics',
                     '-framework', 'CoreFoundation', '-lz', '-o', str(binary)], 'link')
            result['objects_sha256'] = {p.name: sha(p) for p in objects}
            golden = out/'legacy-runtime-wire-tests'
            dep = out/'legacy-runtime-wire-tests.d'
            command(['/usr/bin/clang++', *flags, '-MMD', '-MF', str(dep),
                     str(ROOT/'game_lab/native_house_runtime_wire_tests.cpp'), '-o', str(golden)], 'compile-old-wire')
            for token in shlex.split(dep.read_text().replace(chr(92)+chr(10), ' ').split(':', 1)[1]):
                path = Path(token).resolve()
                if ROOT in path.parents:
                    bindings[str(path)] = frozen.get(str(path), sha(path))
        result.update(binary=str(binary), binary_sha256=sha(binary), golden_binary=str(golden),
                      golden_binary_sha256=sha(golden))
        bindings[str(Path(__file__).resolve())] = result['driver_sha256']
        clean = dict(os.environ)
        clean.pop('EMPEROR_GAMEDATA_ROOT', None)
        skip = command([str(binary)], 'authored-and-real-skip', env=clean)
        rows = [json.loads(line) for line in skip.read_text().splitlines()]
        if not any('SKIP' in row for row in rows):
            raise ValueError('unset-root SKIP missing')
        result['authored_and_skip'] = rows
        old = command([str(golden)], 'legacy-wire-golden', env=clean)
        result['legacy_wire_results'] = [json.loads(line) for line in old.read_text().splitlines()]
        if not root:
            result['SKIP'] = 'EMPEROR_GAMEDATA_ROOT unset'
        else:
            selected = args.source or SOURCES
            for name in (*ASSETS, *selected):
                source = (root/name).resolve()
                if root not in source.parents:
                    raise ValueError('input outside exact source root')
                originals[str(source)] = sha(source)
            for index, name in enumerate(selected):
                target = out/('source-'+str(index))
                target.mkdir()
                invocation = [str(binary), name, str(target), str(args.max_steps)]
                if args.checkpoint:
                    invocation.append(str(args.checkpoint.resolve()))
                stdout = command(invocation, 'source-'+str(index), args.run_timeout)
                rows = [json.loads(line) for line in stdout.read_text().splitlines()]
                terminal = rows[-1]
                if not terminal.get('passed') or terminal.get('natural_rounds', 0) < 2 or terminal.get('risk_reducing_rounds', 0) < 2:
                    raise ValueError('natural proof incomplete: '+name)
                result['sources'].append({'source': name, 'results': rows, 'terminal': terminal})
                print(json.dumps(terminal, ensure_ascii=False), flush=True)
        result['passed'] = True
    except (OSError, ValueError, RuntimeError, KeyError) as error:
        result['error'] = str(error)
        print(str(error), file=sys.stderr, flush=True)
    finally:
        result['inputs_unchanged'] = all(Path(p).is_file() and sha(p) == h for p, h in originals.items())
        result['source_stable'] = all(Path(p).is_file() and sha(p) == h for p, h in bindings.items())
        result['passed'] = result['passed'] and result['inputs_unchanged'] and result['source_stable']
        result['finished_utc'] = utc()
        save()
    print(json.dumps({'passed': result['passed'], 'report': str(out/'verification.json')}), flush=True)
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
