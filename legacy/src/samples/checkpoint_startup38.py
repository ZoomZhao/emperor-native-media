# -*- coding: utf-8 -*-
"""Advance only measured Miles observable startup + real host output device.

Provider/mixer/sample APIs remain fail-closed. Does not run original Miles code.
"""
from checkpoint_startup35 import ContractRuntime,argparse,ctypes,hashlib,json,struct,time,Path,x,u,R,BASE,restore_state,save_state,decode
from stage7_runtime import Boundary
from stage38_miles import NativeMiles
from stage37_audio import NativeAudio
class AudioRuntime(ContractRuntime):
 extra_snapshot_fields=ContractRuntime.extra_snapshot_fields+['miles38_calls','audio_state38','audio_blob38']
 def __init__(self,save):
  super().__init__(save);self.miles.dispose();self.miles=NativeMiles();self.audio=NativeAudio();self.miles38_calls=[];self.audio_state38=dict(driver=0,opened=False,needs_start=False)
 @property
 def audio_blob38(self):
  if not self.audio_state38['opened']:return None
  if self.audio.stats()['phase']==1:assert self.audio.stop()==0
  return self.audio.snapshot()
 @audio_blob38.setter
 def audio_blob38(self,b):
  if b is None:return
  self.audio.restore(b)
 def api(self,dll,name):
  if dll.lower()!='mss32.dll':return super().api(dll,name)
  argc={'_AIL_startup@0':0,'_AIL_set_preference@8':2,'_AIL_get_preference@4':1,'_AIL_waveOutOpen@16':4}.get(name)
  if argc is None:raise Boundary('Miles_next_contract_unimplemented38',name=name,reason='Measured startup/preferences/output-device subset only; sample/provider/mixer/stream/3D semantics remain unbound')
  a=self.args(argc);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));before=self.miles.export();item=dict(name=name,args=a,return_pc=hex(pc),backend='self-authored native arm64 state + real CoreAudio',provider_registry_equivalence=False)
  if name=='_AIL_startup@0':
   reference=json.loads((R/'evidence/stage38-MSS-decoded.json').read_text());assert reference['probe_completed'] and reference['cold_repeated_restart_startup']==[1,0,1]
   value=self.miles.startup();item.update(preference42_after=self.miles.get(42),scope='observed startup guard and preference reset; no codec/provider API exposed')
  elif name=='_AIL_set_preference@8':
   if a[0]not in [15,40,42]:raise Boundary('Miles_preference_index_unreviewed38',args=a)
   value=self.miles.preference(*a)
  elif name=='_AIL_get_preference@4':
   if a[0]!=42:raise Boundary('Miles_get_preference_unreviewed38',args=a)
   value=self.miles.get(a[0])
  else:
   output,hwo,device,wfx=a
   if not output or hwo or device!=0xffffffff or not wfx or self.audio_state38['opened']:raise Boundary('Miles_waveOutOpen_parameters_outside_measured_scope38',args=a)
   self.require_region(output,4,u.UC_PROT_READ|u.UC_PROT_WRITE,'Miles driver DWORD');self.require_region(wfx,16,u.UC_PROT_READ,'16-byte PCMWAVEFORMAT')
   fields=list(struct.unpack('<HHIIHH',bytes(self.uc.mem_read(wfx,16))))
   if fields!=[1,2,22050,88200,4,16]:raise Boundary('Miles_waveOutOpen_PCM_unreviewed38',format=fields)
   state=struct.unpack('<64I',self.miles.export())
   if state[48]!=1 or state[2+40]!=262144 or state[2+15]!=0:raise Boundary('Miles_waveOutOpen_requires_reviewed_startup_preferences38')
   driver=self.audio.open();assert self.audio.start()==0
   self.uc.mem_write(output,struct.pack('<I',driver));self.audio_state38=dict(driver=driver,opened=True,needs_start=True)
   value=0;item.update(PCM=fields,driver_token32=hex(driver),actual_native_device=self.audio.stats()['device'],actual_AudioUnit_initialized_started=True,reference_return=0)
  item.update(return_value=value,native_state_before_sha256=hashlib.sha256(before).hexdigest(),native_state_after_sha256=hashlib.sha256(self.miles.export()).hexdigest());self.miles38_calls.append(item);self.api_trace.append(item);self.ret(value,argc)
 def dispose(self):
  if self.audio.state:self.audio.dispose()
  super().dispose()
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='final35');p.add_argument('--tag',default='device38');p.add_argument('--restore-only',action='store_true');a=p.parse_args();start=time.monotonic();r=AudioRuntime('ST9BASE');r.start_threads(worker_capacity=3);prior=json.loads((R/f'runs/stage12-state-{a.resume}/state.json').read_text())
 if any(q['low']==BASE for q in prior['regions']):r.map_dd()
 r.prepare_heap_restore(prior);r.prepare_resource_restore(prior);m=restore_state(r,a.resume);r.thread_quantum=10000
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 try:
  r.verify_native_geometry()
  if a.restore_only:
   expected=decode(m['host'])['guest_threads20']
   for tid,t in r.threads.items():assert t['context'].__getstate__()[0][40:]==expected[tid]['context'][0][40:]
   out=dict(source=a.resume,all_memory_CPU_four_threads_and_native_blobs_restored=True,audio_state=r.audio_state38,audio_native_sha256=hashlib.sha256(r.audio_blob38).hexdigest()if r.audio_state38['opened']else None,audio_stats=r.audio.stats(),main_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)),native_diagnostic=r.window.diagnostic())
  else:
   if r.audio_state38['opened']:assert r.audio.start()==0
   r.configure_window_callback();r.configure_dd();s=r.thread_run(18,max_steps=200000);r.verify_native_geometry();save_state(r,a.tag,'original startup through measured Miles guard/preference and real CoreAudio output device; unbound sample APIs refuse',m['milestones'])
   out=dict(source=a.resume,state=a.tag,result=s,elapsed_seconds=time.monotonic()-start,miles38_calls=r.miles38_calls,audio_state=r.audio_state38,audio_stats=r.audio.stats(),native_diagnostic=r.window.diagnostic(),boot_events=r.boot_events,original_startup='Unicorn original i386 unpatched instructions; not full native WinMain',original_MSS_code_executed_in_runtime=False,original_IJL_code_executed=False,original_code_patched=False,provider_registry_equivalence=False,main_menu_passed=False,city_logic_passed=False)
  (R/f'evidence/stage38-{a.tag if not a.restore_only else "restore"}.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
 finally:r.dispose()
if __name__=='__main__':main()
