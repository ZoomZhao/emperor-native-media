#!/usr/bin/env python3
"""Call actual App transaction methods with controllable GPU completion; no windows."""
from pathlib import Path
import argparse,ast,hashlib,json,os,shlex,subprocess
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--mode',choices=['release','undefined'],default='release');p.add_argument('--output-dir',type=Path,required=True);a=p.parse_args();out=a.output_dir.resolve()
 if out==ROOT or ROOT in out.parents:p.error('output must be outside repository')
 out.mkdir(parents=True,exist_ok=True)
 module=ast.parse((ROOT/'game_lab/app/build_provenance.py').read_text());names=next(ast.literal_eval(x.value) for x in module.body if isinstance(x,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='compiled' for t in x.targets))
 names=['game_lab/app/native_visual_app_tests.mm' if n=='game_lab/app/main.mm' else n for n in names]
 flags=['-std=c++17','-fobjc-arc','-fno-modules','-Wall','-Wextra','-Werror','-mmacosx-version-min=15.0','-I'+str(ROOT),'-I'+str(ROOT/'media_demo'),'-I'+str(ROOT/'file_io/include')]
 flags+=['-O2'] if a.mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all']
 cache_path=out/'object-cache.json';cache=json.loads(cache_path.read_text()) if cache_path.exists() else {}
 bindings={};commands=[];objects=[]
 for i,n in enumerate(names):
  obj=out/f'{i}.o';dep=out/f'{i}.d';cmd=['/usr/bin/clang++',*flags,'-MMD','-MF',str(dep),'-c',str(ROOT/n),'-o',str(obj)]
  old=cache.get(n,{})
  if not (old.get('command')==cmd and obj.exists() and dep.exists() and sha(obj)==old.get('object_sha256') and all((ROOT/k).exists() and sha(ROOT/k)==h for k,h in old.get('sources_sha256',{}).items())):subprocess.run(cmd,check=True)
  objects.append(obj);commands.append(cmd);unit={}
  for token in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
   path=Path(token).resolve()
   if ROOT in path.parents:bindings[str(path.relative_to(ROOT))]=sha(path);unit[str(path.relative_to(ROOT))]=sha(path)
  cache[n]={'command':cmd,'object_sha256':sha(obj),'sources_sha256':unit};cache_path.write_text(json.dumps(cache,indent=2)+'\n')
 frameworks=['AppKit','Foundation','AVFoundation','AudioToolbox','CoreMedia','CoreVideo','ImageIO','CoreGraphics','Metal','QuartzCore']
 binary=out/'native-visual-app-tests';cmd=['/usr/bin/clang++',*flags,*map(str,objects),*[x for f in frameworks for x in ['-framework',f]],'-lz','-o',str(binary)];subprocess.run(cmd,check=True);commands.append(cmd)
 clean=dict(os.environ);clean.pop('EMPEROR_GAMEDATA_ROOT',None);skip=subprocess.run([str(binary)],env=clean,text=True,capture_output=True,check=True);assert 'SKIP' in json.loads(skip.stdout)
 root=os.environ.get('EMPEROR_GAMEDATA_ROOT');inputs={};run=None
 if root:
  data=Path(root).resolve()
  for n in ['Emperor.inf','Model/EmperorBuildingModels.txt','Save/2026102301/202610230101.sav','Save/2026102301/202610230102.sav','DATA/China_General.sg3','DATA/China_General.555','DATA/China_Terrain.sg3','DATA/China_Terrain.555','DATA/China_Elevation.sg3','DATA/China_Elevation.555']:inputs[n]=sha(data/n)
  result=subprocess.run([str(binary)],text=True,capture_output=True);(out/'run.stdout').write_text(result.stdout);(out/'run.stderr').write_text(result.stderr)
  if result.returncode:raise RuntimeError(f'App method test exit {result.returncode}: {result.stderr}')
  run=json.loads(result.stdout);assert run['passed'];assert 'runtime error:' not in result.stderr
  assert all(sha(data/n)==h for n,h in inputs.items());print(result.stdout,end='')
 assert all(sha(ROOT/n)==h for n,h in bindings.items()),'source changed during build; discard this test binding'
 record={'mode':a.mode,'commands':commands,'sources_sha256':bindings,'driver_sha256':sha(__file__),'objects_sha256':{x.name:sha(x) for x in objects},'binary_sha256':sha(binary),'unset_root':json.loads(skip.stdout),'result':run,'real_inputs_sha256':inputs,'original_inputs_read_only':True,'GUI':False,'GPU_executed':False}
 (out/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
 if not run:print(skip.stdout,end='')
if __name__=='__main__':main()
