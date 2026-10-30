#!/usr/bin/env python3
"""Exercise actual checkpoint App methods/controls/images without visible windows/event loop."""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import argparse, ast, hashlib, json, os, shlex, subprocess


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--mode', choices=['release', 'undefined'], default='release')
    p.add_argument('--build-dir', required=True, type=Path)
    p.add_argument('--source', action='append', default=[])
    a = p.parse_args()
    repo = Path(__file__).resolve().parents[1]
    out = a.build_dir.resolve()
    configured = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if not a.build_dir.is_absolute() or out == repo or repo in out.parents:
        p.error('fresh absolute output outside source required')
    if configured and (out == Path(configured).resolve() or Path(configured).resolve() in out.parents):
        p.error('output must stay outside original data')
    if configured and len(a.source) != 2:
        p.error('exactly two explicit source arguments required for original-data cases')
    out.mkdir(parents=True, exist_ok=False)
    module = ast.parse((repo / 'game_lab/app/build_provenance.py').read_text())
    compiled = next(ast.literal_eval(n.value) for n in module.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'compiled' for t in n.targets))
    names = ['game_lab/app/original_world_presentation_tests.mm' if n == 'game_lab/app/main.mm' else n for n in compiled]
    flags = ['-std=c++17', '-fobjc-arc', '-fno-modules', '-Wall', '-Wextra', '-Werror', '-mmacosx-version-min=15.0', '-I' + str(repo), '-I' + str(repo / 'media_demo'), '-I' + str(repo / 'file_io/include')]
    flags += ['-O2'] if a.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    jobs, hashes = [], {}
    digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    def run(argv, label, env=None):
        start = datetime.now(timezone.utc).isoformat()
        with (out / (label + '.stdout')).open('x') as so, (out / (label + '.stderr')).open('x') as se:
            result = subprocess.run(list(map(str, argv)), cwd=repo, stdout=so, stderr=se, env=env, timeout=1200)
        jobs.append(dict(name=label, argv=list(map(str, argv)), exit=result.returncode, started_utc=start, finished_utc=datetime.now(timezone.utc).isoformat()))
        print(json.dumps({'name': label, 'exit': result.returncode}), flush=True)
        if result.returncode:
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
        hashes.update(bindings)
        run(['/usr/bin/clang++', *flags, '-c', repo / name, '-o', obj], str(i) + '-compile')
        assert all(digest(repo / n) == h for n, h in bindings.items()), 'source changed during compilation'
        return obj
    error, result = None, None
    try:
        hashes[str(Path(__file__).resolve().relative_to(repo))] = digest(Path(__file__))
        with ThreadPoolExecutor(max_workers=2) as pool:
            objects = list(pool.map(compile_unit, enumerate(names)))
        binary = out / 'world-presentation-tests'
        frameworks = ['AppKit', 'Foundation', 'AVFoundation', 'AudioToolbox', 'CoreMedia', 'CoreVideo', 'ImageIO', 'CoreGraphics', 'Metal', 'QuartzCore']
        run(['/usr/bin/clang++', *flags, *objects, *[arg for f in frameworks for arg in ['-framework', f]], '-lz', '-o', binary], 'link')
        clean = dict(os.environ)
        clean.pop('EMPEROR_GAMEDATA_ROOT', None)
        run([binary], 'unset-root', clean)
        assert 'SKIP' in json.loads((out / 'unset-root.stdout').read_text())
        if configured:
            run([binary, out / 'authored-output', *a.source], 'run', dict(os.environ))
            result = json.loads((out / 'run.stdout').read_text())
            assert result['passed'] and result['actual_presenter'] == 'CPU NSImageView' and not result['GPU_executed'] and result['visible_windows'] == 0 and not result['event_loop_run']
    except (OSError, RuntimeError, AssertionError, subprocess.SubprocessError) as e:
        error = str(e)
    stable = all(digest(repo / n) == h for n, h in hashes.items())
    report = dict(mode=a.mode, passed=error is None and stable, source_stable=stable, error=error, result=result, sources_sha256=hashes, jobs=jobs, GUI=False, NSApplication_created=result.get('NSApplication_created') if result else False, GPU_executed=False, original_game_executed=False, ASan=False)
    (out / 'verification.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'passed': report['passed'], 'error': error, 'report': str(out / 'verification.json')}), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
