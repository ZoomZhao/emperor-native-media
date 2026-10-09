"""Build only: does not load the archive or execute its game instructions."""
import argparse,hashlib,json,re,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage80'
def main():
 parser=argparse.ArgumentParser();parser.add_argument('--config',type=Path,default=S/'configs/runtime-build.json');parser.add_argument('--out',type=Path,required=True);args=parser.parse_args();config=json.loads(args.config.read_text());assert config['game_execution_allowed']is False
 for p,digest in config['pinned_inputs'].items():
  h=hashlib.sha256()
  with (ROOT/p).open('rb')as f:
   for b in iter(lambda:f.read(1<<20),b''):h.update(b)
  assert h.hexdigest()==digest,p
 out=args.out.resolve();assert out.is_relative_to(S/'builds');out.mkdir(exist_ok=False);steps=[]
 def run(label,args):
  with(out/(label+'.stdout.log')).open('xb')as a,(out/(label+'.stderr.log')).open('xb')as b:q=subprocess.run([str(x)for x in args],cwd=ROOT,stdout=a,stderr=b,timeout=30)
  steps.append(dict(label=label,returncode=q.returncode,command=[str(x)for x in args]));assert not q.returncode,label
 for src in config['authored_C_sources']:
  run('clang-'+src,['/usr/bin/clang','-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(ROOT/'stage79/tools'),'-I'+str(ROOT/'stage78/tools'),'-c',S/'tools'/(src+'.c'),'-o',out/(src+'.o')])
 objects=[ROOT/p for p in config['frozen_objects']]+list(out.glob('*.o'))
 archive=out/'phase8-runtime-connected-OPEN.a';run('archive',['/usr/bin/ar','rcs',archive]+objects);run('symbols',['/usr/bin/nm','-g',archive]);defined=set();undefined=set()
 for line in(out/'symbols.stdout.log').read_text().splitlines():
  m=re.fullmatch(r'\s*(?:[0-9a-fA-F]+\s+)?([A-Za-z])\s+(\S+)',line)
  if m:(undefined if m[1]=='U'else defined).add(m[2])
 connected=['stage72_step','stage72_guard','stage72_note_write','stage72_lookup_return','stage77_lookup_target','stage78_current_context','stage78_abort_boundary','stage79_active_threads','stage79_thread_boundary'];assert all('_'+n in defined for n in connected)
 c=json.loads((ROOT/'stage79/evidence/known-method-completed-closure.json').read_text());assert not any(q['mnemonic'].startswith('f')for q in c['instructions'])
 result=dict(status='AUTHORED_BRIDGES_COMPILED_WITH_FROZEN_203_FUNCTION_GAME_OBJECT_BUILD_ONLY',steps=steps,game_objects_reused_without_modification=True,connected_bridge_symbols=connected,remaining_external_symbols=sorted(undefined-defined),source_functions=203,source_instructions=7346,source_bytes=20089,x87_instructions_in_selected_closure=0,conditional_cases=21,universal_dynamic_domains=0,private_memory_scalar_REP_FS_callbacks_connected=True,native_code_and_guest_backing_co_placement_proved=False,full_game_executed=False,game_library_loaded=False,phase8_runtime_allowed=False,archive_sha256=hashlib.sha256(archive.read_bytes()).hexdigest())
 (out/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()
