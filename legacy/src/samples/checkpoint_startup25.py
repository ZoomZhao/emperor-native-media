#!/usr/bin/env python3
"""Continue ordered original WinMain with a native, scoped Miles preference ABI."""
import argparse,json,time,hashlib,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from checkpoint_startup22 import ProfileRuntime,x,u
from stage7_runtime import Boundary
from stage12_state import save_state,restore_state,decode
from stage25_miles import NativeMiles
R=Path(__file__).resolve().parents[1]
class MilesRuntime(ProfileRuntime):
 extra_snapshot_fields=ProfileRuntime.extra_snapshot_fields+['miles_blob','miles_calls']
 def __init__(self,save):super().__init__(save);self.miles=NativeMiles();self.miles_calls=[];self.miles_mode='attached NT pre-startup, debug off, legacy dispatcher absent'
 @property
 def miles_blob(self):return self.miles.export()
 @miles_blob.setter
 def miles_blob(self,b):self.miles.restore(b)
 def api(self,dll,name):
  if dll.lower()=='mss32.dll'and name=='_AIL_set_preference@8':
   a=self.args(2);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))
   if a[0]!=42:raise Boundary('Miles_preference_index_unreviewed',args=a,return_pc=hex(pc))
   before=self.miles.export();old=self.miles.preference(*a);after=self.miles.export();decoded=struct.unpack('<64I',after);assert decoded[44]==a[1]and decoded[46]==1 and decoded[48:51]==(0,0,0)
   item=dict(dll=dll,name=name,args=a,return_pc=hex(pc),return_value=old,current_preference42=decoded[44],previous_state_sha256=hashlib.sha256(before).hexdigest(),next_state_sha256=hashlib.sha256(after).hexdigest(),backend='authored native arm64 pthread mutex + preference exchange, original DLL not executed',scope=self.miles_mode)
   self.miles_calls.append(item);self.api_trace.append(item);self.ret(old,2);return
  if dll.lower()=='mss32.dll':raise Boundary('Miles_semantics_not_implemented',name=name,reason='Startup/providers/drivers/timers/audio/callbacks cannot be reported successful by preference adapter')
  super().api(dll,name)
 def dispose(self):self.miles.dispose();super().dispose()
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='profile22');p.add_argument('--tag',default='miles25');p.add_argument('--restore-only',action='store_true');a=p.parse_args();start=time.monotonic();r=MilesRuntime('ST9BASE');r.start_threads(worker_capacity=3);m=restore_state(r,a.resume);r.thread_quantum=10000
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 if a.resume=='profile22':assert not r.miles_calls and not any(q.get('dll','').lower()=='mss32.dll'and q.get('return_value')is not None for q in r.api_trace)
 if a.restore_only:
  expected=decode(m['host'])['guest_threads20'];payloads={}
  for tid,t in r.threads.items():
   original=expected[tid]['context'][0];actual=t['context'].__getstate__()[0];assert actual[40:]==original[40:];payloads[tid]=hashlib.sha256(actual[40:]).hexdigest()
  record=dict(source=a.resume,all_native_Miles_state_restored=True,miles_state_sha256=hashlib.sha256(r.miles.export()).hexdigest(),miles_calls=r.miles_calls,main_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)),entire_CPU_and_memory_restored=True,all_four_guest_context_payloads_restored=payloads)
  (R/'evidence/stage25-restore.json').write_text(json.dumps(record,indent=2));print(json.dumps(record));r.dispose();return
 s=r.thread_run(25,max_steps=200000);save_state(r,a.tag,'ordered original WinMain with native pre-startup Miles preference state',m['milestones'])
 out=dict(source=a.resume,state=a.tag,result=s,elapsed_seconds=time.monotonic()-start,miles_calls=r.miles_calls,boot_events=r.boot_events,threads=r.thread_summary(),main_return_PC_after_preference='0x4755d1',native_adapter='arm64 dylib',original_game_startup_execution='Unicorn i386 original bytes, not native WinMain',original_DLL_code_executed=False,original_code_patched=False,full_original_resource_initializer_returned=False,city_root_returned=False,city_cycles=0)
 (R/f'evidence/stage25-{a.tag}.json').write_text(json.dumps(out,indent=2));print(json.dumps({k:v for k,v in out.items()if k not in ['threads','boot_events']}));r.dispose()
if __name__=='__main__':main()
