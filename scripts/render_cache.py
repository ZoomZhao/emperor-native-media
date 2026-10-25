#!/usr/bin/env python3
"""Bounded sprite-cache checks and original-data CPU comparison; no App launch."""
from pathlib import Path
import argparse,hashlib,json,os,shlex,subprocess
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--mode',choices=['release','undefined'],default='release');p.add_argument('--output-dir',type=Path,required=True);a=p.parse_args();out=a.output_dir.resolve();assert out!=ROOT and ROOT not in out.parents;out.mkdir(parents=True,exist_ok=True)
 flags=['-std=c++17','-Wall','-Wextra','-Werror']+(['-O2']if a.mode=='release'else['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all']);sources=['render_cache_tests','app/map_surface','app/map_surface_preview','app/city_playback','map_reference','original_map','original_terrain','original_ground','original_save','original_pak','sg3','resource_root'];commands=[];bindings={};objects=[]
 for name in sources:
  obj=out/(name.replace('/','-')+'.o');dep=obj.with_suffix('.d');cmd=['/usr/bin/clang++',*flags,'-MMD','-MF',str(dep),'-c',str(ROOT/'game_lab'/(name+'.cpp')),'-o',str(obj)];subprocess.run(cmd,check=True);commands.append(cmd);objects.append(obj)
  for item in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
   f=Path(item).resolve()
   if ROOT in f.parents:bindings[str(f.relative_to(ROOT))]=sha(f)
 binary=out/'render-cache-tests';cmd=['/usr/bin/clang++',*flags,*map(str,objects),'-framework','CoreGraphics','-framework','CoreFoundation','-lz','-o',str(binary)];subprocess.run(cmd,check=True);commands.append(cmd);runs=[]
 def run(args,env=None):
  x=subprocess.run([str(binary),*args],env=env,check=True,capture_output=True,text=True);assert not x.stderr,x.stderr;rows=[json.loads(s) for s in x.stdout.splitlines()];runs.append({'arguments':args,'results':rows});print(x.stdout,end='',flush=True)
 run([]);clean=dict(os.environ);clean.pop('EMPEROR_GAMEDATA_ROOT',None);run(['--real'],clean);assert 'SKIP' in runs[-1]['results'][0]
 root=os.environ.get('EMPEROR_GAMEDATA_ROOT');inputs={}
 if root:
  names=['Model/EmperorBuildingModels.txt','Save/2026102301/202610230101.sav']+[f'DATA/China_{name}.{ext}'for name in ['General','Terrain','Elevation']for ext in ['sg3','555']];inputs={n:sha(Path(root)/n)for n in names};run(['--real']);assert all(sha(Path(root)/n)==h for n,h in inputs.items())
 assert all(sha(ROOT/n)==h for n,h in bindings.items())
 d={'mode':a.mode,'driver_sha256':sha(__file__),'sources_sha256':bindings,'objects_sha256':{p.name:sha(p)for p in objects},'binary_sha256':sha(binary),'commands':commands,'runs':runs,'read_only_inputs_sha256':inputs,'App_executed':False};(out/'verification.json').write_text(json.dumps(d,indent=2)+'\n')
if __name__=='__main__':main()
