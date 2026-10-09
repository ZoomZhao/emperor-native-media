# -*- coding: utf-8 -*-
"""Continue adjacent empty sample management; data/provider APIs still refuse."""
from checkpoint_startup38 import AudioRuntime,argparse,ctypes,hashlib,json,struct,time,Path,x,u,R,BASE,restore_state,save_state,decode
from stage7_runtime import Boundary
from stage39_samples import NativeSamples
class SampleRuntime(AudioRuntime):
 extra_snapshot_fields=AudioRuntime.extra_snapshot_fields+['samples_blob39','samples_calls39']
 def __init__(self,save):
  super().__init__(save);self.sample_pool=NativeSamples(self.audio);self.samples_calls39=[]
  self.uc.hook_add(u.UC_HOOK_CODE,self.boot_hook,user_data='2D_audio_initializer_return',begin=0x762050,end=0x762050)
 @property
 def samples_blob39(self):return self.sample_pool.snapshot()
 @samples_blob39.setter
 def samples_blob39(self,b):self.sample_pool.restore(b)
 def api(self,dll,name):
  names={'_AIL_allocate_sample_handle@4':1,'_AIL_init_sample@4':1,'_AIL_release_sample_handle@4':1,'_AIL_sample_status@4':1,'_AIL_sample_loop_count@4':1,'_AIL_set_sample_loop_count@8':2,'_AIL_set_sample_playback_rate@8':2,'_AIL_set_sample_volume@8':2,'_AIL_set_sample_pan@8':2,'_AIL_start_sample@4':1}
  if dll.lower()!='mss32.dll'or name not in names:return super().api(dll,name)
  n=names[name];a=self.args(n);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP));before=self.sample_pool.snapshot();void=False
  if not self.audio_state38['opened']:raise Boundary('Sample_requires_real_open_driver39')
  if name=='_AIL_allocate_sample_handle@4':
   status,value=self.sample_pool.allocate(a[0])
   if status==3:value=0
   elif status!=0:raise Boundary('Native_sample_allocation_refused39',status=status,args=a)
  elif name in ['_AIL_sample_status@4','_AIL_sample_loop_count@4']:
   status,value=self.sample_pool.query(a[0],2 if name=='_AIL_sample_status@4'else 17)
  else:
   void=True;value=self.uc.reg_read(x.UC_X86_REG_EAX)
   if name=='_AIL_init_sample@4':status=self.sample_pool.call('init',a[0])
   elif name=='_AIL_release_sample_handle@4':status=self.sample_pool.call('release',a[0])
   elif name=='_AIL_start_sample@4':status=self.sample_pool.call('start_empty',a[0])
   else:status=self.sample_pool.call('set',a[0],{'_AIL_set_sample_loop_count@8':17,'_AIL_set_sample_playback_rate@8':22,'_AIL_set_sample_volume@8':23,'_AIL_set_sample_pan@8':24}[name],a[1])
  if status not in [0,3]:raise Boundary('Sample_contract_outside_empty_scope39',name=name,args=a,status=status)
  item=dict(name=name,args=a,return_pc=hex(pc),guest_return_value=value,EAX_contract='void/undefined EAX preserved'if void else 'opaque owned native token or observed state',native_status=status,backend='authored arm64 owned sample pool bound to actual Stage37 CoreAudio',scope='empty samples, null ASI/filter; live mixer binding handles only empty input',old_state_sha256=hashlib.sha256(before).hexdigest(),new_state_sha256=hashlib.sha256(self.sample_pool.snapshot()).hexdigest(),original_MSS_sample_code_executed=False)
  if name=='_AIL_allocate_sample_handle@4'and value:item['sample_state']=self.sample_pool.description(value)
  self.samples_calls39.append(item);self.api_trace.append(item);self.ret(value,n)
 def dispose(self):
  if self.sample_pool.state:self.sample_pool.dispose()
  super().dispose()
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='device38');p.add_argument('--tag',default='empty39');p.add_argument('--restore-only',action='store_true');a=p.parse_args();start=time.monotonic();r=SampleRuntime('ST9BASE');r.start_threads(worker_capacity=3);prior=json.loads((R/f'runs/stage12-state-{a.resume}/state.json').read_text())
 if any(q['low']==BASE for q in prior['regions']):r.map_dd()
 r.prepare_heap_restore(prior);r.prepare_resource_restore(prior);m=restore_state(r,a.resume);r.thread_quantum=10000
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 try:
  r.verify_native_geometry()
  if a.restore_only:
   expected=decode(m['host'])['guest_threads20']
   for tid,t in r.threads.items():assert t['context'].__getstate__()[0][40:]==expected[tid]['context'][0][40:]
   handles=[row['guest_return_value']for row in r.samples_calls39 if row['name']=='_AIL_allocate_sample_handle@4'and row['guest_return_value']]
   obj=0x1156bc8;count=r.readword(obj+408);pool=r.readword(obj+412);stored=list(struct.unpack('<8I',bytes(r.uc.mem_read(pool,32))))
   (R/'evidence/stage39-restore-observed.json').write_text(json.dumps(dict(count=count,stored=stored,handles=handles,success_pc_in_visited=0x762048 in r.visited,visited_count=len(r.visited),success_pc_in_boot_events=any(e['pc']=='0x762048'for e in r.boot_events)),indent=2))
   assert count==8 and stored==handles
   out=dict(source=a.resume,all_memory_CPU_four_threads_and_native_blobs_restored=True,sample_snapshot_sha256=hashlib.sha256(r.samples_blob39).hexdigest(),sample_states=[dict(handle=h,state=r.sample_pool.description(h))for h in handles],original_engine_stored_native_handles=stored,original_engine_sample_count=count,original_2D_initializer_observed_returns=[e for e in r.boot_events if e['label']=='2D_audio_initializer_return'],trace_note='sparse mode does not populate general visited set; use dedicated return hook',next_original_args=r.args(3),native_audio=r.audio.stats(),main_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)),native_diagnostic=r.window.diagnostic())
  else:
   if r.audio_state38['opened']:assert r.audio.start()==0
   r.configure_window_callback();r.configure_dd();s=r.thread_run(18,max_steps=200000);r.verify_native_geometry();save_state(r,a.tag,'original startup through actual owned empty sample pool, null ASI/filter and empty mixer binding',m['milestones'])
   out=dict(source=a.resume,state=a.tag,result=s,elapsed_seconds=time.monotonic()-start,samples_calls=r.samples_calls39,native_audio=r.audio.stats(),native_diagnostic=r.window.diagnostic(),boot_events=r.boot_events,original_startup='Unicorn original i386 reference',original_code_patched=False,original_MSS_sample_code_executed=False,nonempty_mixer_or_codec_equivalence_proven=False,main_menu_passed=False,city_logic_passed=False)
  (R/f'evidence/stage39-{a.tag if not a.restore_only else "restore"}.json').write_text(json.dumps(out,indent=2));print(json.dumps(dict(source=a.resume,result=out.get('result'),sample_calls=len(r.samples_calls39),elapsed_seconds=time.monotonic()-start)))
 finally:r.dispose()
if __name__=='__main__':main()
