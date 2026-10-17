"""Build and run the authored bit-exact reimplementation of the original per-cell
random grid 0x00F1E780 (FUN_004189B0 @0x004189B0, FUN_004B0AC0 @0x004B0AC0).

Emperor[CH].exe sha256 0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658.
No original asset, save, listing or binary is committed; only offsets and formulas.
"""
from pathlib import Path
import argparse,json,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('task',choices=['test']);p.add_argument('--mode',choices=['release','undefined'],default='release');p.add_argument('--build-dir',type=Path);a=p.parse_args()
 build=(a.build_dir or Path(tempfile.mkdtemp(prefix='emperor-random-'))).resolve();build.mkdir(parents=True,exist_ok=True);binary=build/('original_random-'+a.mode)
 flags=['-O2']if a.mode=='release'else['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all']
 sources=['game_lab/original_map.cpp','game_lab/original_save.cpp','game_lab/original_random.cpp','game_lab/original_random_tests.cpp']
 subprocess.run(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*flags,*[str(ROOT/s)for s in sources],'-lz','-o',str(binary)],check=True)
 run=subprocess.run([str(binary)],capture_output=True,text=True)
 passed=run.returncode==0
 report={'passed':passed,'assertions':0,'mode':a.mode,'original_data':False}
 try:
  detail=json.loads(run.stdout.strip().splitlines()[-1]);report['assertions']=detail.get('assertions',0);report['original_data']=detail.get('original_data',False)
  if report['original_data']:report['saves_checked']=detail.get('saves_checked',0);report['stream_indices']=detail.get('stream_indices',[])
  passed=passed and detail.get('passed') is True
 except Exception:passed=False
 report['passed']=passed
 print(json.dumps(report))
 if not passed:print(run.stderr.strip(),flush=True)
 return 0 if passed else 1
if __name__=='__main__':raise SystemExit(main())
