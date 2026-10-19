"""Re-run authored-only independent review. Each compile<=60s, each run<=30s."""
from pathlib import Path
import argparse,concurrent.futures,json,os,subprocess,sys,tempfile
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-root',type=Path,required=True)
parser.add_argument('--out-dir',type=Path)
args=parser.parse_args();src=args.source_root.resolve();tests=Path(__file__).resolve().parent
out=(args.out_dir or Path(tempfile.mkdtemp(prefix='emperor-sequence-independent-'))).resolve()
if out==src or src in out.parents:parser.error('output must be outside source checkout')
out.mkdir(parents=True,exist_ok=True)
env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1');env.pop('EMPEROR_GAMEDATA_ROOT',None)
subprocess.run([sys.executable,'-B',str(tests/'headless_harness.py'),'--source-root',str(src),'--out-dir',str(out)],check=True,timeout=10,env=env)
common=['/usr/bin/clang++','-std=c++17','-I'+str(src),str(src/'game_lab/sg3.cpp'),str(src/'game_lab/asset_pixels.cpp'),str(src/'game_lab/resource_root.cpp'),str(src/'surface_lab/surface.cpp')]
configs=[]
for mode,flags in [('release',['-O2']),('ubsan',['-O1','-g','-fsanitize=undefined,bounds','-fno-sanitize-recover=all','-D_LIBCPP_ENABLE_ASSERTIONS=1'])]:
    configs.append(('independent-'+mode,tests/'independent_sequence.cpp',flags))
    configs.append(('callbacks-'+mode,out/'headless_callbacks.mm',flags+['-fobjc-arc','-framework','Foundation']))
def one(c):
    name,file,flags=c;binary=out/name;command=common+[str(file),*flags,'-o',str(binary)]
    log={'name':name,'compile_command':command}
    try:
        q=subprocess.run(command,capture_output=True,text=True,timeout=60,env=env)
        log.update(compile_exit=q.returncode,compile_stderr=q.stderr)
        if not q.returncode:
            fixtures=out/(name+'-fixtures');fixtures.mkdir(exist_ok=True)
            q=subprocess.run([str(binary),str(fixtures)],capture_output=True,text=True,timeout=20,env=env)
            log.update(exit_code=q.returncode,stdout=q.stdout,stderr=q.stderr)
    except Exception as e:log['error']=str(e)
    (out/(name+'.json')).write_text(json.dumps(log,indent=2));return log
with concurrent.futures.ThreadPoolExecutor(max_workers=2)as pool:logs=list(pool.map(one,configs))
for name,command in [
    ('sav-oracle',[sys.executable,'-B',str(tests/'sav_endian_independent.py'),'--source-root',str(src),'--output',str(out/'sav-endian-oracle.json')]),
    ('sav-existing',[sys.executable,'-B',str(src/'scripts/sav_surface_search_tests.py')])]:
    q=subprocess.run(command,capture_output=True,text=True,timeout=30,env=env,cwd=src)
    logs.append({'name':name,'exit_code':q.returncode,'stdout':q.stdout,'stderr':q.stderr})
passed=all(r.get('exit_code')==0 for r in logs)
(out/'summary.json').write_text(json.dumps({'passed':passed,'source_root':str(src),'authored_only':True,'gui_operations':0,'logs':logs},indent=2))
print(json.dumps({'passed':passed,'output':str(out),'results':[{k:v for k,v in r.items()if k in ('name','exit_code','stdout','stderr','error')}for r in logs]}))
raise SystemExit(0 if passed else 1)
