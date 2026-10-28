#!/usr/bin/env python3
"""Concentrated fresh-start checks; real data only via EMPEROR_GAMEDATA_ROOT."""
from pathlib import Path
import argparse,hashlib,json,os,shlex,subprocess
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output-dir',type=Path,required=True);p.add_argument('--mode',choices=['release','undefined'],default='release');p.add_argument('--save',action='append',default=[]);p.add_argument('--plan-only',action='store_true');a=p.parse_args();out=a.output_dir.resolve()
 if ROOT==out or ROOT in out.parents or out.exists():p.error('a new output directory outside repository is required')
 out.mkdir(parents=True);flags=['-std=c++17','-Wall','-Wextra','-Werror','-I'+str(ROOT/'game_lab')]+(['-O2']if a.mode=='release'else['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all']);objects=[];sources={};commands=[]
 for name in ['native_start_delivery_tests','original_map','original_terrain','original_ground','original_save','original_pak','sg3','resource_root']:
  obj=out/(name+'.o');dep=out/(name+'.d');cmd=['/usr/bin/clang++',*flags,'-MMD','-MF',str(dep),'-c',str(ROOT/'game_lab'/(name+'.cpp')),'-o',str(obj)];subprocess.run(cmd,check=True);commands.append(cmd);objects.append(str(obj))
  for s in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
   f=Path(s).resolve()
   if ROOT in f.parents:sources[str(f.relative_to(ROOT))]=sha(f)
 binary=out/'native-start-tests';cmd=['/usr/bin/clang++',*flags,*objects,'-framework','CoreGraphics','-framework','CoreFoundation','-lz','-o',str(binary)];subprocess.run(cmd,check=True);commands.append(cmd)
 clean=dict(os.environ);clean.pop('EMPEROR_GAMEDATA_ROOT',None);unset=subprocess.run([str(binary)],env=clean,capture_output=True,text=True,check=True);assert 'SKIP'in unset.stdout;rows=[];inputs={};root=os.environ.get('EMPEROR_GAMEDATA_ROOT')
 for i,save in enumerate(a.save):
  if root:
   f=(Path(root)/save).resolve();assert not Path(save).is_absolute()and Path(root).resolve()in f.parents;inputs[save]=sha(f)
  dest=out/('source-'+str(i));args=[str(binary),save,str(dest)]+(['--plan-only']if a.plan_only else[])
  with(out/f'source-{i}.stdout').open('x')as o,(out/f'source-{i}.stderr').open('x')as e:x=subprocess.run(args,stdout=o,stderr=e)
  rows.append({'source':save,'exit_code':x.returncode,'output':str(dest)});print(json.dumps(rows[-1]),flush=True);assert x.returncode==0,(out/f'source-{i}.stderr').read_text()
 assert all(sha(ROOT/n)==h for n,h in sources.items());assert all(sha(Path(root)/n)==h for n,h in inputs.items())
 (out/'verification.json').write_text(json.dumps({'passed':True,'mode':a.mode,'commands':commands,'sources_sha256':sources,'binary_sha256':sha(binary),'driver_sha256':sha(__file__),'unset_root_SKIP':True,'inputs_sha256':inputs,'runs':rows,'GUI':False,'original_game_run':False},indent=2)+'\n')
if __name__=='__main__':main()
