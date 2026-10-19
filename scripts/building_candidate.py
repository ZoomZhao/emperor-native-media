"""Authored manual building-candidate contract tests; no automatic image binding."""
from pathlib import Path
import argparse,os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
def run(a,build):
    build=build.resolve()
    selected=os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if selected:
        data=Path(selected).resolve()
        if build==data or data in build.parents:raise ValueError('build directory must stay outside GameData')
    build.mkdir(parents=True,exist_ok=True);binary=build/('building-candidate-'+a.task+'-'+a.mode)
    flags=['-O2']if a.mode=='release'else['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all']
    source='game_lab/original_building_candidate_tests.mm' if a.task=='test' else 'game_lab/original_building_candidate_metrics.mm'
    sources=[source,'game_lab/original_map.cpp']
    frameworks=['-framework','Foundation']
    if a.task=='metrics':
        sources+=['game_lab/original_pak.cpp','game_lab/original_save.cpp','game_lab/original_terrain.cpp','game_lab/resource_root.cpp','game_lab/sg3.cpp']
        frameworks+=['-framework','CoreGraphics']
    subprocess.run(['/usr/bin/clang++','-std=c++17','-fobjc-arc','-Wall','-Wextra','-Werror',*flags,*[str(ROOT/x)for x in sources],*frameworks,'-lz','-o',str(binary)],check=True)
    return subprocess.run([str(binary),*([str(a.manifest)]if a.task=='metrics'else[])]).returncode

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('task',choices=['test','metrics']);p.add_argument('--mode',choices=['release','undefined'],default='release');p.add_argument('--build-dir',type=Path);p.add_argument('--manifest',type=Path);a=p.parse_args()
    if a.task=='metrics' and not os.environ.get('EMPEROR_GAMEDATA_ROOT'):
        print('{"skipped":true,"reason":"EMPEROR_GAMEDATA_ROOT unset or empty"}');return 0
    if a.task=='metrics' and (not a.manifest or not a.manifest.is_absolute()):p.error('absolute local --manifest required')
    if a.build_dir:
        if not a.build_dir.is_absolute():p.error('absolute build directory required')
        return run(a,a.build_dir)
    with tempfile.TemporaryDirectory(prefix='emperor-building-candidate-')as temp:return run(a,Path(temp))
if __name__=='__main__':raise SystemExit(main())
