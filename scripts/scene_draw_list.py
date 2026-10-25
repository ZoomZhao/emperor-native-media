#!/usr/bin/env python3
"""CPU / offscreen Metal draw-list verification. No App, assets written or GUI."""
from pathlib import Path
import argparse, hashlib, json, os, shlex, subprocess
ROOT = Path(__file__).resolve().parents[1]
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=['release', 'undefined'], default='release')
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--metal', action='store_true')
    parser.add_argument('--reuse-build', action='store_true', help='Require exact existing source/object/binary hashes')
    args = parser.parse_args(); out = args.output_dir.resolve()
    if out == ROOT or ROOT in out.parents: parser.error('output must be outside repository')
    out.mkdir(parents=True, exist_ok=True)
    binary = out / 'scene-draw-tests'
    if not args.reuse_build:
        flags = ['-std=c++17', '-Wall', '-Wextra', '-Werror', '-mmacosx-version-min=15.2', '-I'+str(ROOT/'game_lab')]
        flags += ['-O2'] if args.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
        sources = ['scene_draw_list_tests.mm', 'metal_scene_prototype/metal_sprite_renderer.mm']
        sources += [n+'.cpp' for n in ['app/map_surface','app/map_surface_preview','app/city_playback','map_reference','original_map','original_terrain','original_ground','original_save','original_pak','sg3','resource_root']]
        commands = []; objects = []; bindings = {}
        for name in sources:
            source = ROOT/'game_lab'/name; obj = out/(name.replace('/','-')+'.o'); dep = obj.with_suffix('.d')
            cmd = ['/usr/bin/clang++', *flags, *(['-fobjc-arc','-fno-modules'] if source.suffix=='.mm' else []), '-MMD','-MF',str(dep),'-c',str(source),'-o',str(obj)]
            subprocess.run(cmd, check=True); commands.append(cmd); objects.append(obj)
            for item in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                p = Path(item).resolve()
                if ROOT in p.parents: bindings[str(p.relative_to(ROOT))] = sha(p)
        cmd = ['/usr/bin/clang++', *flags, *map(str,objects), '-framework','Foundation','-framework','Metal','-framework','CoreGraphics','-framework','CoreFoundation','-lz','-o',str(binary)]
        subprocess.run(cmd,check=True); commands.append(cmd)
        (out/'build.json').write_text(json.dumps({'mode':args.mode,'sources_sha256':bindings,'commands':commands,'objects_sha256':{p.name:sha(p) for p in objects},'binary_sha256':sha(binary)},indent=2)+'\n')
    build = json.loads((out/'build.json').read_text())
    assert build['mode']==args.mode
    assert all(sha(ROOT/n)==h for n,h in build['sources_sha256'].items())
    assert all(sha(out/n)==h for n,h in build['objects_sha256'].items()) and sha(binary)==build['binary_sha256']
    runs = []
    def run(arguments, env=None):
        p = subprocess.run([str(binary),*arguments],env=env,text=True,capture_output=True,check=True)
        if p.stderr: raise RuntimeError(p.stderr)
        rows = [json.loads(line) for line in p.stdout.splitlines()]
        runs.append({'arguments':arguments,'results':rows}); print(p.stdout,end='',flush=True)
    run([]); clean = dict(os.environ); clean.pop('EMPEROR_GAMEDATA_ROOT',None)
    run(['--cpu'],clean); assert 'SKIP' in runs[-1]['results'][0]
    configured = os.environ.get('EMPEROR_GAMEDATA_ROOT'); inputs = {}
    if configured:
        root = Path(configured).resolve()
        names = ['Model/EmperorBuildingModels.txt','Save/2026102301/202610230101.sav','Save/2026102301/320111.sav','Save/2026102401/240103.sav']
        names += [str(p.relative_to(root)) for p in (root/'DATA').rglob('*') if p.is_file() and p.suffix.lower() in ('.sg3','.555')]
        inputs = {n:sha(root/n) for n in names}
        run(['--metal' if args.metal else '--cpu'])
        assert all(sha(root/n)==h for n,h in inputs.items())
    assert all(sha(ROOT/n)==h for n,h in build['sources_sha256'].items())
    (out/'verification.json').write_text(json.dumps({'driver_sha256':sha(__file__),'build':build,'runs':runs,'read_only_inputs_sha256':inputs,'App_executed':False,'GPU_offscreen_requested':args.metal},indent=2)+'\n')
if __name__=='__main__': main()
