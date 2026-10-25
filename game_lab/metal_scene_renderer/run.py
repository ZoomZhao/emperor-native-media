#!/usr/bin/env python3
"""Build and run the persistent Metal scene renderer tests (no App, no GUI).

Authored tests always run. `--real` scenes run only when EMPEROR_GAMEDATA_ROOT
is set for this process; the data files are hashed before/after (read-only).
Build products and verification.json go to --output-dir, outside the repository.
"""
from pathlib import Path
import argparse, hashlib, json, os, re, subprocess, sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
LAB = ROOT / 'game_lab'
FORBIDDEN = ['getBytes', 'waitUntilCompleted', 'waitUntilScheduled', 'AppKit', 'NSView', 'NSImage',
             'CAMetalLayer', 'nextDrawable', 'presentDrawable', 'CGImage']

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def static_checks():
    text = (HERE / 'metal_scene_renderer.mm').read_text()
    code = re.sub(r'//[^\n]*', '', text)  # comments may name the rules
    hits = {name: len(re.findall(r'\b' + name + r'\b', code)) for name in FORBIDDEN}
    return {'file': 'metal_scene_renderer.mm', 'forbidden_tokens': hits,
            'forbidden_total': sum(hits.values()), 'tokens_checked': len(FORBIDDEN)}

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--mode', choices=['release', 'undefined'], default='release')
    ap.add_argument('--output-dir', type=Path, required=True)
    args = ap.parse_args()
    out = args.output_dir.resolve()
    if out == ROOT or ROOT in out.parents: ap.error('output must be outside the repository')
    out.mkdir(parents=True, exist_ok=True)
    flags = ['-std=c++17', '-Wall', '-Wextra', '-Werror', '-mmacosx-version-min=15.2', '-I' + str(LAB)]
    flags += ['-O2'] if args.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
    own = ['metal_scene_renderer/metal_scene_renderer_tests.mm', 'metal_scene_renderer/metal_scene_renderer.mm',
           'metal_scene_renderer/metal_scene_renderer_testing.mm']
    existing = [n + '.cpp' for n in ['app/map_surface', 'app/map_surface_preview', 'app/city_playback', 'map_reference',
                                     'original_map', 'original_terrain', 'original_ground', 'original_save', 'original_pak',
                                     'sg3', 'resource_root']]
    objects, bindings = [], {}
    for name in own + existing:
        src = LAB / name; obj = out / (name.replace('/', '-') + '.o'); dep = obj.with_suffix('.d')
        extra = ['-fobjc-arc', '-fno-modules'] if src.suffix == '.mm' else []
        subprocess.run(['/usr/bin/clang++', *flags, *extra, '-MMD', '-MF', str(dep), '-c', str(src), '-o', str(obj)], check=True)
        objects.append(obj)
        for item in dep.read_text().replace('\\\n', ' ').split(':', 1)[1].split():
            p = Path(item).resolve()
            if ROOT in p.parents: bindings[str(p.relative_to(ROOT))] = sha(p)
    binary = out / 'metal-scene-renderer-tests'
    subprocess.run(['/usr/bin/clang++', *flags, *map(str, objects), '-framework', 'Foundation', '-framework', 'Metal',
                    '-framework', 'CoreGraphics', '-framework', 'CoreFoundation', '-lz', '-o', str(binary)], check=True)
    runs = []
    def run(argv, env):
        p = subprocess.run([str(binary), *argv], env=env, text=True, capture_output=True)
        rows = []
        for line in p.stdout.splitlines():
            try: rows.append(json.loads(line))
            except json.JSONDecodeError: rows.append({'raw': line})
        runs.append({'arguments': argv, 'exit': p.returncode, 'stderr': p.stderr, 'results': rows})
        sys.stdout.write(p.stdout); sys.stdout.flush()
        if p.stderr: sys.stderr.write(p.stderr)
        return p.returncode
    clean = dict(os.environ); clean.pop('EMPEROR_GAMEDATA_ROOT', None)
    codes = {'authored': run([], clean), 'real_unset': run(['--real'], clean)}
    skip_ok = bool(runs[-1]['results']) and 'SKIP' in runs[-1]['results'][0]
    inputs_before, inputs_after = {}, {}
    configured = os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if configured:
        data = Path(configured).resolve()
        names = ['Model/EmperorBuildingModels.txt', 'Save/2026102301/202610230101.sav', 'Save/2026102301/320111.sav', 'Save/2026102401/240103.sav']
        names += [str(p.relative_to(data)) for p in (data / 'DATA').rglob('*') if p.is_file() and p.suffix.lower() in ('.sg3', '.555')]
        inputs_before = {n: sha(data / n) for n in names}
        codes['real'] = run(['--real'], dict(os.environ))
        inputs_after = {n: sha(data / n) for n in names}
    checks = static_checks()
    sources_unchanged = all(sha(ROOT / n) == h for n, h in bindings.items())
    verdict = (codes['authored'] == 0 and codes['real_unset'] == 0 and skip_ok and checks['forbidden_total'] == 0
               and sources_unchanged and codes.get('real', 0) == 0 and inputs_before == inputs_after)
    summary = {'mode': args.mode, 'exit_codes': codes, 'real_unset_skip': skip_ok, 'static_checks': checks,
               'sources_sha256': bindings, 'sources_unchanged_during_run': sources_unchanged, 'binary_sha256': sha(binary),
               'real_inputs_checked': len(inputs_before), 'real_inputs_unchanged': inputs_before == inputs_after,
               'App_executed': False, 'GUI_or_display_claim': False, 'passed': verdict, 'runs': runs}
    (out / 'verification.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(json.dumps({'runner': {k: summary[k] for k in ('mode', 'exit_codes', 'real_unset_skip', 'sources_unchanged_during_run',
                                                         'real_inputs_checked', 'real_inputs_unchanged', 'passed')},
                      'static_forbidden_total': checks['forbidden_total'], 'tokens_checked': checks['tokens_checked']}))
    return 0 if verdict else 1

if __name__ == '__main__':
    sys.exit(main())
