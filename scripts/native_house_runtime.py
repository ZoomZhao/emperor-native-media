#!/usr/bin/env python3
"""Test native v8 whole runtime on explicit local original sources. Real data only via EMPEROR_GAMEDATA_ROOT.
This independent runner does not modify or invoke existing gameplay runners.
"""
from pathlib import Path
import argparse, hashlib, json, os, shlex, subprocess
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--mode',choices=['release','undefined'],default='release');p.add_argument('--output-dir',type=Path,required=True);p.add_argument('--save',action='append',default=[]);p.add_argument('--soak-ticks',type=int,default=0);a=p.parse_args()
 out=a.output_dir.resolve()
 if out==ROOT or ROOT in out.parents:p.error('output must be outside repository')
 out.mkdir(parents=True,exist_ok=True)
 flags=['-std=c++17','-Wall','-Wextra','-Werror','-I'+str(ROOT/'game_lab')]+(['-O2'] if a.mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all'])
 commands=[];bindings={};objects=[]
 for name in ['native_house_runtime_tests','original_map','original_terrain','original_ground','original_save','original_pak','sg3','resource_root']:
  obj=out/(name+'.o');dep=out/(name+'.d');cmd=['/usr/bin/clang++',*flags,'-MMD','-MF',str(dep),'-c',str(ROOT/'game_lab'/(name+'.cpp')),'-o',str(obj)];subprocess.run(cmd,check=True);commands.append(cmd);objects.append(obj)
  for item in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
   f=Path(item).resolve()
   if ROOT in f.parents:bindings[str(f.relative_to(ROOT))]=sha(f)
 binary=out/'native-house-runtime-tests';cmd=['/usr/bin/clang++',*flags,*map(str,objects),'-framework','CoreGraphics','-framework','CoreFoundation','-lz','-o',str(binary)];subprocess.run(cmd,check=True);commands.append(cmd)
 runs=[]
 def run(args,env=None):
  x=subprocess.run([str(binary),*args],env=env,capture_output=True,text=True)
  prefix=out/("run-"+str(len(runs)))
  prefix.with_suffix(".stdout").write_text(x.stdout);prefix.with_suffix(".stderr").write_text(x.stderr)
  if x.returncode or x.stderr:raise RuntimeError("runtime test exit "+str(x.returncode)+": "+x.stderr)
  rows=[json.loads(line) for line in x.stdout.splitlines()];runs.append({'arguments':args,'results':rows});print(x.stdout,end='',flush=True)
 clean=dict(os.environ);clean.pop('EMPEROR_GAMEDATA_ROOT',None);run([],clean);assert 'SKIP' in runs[-1]['results'][0]
 root=os.environ.get('EMPEROR_GAMEDATA_ROOT');inputs={}
 for index,name in enumerate(a.save):
  if root:
   f=(Path(root)/name).resolve()
   if Path(name).is_absolute() or Path(root).resolve() not in f.parents:raise ValueError('source outside data root')
   inputs[name]=sha(f)
  target=out/('source-'+str(index));target.mkdir(exist_ok=False)
  run([name,str(target)])
 assert all(sha(Path(root)/n)==v for n,v in inputs.items())
 assert all(sha(ROOT/n)==v for n,v in bindings.items())
 result={'suite':'native-v8-house-runtime','mode':a.mode,'driver_sha256':sha(__file__),'commands':commands,'sources_sha256':bindings,'objects_sha256':{f.name:sha(f) for f in objects},'binary_sha256':sha(binary),'runs':runs,'real_inputs_sha256':inputs,'App_executed':False}
 (out/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
