import json,struct,subprocess,sys
sys.dont_write_bytecode=True
from common import ROOT,S,put
def main():
 out=S/'builds/negative-v2';out.mkdir(exist_ok=False);p=ROOT/'stage82/runs/fixture-input';(out/'blocked-lock.bin').write_bytes(b'S82L'+struct.pack('<4I',1,0xa1000400,1,2));(out/'own-api-sites.bin').write_bytes(b'S82I'+struct.pack('<5I',1,0xa0000380,0xa1000010,0xa0000860,2))
 inc=['-I'+str(ROOT/x)for x in ('stage82/tools','stage80/tools','stage79/tools','stage78/tools','stage77/tools')];sources=[ROOT/x for x in ('stage82/tools/negative_selftest.c','stage82/tools/runtime.c','stage82/tools/platform_native.c','stage82/tools/heap_connection.c','stage79/tools/guest_memory.c','stage79/tools/thread_native.c','stage77/tools/heap_native.c')]
 cmd=['/usr/bin/clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-logical-op-parentheses','-fsanitize=undefined',*inc,*map(str,sources),'-o',str(out/'negative-selftest')]
 with(out/'compile.stdout.log').open('xb')as o,(out/'compile.stderr.log').open('xb')as e:q=subprocess.run(cmd,stdout=o,stderr=e,timeout=30)
 assert not q.returncode
 q=subprocess.run([str(out/'negative-selftest'),str(p/'threads.bin'),str(p/'regions.bin'),str(p/'memory.bin'),str(p/'heap.bin'),str(out/'blocked-lock.bin'),str(out/'result.json'),str(out/'own-api-sites.bin')],timeout=30);assert not q.returncode
 from pipeline import gate,worker
 c=json.loads((S/'configs/original.json').read_text());c['_config']=str(S/'configs/original.json')
 failures=[]
 for mode in ('root_without_approval','worker_without_root_claim'):
  try:
   if mode=='root_without_approval':gate(c,S/'configs/original.json',None,S/'runs/NEVER_CREATED')
   else:worker(c,'capture',ROOT/c['source'],S/'runs/NEVER_CREATED/capture',None)
  except Exception as e:failures.append({'mode':mode,'rejected':str(e)})
 assert len(failures)==2 and 'unicorn'not in sys.modules and not list((S/'evidence').glob('original-*-attempt.claim.json'))and not(S/'runs/NEVER_CREATED').exists()
 put(S/'evidence/execution-gate-negative-tests-v2.json',{'status':'ORIGINAL_GATE_REFUSED_BEFORE_ENGINE_IMPORT_OR_CLAIM','cases':failures,'original_game_instructions':0});print('12 authored C boundary/binding cases + 2 original-gate denial tests passed')
if __name__=='__main__':main()
