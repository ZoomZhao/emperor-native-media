#!/usr/bin/env python3
"""Verify existing native checkpoints with nonempty building/figure sprites on real Metal."""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import argparse, ast, hashlib, json, os, shlex, subprocess
from threading import Lock


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--mode', choices=['release', 'undefined'], default='release')
    p.add_argument('--build-dir', required=True, type=Path)
    p.add_argument('--checkpoint', action='append', default=[])
    a = p.parse_args()
    repo = Path(__file__).resolve().parents[1]
    out = a.build_dir.resolve()
    configured = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if not a.build_dir.is_absolute() or out == repo or repo in out.parents:
        p.error('fresh absolute output outside source required')
    if configured and (out == Path(configured).resolve() or Path(configured).resolve() in out.parents):
        p.error('output must stay outside original data')
    if configured and len(a.checkpoint) != 2:
        p.error('exactly two explicit accepted native checkpoints required')
    if configured:
        data_root = Path(configured).resolve()
        checkpoint_paths = []
        for name in a.checkpoint:
            path = Path(name)
            resolved = path.resolve()
            if not path.is_absolute() or resolved.suffix != '.encity' or not resolved.is_file() or data_root == resolved or data_root in resolved.parents:
                p.error('checkpoint must be an existing absolute .encity outside original data')
            checkpoint_paths.append(resolved)
        if checkpoint_paths[0].samefile(checkpoint_paths[1]):
            p.error('two distinct accepted checkpoint files required')
    out.mkdir(parents=True, exist_ok=False)
    checkpoint_hashes = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in checkpoint_paths} if configured else {}
    manifest_name = 'game_lab/app/build_provenance.py'
    manifest_bytes = (repo / manifest_name).read_bytes()
    module = ast.parse(manifest_bytes.decode())
    compiled = next(ast.literal_eval(n.value) for n in module.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'compiled' for t in n.targets))
    names = ['game_lab/app/native_checkpoint_metal_tests.mm' if n == 'game_lab/app/main.mm' else n for n in compiled]
    names.append('game_lab/metal_scene_renderer/metal_scene_renderer_testing.mm')
    flags = ['-std=c++17', '-fobjc-arc', '-fno-modules', '-Wall', '-Wextra', '-Werror', '-mmacosx-version-min=15.0', '-I' + str(repo), '-I' + str(repo / 'media_demo'), '-I' + str(repo / 'file_io/include')]
    flags += ['-O2'] if a.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    jobs, hashes = [], {manifest_name: hashlib.sha256(manifest_bytes).hexdigest()}
    binding_lock = Lock()
    digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    def run(argv, label, env=None, expected=0):
        start = datetime.now(timezone.utc).isoformat()
        with (out / (label + '.stdout')).open('x') as so, (out / (label + '.stderr')).open('x') as se:
            result = subprocess.run(list(map(str, argv)), cwd=repo, stdout=so, stderr=se, env=env, timeout=1200)
        jobs.append(dict(name=label, argv=list(map(str, argv)), exit=result.returncode, started_utc=start, finished_utc=datetime.now(timezone.utc).isoformat()))
        print(json.dumps({'name': label, 'exit': result.returncode}), flush=True)
        if result.returncode != expected:
            raise RuntimeError(label + ': ' + (out / (label + '.stderr')).read_text()[-3000:])
    def compile_unit(item):
        i, name = item
        obj, dep = out / (str(i) + '.o'), out / (str(i) + '.d')
        run(['/usr/bin/clang++', *flags, '-MM', '-MT', str(obj), '-MF', dep, repo / name], str(i) + '-dependencies')
        bindings = {}
        for n in shlex.split(dep.read_text().replace('\\\n', ' ').partition(':')[2]):
            path = Path(n).resolve()
            if repo in path.parents:
                bindings[str(path.relative_to(repo))] = digest(path)
        with binding_lock:
            for dependency_name, dependency_hash in bindings.items():
                assert dependency_name not in hashes or hashes[dependency_name] == dependency_hash, 'mixed compilation dependency: ' + dependency_name
                hashes.setdefault(dependency_name, dependency_hash)
        run(['/usr/bin/clang++', *flags, '-c', repo / name, '-o', obj], str(i) + '-compile')
        assert all(digest(repo / n) == h for n, h in bindings.items()), 'source changed during compilation'
        return obj
    error, result = None, None
    try:
        hashes[str(Path(__file__).resolve().relative_to(repo))] = digest(Path(__file__))
        with ThreadPoolExecutor(max_workers=2) as pool:
            objects = list(pool.map(compile_unit, enumerate(names)))
        binary = out / 'native-checkpoint-metal-tests'
        frameworks = ['AppKit', 'Foundation', 'AVFoundation', 'AudioToolbox', 'CoreMedia', 'CoreVideo', 'ImageIO', 'CoreGraphics', 'Metal', 'QuartzCore']
        run(['/usr/bin/clang++', *flags, *objects, *[arg for f in frameworks for arg in ['-framework', f]], '-lz', '-o', binary], 'link')
        clean = dict(os.environ)
        clean.pop('EMPEROR_GAMEDATA_ROOT', None)
        run([binary], 'unset-root', clean)
        assert 'SKIP' in json.loads((out / 'unset-root.stdout').read_text())
        if configured:
            # Binary guard tests use only our authored temporary directories.
            guard_root = out / 'authored-root'
            guard_root.mkdir()
            for name in ('a.encity', 'b.encity'):
                (guard_root / name).write_text('authored path-guard input')
            guard_env = dict(os.environ, EMPEROR_GAMEDATA_ROOT=str(guard_root))
            run([binary, '--prepare', guard_root / 'forbidden-output', guard_root / 'a.encity', guard_root / 'b.encity'], 'inside-root-refused', guard_env, 1)
            assert not (guard_root / 'forbidden-output').exists()
            assert (out / 'inside-root-refused.stderr').read_text().strip() == 'new output outside original data'
            run([binary, '--prepare', out / 'duplicate-output', guard_root / 'a.encity', guard_root / 'a.encity'], 'duplicate-source-refused', guard_env, 1)
            assert not (out / 'duplicate-output').exists()
            assert (out / 'duplicate-source-refused.stderr').read_text().strip() == 'two distinct accepted checkpoint files'
            cases_dir = out / 'checkpoints'
            run([binary, '--prepare', cases_dir, *a.checkpoint], 'prepare', dict(os.environ))
            cases = [json.loads(line) for line in (out / 'prepare.stdout').read_text().splitlines() if line.strip()]
            assert len(cases) == 2 and {c['source_index'] for c in cases} == {0, 1}
            assert all(c['pre_save_GPU'] and c['building_resources'] > 0 and c['figure_pixel_differences'] > 0 for c in cases)
            verified = []
            for i, case in enumerate(cases):
                file = cases_dir / case['case']
                assert file.is_file() and digest(file) == case['state_sha256']
                run([binary, '--verify', file, case['cpu_sha256'], case['state_sha256']], 'restore-' + str(i), dict(os.environ))
                value = json.loads((out / ('restore-' + str(i) + '.stdout')).read_text())
                assert value['passed'] and value['GPU_executed'] and not value['GUI']
                assert value['restored_ready_frames'] == 2 and value['failed_frames'] == 1
                assert value['scene_items'] > 0 and value['drawn_items'] > 0 and value['resources_drawn'] > 0 and value['draw_calls'] > 1
                assert value['building_resources'] > 0 and value['visible_figure_pixels'] > 0
                assert value['registry_id'] != 0 and value['different_pixels'] == 0
                verified.append(dict(case=case, result=value))
            result = dict(passed=True, original_sources=2, checkpoint_cases=len(verified), fresh_process_restores=len(verified),
                          GPU_executed=True, pre_save_ready_frames=len(cases), restored_ready_frames=sum(v['result']['restored_ready_frames'] for v in verified),
                          injected_failed_frames=sum(v['result']['failed_frames'] for v in verified),
                          sprite_cases=sum(v['result']['drawn_items'] > 0 for v in verified),
                          checks=sum(v['result']['checks'] for v in verified), cases=verified)
    except (OSError, RuntimeError, AssertionError, subprocess.SubprocessError) as e:
        error = str(e)
    stable = all(digest(repo / n) == h for n, h in hashes.items()) and all(digest(Path(n)) == h for n, h in checkpoint_hashes.items())
    report = dict(mode=a.mode, passed=error is None and stable, source_stable=stable, error=error, result=result, sources_sha256=hashes, checkpoint_inputs_sha256=checkpoint_hashes, jobs=jobs, GUI=False, NSApplication_created=False, GPU_executed=bool(result and result.get('GPU_executed')), original_game_executed=False, ASan=False)
    (out / 'verification.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'passed': report['passed'], 'error': error, 'report': str(out / 'verification.json')}), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
