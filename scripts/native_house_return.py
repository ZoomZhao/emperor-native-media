#!/usr/bin/env python3
"""Independent native Well return tests; real inputs are read-only and outputs external."""
from pathlib import Path
from datetime import datetime, timezone
import argparse, hashlib, json, os, shlex, subprocess, sys
ROOT=Path(__file__).resolve().parents[1]
def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def deps(path):
 text=Path(path).read_text().replace(chr(92)+chr(10),' ')
 return [Path(x).resolve() for x in shlex.split(text.split(':',1)[1])]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--mode',choices=['release','undefined'],default='release');p.add_argument('--output-dir',type=Path,required=True);p.add_argument('--save',action='append',default=[]);p.add_argument('--checkpoint',type=Path,action='append',default=[]);p.add_argument('--test-mode',choices=['local','soak'],default='local');p.add_argument('--max-steps',type=int,default=10000);p.add_argument('--reuse-build',type=Path);p.add_argument('--run-timeout',type=int,default=7200);a=p.parse_args()
 out=a.output_dir.resolve()
 if out==ROOT or ROOT in out.parents: p.error('output must be outside repository')
 if out.exists(): p.error('output directory must be new to preserve previous evidence')
 if len(a.save)!=len(a.checkpoint): p.error('one checkpoint required per source')
 if not 0<a.max_steps<=10000:p.error('max steps1..10000')
 out.mkdir(parents=True);m={'suite':'native-house-return','mode':a.mode,'test_mode':a.test_mode,'started_utc':datetime.now(timezone.utc).isoformat(),'driver_sha256':sha(__file__),'commands':[],'runs':[],'input_hashes':{},'dependency_hashes':{},'App_executed':False,'ASan_executed':False}
 def save(): (out/'verification.json').write_text(json.dumps(m,indent=2)+'\n')
 def command(argv,name,timeout,env=None,stream=False):
  m['commands'].append(argv);save();stdout=out/(name+'.stdout');stderr=out/(name+'.stderr')
  with stdout.open('w') as so,stderr.open('w') as se:
   proc=subprocess.Popen(argv,stdout=so,stderr=se,env=env)
   try:code=proc.wait(timeout=timeout)
   except subprocess.TimeoutExpired:proc.terminate();proc.wait(timeout=15);code=-1;m['timeout']=name
  m['runs'].append({'name':name,'returncode':code,'stdout':str(stdout),'stderr':str(stderr)});save()
  if code:raise RuntimeError(name+' exit '+str(code)+': '+stderr.read_text()[-4000:])
  if stream:print(stdout.read_text(),end='',flush=True)
 try:
  data=os.environ.get('EMPEROR_GAMEDATA_ROOT')
  if not data:m['SKIP']='EMPEROR_GAMEDATA_ROOT unset';save();print(json.dumps({'SKIP':m['SKIP']}));return 0
  data=Path(data).resolve();names=['Model/EmperorBuildingModels.txt','Model/EmperorFigureModels.txt','EmperorText.eng']+['DATA/'+x for x in ['China_General.sg3','China_Terrain.sg3','China_Elevation.sg3','SprAmbient.sg3','SprMain.sg3','SprMain2.sg3','SprMain3.sg3']]
  for name in [*names,*a.save]:
   f=(data/name).resolve()
   if f==data or data not in f.parents:raise ValueError('input outside authorized data root')
   m['input_hashes'][str(f)]=sha(f)
  for f in a.checkpoint:m['input_hashes'][str(f.resolve())]=sha(f)
  flags=['-std=c++17','-Wall','-Wextra','-Werror','-I'+str(ROOT/'game_lab')]+(['-O2'] if a.mode=='release' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all'])
  if a.reuse_build:
   prior=json.loads((a.reuse_build/'verification.json').read_text());assert prior['mode']==a.mode
   for f,h in prior['dependency_hashes'].items():assert sha(f)==h,'changed dependency '+f
   binary=Path(prior['binary']);assert sha(binary)==prior['binary_sha256'];m['dependency_hashes']=prior['dependency_hashes'];m['reused_build']=str(a.reuse_build)
  else:
   objects=[]
   for name in ['native_house_return_tests','original_map','original_terrain','original_ground','original_save','original_pak','sg3','resource_root']:
    obj=out/(name+'.o');dep=out/(name+'.d');print('compile '+name,flush=True)
    command(['/usr/bin/clang++',*flags,'-MMD','-MF',str(dep),'-c',str(ROOT/'game_lab'/(name+'.cpp')),'-o',str(obj)],'compile-'+name,60);objects.append(obj)
    for f in deps(dep):
     if ROOT in f.parents:m['dependency_hashes'][str(f)]=sha(f)
   binary=out/'native-house-return-tests';command(['/usr/bin/clang++',*flags,*map(str,objects),'-framework','CoreGraphics','-framework','CoreFoundation','-lz','-o',str(binary)],'link',60)
   m['objects_sha256']={str(f):sha(f) for f in objects}
  m['binary']=str(binary);m['binary_sha256']=sha(binary);save()
  clean=dict(os.environ);clean.pop('EMPEROR_GAMEDATA_ROOT',None);command([str(binary)],'skip',60,clean);assert 'SKIP' in (out/'skip.stdout').read_text()
  for i,(source,checkpoint) in enumerate(zip(a.save,a.checkpoint)):
   target=out/('source-'+str(i));target.mkdir();print('run '+a.test_mode+' '+source,flush=True)
   command([str(binary),source,str(checkpoint.resolve()),str(target),a.test_mode,str(a.max_steps)],'source-'+str(i),a.run_timeout,stream=True)
  m['passed']=True;return 0
 except Exception as e:m['passed']=False;m['error']=str(e);print(str(e),file=sys.stderr,flush=True);return 1
 finally:
  m['inputs_unchanged']=all(Path(f).exists() and sha(f)==h for f,h in m['input_hashes'].items());m['dependencies_unchanged']=all(Path(f).exists() and sha(f)==h for f,h in m['dependency_hashes'].items());m['finished_utc']=datetime.now(timezone.utc).isoformat();save()
if __name__=='__main__':sys.exit(main())
