"""App stored-surface adapter checks; metrics only read process-local data root."""
import os
import sys
if len(sys.argv)>1 and sys.argv[1]=='metrics' and not os.environ.get('EMPEROR_GAMEDATA_ROOT'):
    print('{"skipped":true,"reason":"EMPEROR_GAMEDATA_ROOT unset or empty"}')
    raise SystemExit(0)
from pathlib import Path
import argparse
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('task',choices=['test','metrics']);p.add_argument('--mode',choices=['release','undefined'],default='release');p.add_argument('--build-dir',type=Path)
    a=p.parse_args();build=(a.build_dir or Path(tempfile.mkdtemp(prefix='emperor-app-surface-'))).resolve()
    configured=os.environ.get('EMPEROR_GAMEDATA_ROOT')
    if configured and build.is_relative_to(Path(configured).resolve()):p.error('--build-dir must be outside the data root')
    build.mkdir(parents=True,exist_ok=True)
    binary=build/('app-map-surface-'+a.mode)
    sources=['game_lab/app/map_surface.cpp','game_lab/app/map_surface_tests.cpp','game_lab/map_reference.cpp','game_lab/original_map.cpp','game_lab/original_terrain.cpp','game_lab/original_ground.cpp','game_lab/original_save.cpp','game_lab/sg3.cpp','game_lab/resource_root.cpp']
    flags=['-O2']if a.mode=='release'else['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all']
    subprocess.run(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,*[str(ROOT/s)for s in sources],'-framework','CoreGraphics','-framework','CoreFoundation','-lz','-o',str(binary)],check=True)
    fixture=Path(tempfile.mkdtemp(prefix='owned-fixtures-',dir=build))if a.task=='test'else None
    return subprocess.run([str(binary),'--metrics'if a.task=='metrics'else str(fixture)],check=False).returncode
if __name__=='__main__':raise SystemExit(main())
