"""Build/test the explicit experimental native raw-MAP reference viewer.

Original-data metrics use only EMPEROR_GAMEDATA_ROOT and successful SKIP if empty.
A view needs the user's explicit --data-root. No assets or screenshots are shipped.
"""
from pathlib import Path
import argparse,json,os,subprocess,tempfile,sys,plistlib
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('task',choices=['test','scene-test','metrics','view','scene','scene-metrics']);p.add_argument('--scene-manifest',type=Path);p.add_argument('--producer-output',type=Path);p.add_argument('--mode',choices=['release','undefined'],default='release');p.add_argument('--build-dir',type=Path);p.add_argument('--data-root');p.add_argument('--map');p.add_argument('--sg3');p.add_argument('--raw-offset',type=int);p.add_argument('--raw-width',type=int);p.add_argument('--id-base',type=int);p.add_argument('--ground-selector',action='store_true');p.add_argument('--image-grid',action='store_true');p.add_argument('--sg3-elevation');p.add_argument('--sg3-tilesets');p.add_argument('--no-window',action='store_true');p.add_argument('--camera-x',type=float);p.add_argument('--camera-y',type=float);p.add_argument('--width',type=int);p.add_argument('--height',type=int);p.add_argument('--scale',type=int);p.add_argument('--output',type=Path);a=p.parse_args()
 if a.task in ('metrics','scene-metrics') and not os.environ.get('EMPEROR_GAMEDATA_ROOT'):print(json.dumps({'skipped':True,'reason':'EMPEROR_GAMEDATA_ROOT unset'}));return 0
 if a.output and not a.output.is_absolute():p.error('--output requires an absolute path')
 if a.producer_output and (not a.producer_output.is_absolute()or a.task not in ('scene','scene-metrics')):p.error('--producer-output is an absolute private scene-only path')
 build=(a.build_dir or Path(tempfile.mkdtemp(prefix='emperor-reference-'))).resolve();build.mkdir(parents=True,exist_ok=True);binary=build/('reference-'+a.task+'-'+a.mode)
 flags=['-O2']if a.mode=='release'else['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all'];sources=['game_lab/original_map.cpp','game_lab/original_save.cpp','game_lab/original_terrain.cpp','game_lab/map_reference.cpp','game_lab/original_ground.cpp','game_lab/sg3.cpp','game_lab/resource_root.cpp'];frameworks=['-framework','CoreGraphics','-framework','CoreFoundation'];objc=[]
 if a.task=='test':sources+=['game_lab/map_reference_tests.cpp']
 else:sources+=['game_lab/map_reference_view.mm'];objc=['-fobjc-arc'];frameworks+=['-framework','AppKit','-framework','ImageIO']
 subprocess.run(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror',*objc,*flags,*[str(ROOT/s)for s in sources],*frameworks,'-lz','-o',str(binary)],check=True)
 if a.task=='test':return subprocess.run([str(binary)]).returncode
 if a.task=='scene-test':return subprocess.run([sys.executable,'-B',str(ROOT/'scripts/map_reference_scene_tests.py'),'--binary',str(binary)]).returncode
 scene=a.task in ('scene','scene-metrics');raw=[]if (a.ground_selector or a.image_grid)else['raw_offset','raw_width','id_base']
 if scene:
  if not a.scene_manifest or not a.scene_manifest.is_absolute():p.error('--scene-manifest requires absolute path')
 else:
  for name in ['map','sg3']+raw+['camera_x','camera_y','width','height','scale']:
   if getattr(a,name)is None:p.error('--'+name.replace('_','-')+' must be explicit')
 data_root=os.environ['EMPEROR_GAMEDATA_ROOT']if a.task in ('metrics','scene-metrics')else a.data_root
 if not data_root:p.error('--data-root required for view')
 command=[str(binary),'--data-root',data_root]
 if scene:command+=['--scene-manifest',str(a.scene_manifest)]
 else:
  if a.ground_selector:command+=['--ground-selector']
  if a.image_grid:command+=['--image-grid']
  if a.sg3_elevation:command+=['--sg3-elevation',str(a.sg3_elevation)]
  if a.sg3_tilesets:command+=['--sg3-tilesets',str(a.sg3_tilesets)]
  for name in ['map','sg3']+raw+['camera_x','camera_y','width','height','scale']:command+=['--'+name.replace('_','-'),str(getattr(a,name))]
 if a.output:command+=['--output',str(a.output)]
 if a.producer_output:command+=['--producer-output',str(a.producer_output)]
 if a.task in ('metrics','scene-metrics')or a.no_window:command+=['--no-window']
 else:
  bundle=build/'ExperimentalMapReference.app';contents=bundle/'Contents';(contents/'MacOS').mkdir(parents=True,exist_ok=True);exe=contents/'MacOS'/'Reference';exe.write_bytes(binary.read_bytes());exe.chmod(0o755)
  with(contents/'Info.plist').open('wb')as f:plistlib.dump({'CFBundleExecutable':'Reference','CFBundleIdentifier':'org.emperor-native-media.experimental-map-reference','CFBundleName':'Experimental MAP Reference','CFBundleVersion':'1','LSMinimumSystemVersion':'15.0','NSHighResolutionCapable':True},f)
  subprocess.run(['/usr/bin/codesign','--force','--sign','-',str(bundle)],check=True);command[0]=str(exe)
 return subprocess.run(command).returncode
if __name__=='__main__':raise SystemExit(main())
