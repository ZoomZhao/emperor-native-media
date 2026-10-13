"""Build modern SG3 tests or emit aggregate original-data metrics; never export assets."""
from pathlib import Path
import argparse
import os
import subprocess

ROOT=Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('task',choices=['sg3-test','metrics'])
    parser.add_argument('--mode',choices=['release','undefined'],default='release')
    parser.add_argument('--build-dir',type=Path,default=ROOT/'.local'/'data-first-build')
    args=parser.parse_args()
    if args.task=='metrics' and not os.environ.get('EMPEROR_GAMEDATA_ROOT'):
        print('{"skipped":true,"reason":"EMPEROR_GAMEDATA_ROOT unset"}')
        return 0
    build=args.build_dir.resolve();build.mkdir(parents=True,exist_ok=True)
    binary=build/(args.task+'-'+args.mode)
    flags=['-O2'] if args.mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all']
    sources=['game_lab/sg3.cpp','game_lab/resource_root.cpp']
    frameworks=[]
    if args.task=='sg3-test':sources+=['game_lab/sg3_tests.cpp']
    else:
        sources+=['game_lab/data_metrics.mm','game_lab/emperor_assets.mm','game_lab/asset_pixels.cpp','game_lab/wav_pcm.cpp','surface_lab/surface.cpp']
        frameworks=['-fobjc-arc','-fno-modules','-framework','Foundation','-framework','ImageIO','-framework','CoreGraphics','-lz']
    subprocess.run(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,*[str(ROOT/p) for p in sources],*frameworks,'-o',str(binary)],check=True)
    # No subprocess timeout/forced termination: process ownership and normal cleanup remain intact.
    return subprocess.run([str(binary)],check=False).returncode
if __name__=='__main__':raise SystemExit(main())
