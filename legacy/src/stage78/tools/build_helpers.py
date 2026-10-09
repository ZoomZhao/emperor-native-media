"""Compile own helpers into the prior full guarded archive, without loading it."""
import hashlib,json,re,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage78'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 out=S/'builds/full-helpers-open';out.mkdir(exist_ok=False);steps=[]
 def run(label,command):
  with(out/(label+'.stdout.log')).open('xb') as o,(out/(label+'.stderr.log')).open('xb') as e:q=subprocess.run([str(x) for x in command],stdout=o,stderr=e,cwd=ROOT,timeout=30)
  steps.append(dict(label=label,command=[str(x) for x in command],returncode=q.returncode));assert not q.returncode,label
 for src,obj in [('guarded_helpers.c','guarded-helpers.o'),('helper_frontend.c','helper-frontend.o')]:
  run('compile-'+obj,['/usr/bin/clang','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-logical-op-parentheses','-c',S/'tools'/src,'-o',out/obj])
 previous=[ROOT/'stage77/builds/full-bound-guarded-open/guarded-full.o']+[ROOT/'stage77/builds/heap-integrated-open'/n for n in ['heap-native.o','heap-frontend.o','heap-wrappers.o']]
 run('archive',['/usr/bin/ar','rcs',out/'phase8-helpers-OPEN.a']+previous+[out/'guarded-helpers.o',out/'helper-frontend.o'])
 run('symbols',['/usr/bin/nm','-g',out/'phase8-helpers-OPEN.a'])
 defined=set();undefined=set()
 for line in(out/'symbols.stdout.log').read_text().splitlines():
  m=re.fullmatch(r'\s*(?:[0-9a-fA-F]+\s+)?([A-Za-z])\s+(\S+)',line)
  if m:(undefined if m[1]=='U' else defined).add(m[2])
 unresolved=sorted(undefined-defined)
 r=dict(status='FULL_GUARDED_HEAP_REP_FS_ARCHIVE_BUILT_OPEN',steps=steps,previous_object_hashes={str(p.relative_to(ROOT)):sha(p) for p in previous},archive_sha256=sha(out/'phase8-helpers-OPEN.a'),new_helpers_defined=sorted(x for x in defined if x.startswith('_x86_')),remaining_external_symbols=unresolved,remaining_external_count=len(unresolved),runtime_context_and_abort_hooks_implemented=False,whole_guest_mapping_restored=False,full_CPU_contract_verified_at_phase8_entry=False,REP_intra_element_fault_behavior_verified_against_original=False,SEH_dispatch_unwind_or_RaiseException_implemented=False,complete_runnable_phase8=False,game_library_loaded_or_executed=False)
 (out/'result.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps({k:v for k,v in r.items() if k not in ['steps','previous_object_hashes']}))
if __name__=='__main__':main()
