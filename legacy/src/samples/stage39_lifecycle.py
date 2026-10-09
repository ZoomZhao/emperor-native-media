#!/usr/bin/env python3
"""One integrated authored empty-pool lifecycle/exhaustion/restore scenario."""
import hashlib,json,subprocess,sys,time
from pathlib import Path
from stage37_audio import NativeAudio,R
from stage39_samples import NativeSamples
def main(restore=False):
 audio=NativeAudio();pool=NativeSamples(audio)
 try:
  if restore:
   spec=json.loads((R/'evidence/stage39-lifecycle-spec.json').read_text());a=(R/'runs/stage39-audio.bin').read_bytes();b=(R/'runs/stage39-samples.bin').read_bytes()
   assert hashlib.sha256(a).hexdigest()==spec['audio_sha256'] and hashlib.sha256(b).hexdigest()==spec['samples_sha256']
   audio.restore(a);pool.restore(b);before=audio.stats();assert audio.start()==0
   for h in spec['handles']:assert pool.description(h)['status']==2 and pool.call('start_empty',h)==0
   time.sleep(.040);assert audio.stop()==0;after=audio.stats()
   assert after['callbacks']>before['callbacks'] and after['source_frames']==0 and after['bad_buffers']==0 and after['render_frames']==after['zero_frames']
   out=dict(ok=True,independent_restore=True,empty_samples_survived=len(spec['handles']),audio_before=before,audio_after=after,original_MSS_sample_executed=False)
   (R/'evidence/stage39-lifecycle-restore.json').write_text(json.dumps(out,indent=2));print(json.dumps(out));return
  driver=audio.open();assert audio.start()==0
  state_before=pool.snapshot();bad,h=pool.allocate(0x5d000101);assert bad==1 and h==0 and pool.snapshot()==state_before
  handles=[]
  for i in range(64):
   status,h=pool.allocate(driver);assert status==0 and h>>24==0x5e;handles.append(h)
   info=pool.description(h);assert info['status']==2 and info['rate']==11025 and info['volume']==127 and info['pan']==64 and info['loop_count']==1 and info['ASI_active']==info['filter_active']==0 and info['mixer_active']==1
  status,h=pool.allocate(driver);assert status==3 and h==0;full_status=status
  old=handles[0];assert pool.call('release',old)==0;status,new=pool.allocate(driver);assert status==0 and new!=old and new&255==old&255;handles[0]=new
  assert pool.query(old,2)[0]==1
  assert pool.call('set',new,23,250)==0 and pool.description(new)['volume']==127
  assert pool.call('set',new,24,0)==0 and pool.description(new)['pan']==0
  assert pool.call('set',new,17,3)==0 and pool.call('set',new,22,22050)==0
  assert pool.call('init',new)==0 and pool.description(new)['loop_count']==1 and pool.description(new)['rate']==11025 and pool.description(new)['pan']==64
  assert pool.call('start_empty',new)==0 and pool.description(new)['status']==2
  assert audio.stop()==0;a=audio.snapshot();b=pool.snapshot();before=audio.stats()
  assert before['source_frames']==0 and before['bad_buffers']==0
  (R/'runs/stage39-audio.bin').write_bytes(a);(R/'runs/stage39-samples.bin').write_bytes(b)
  spec=dict(handles=handles,audio_sha256=hashlib.sha256(a).hexdigest(),samples_sha256=hashlib.sha256(b).hexdigest(),pool_bytes=len(b),audio_bytes=len(a),capacity64_exhaustion_status=full_status,old_handle=hex(old),new_handle=hex(new),wrong_driver_refused_without_state_change=True,empty_start_retains_done_state=True,original_MSS_sample_executed=False)
  (R/'evidence/stage39-lifecycle-spec.json').write_text(json.dumps(spec,indent=2))
  with (R/'evidence/stage39-lifecycle-restore.stdout.log').open('wb')as out,(R/'evidence/stage39-lifecycle-restore.stderr.log').open('wb')as err:
   child=subprocess.run([sys.executable,str(Path(__file__).resolve()),'--restore'],stdout=out,stderr=err,cwd=R,timeout=5)
  assert child.returncode==0,child.returncode
  restored=json.loads((R/'evidence/stage39-lifecycle-restore.json').read_text());assert restored['ok']
  result=dict(ok=True,scope='authored empty native pool, lifecycle/failure/independent restore bound to real silent CoreAudio; not nonempty mixer',spec=spec,independent_restore=restored,no_original_DLL_call=True)
  (R/'evidence/stage39-lifecycle.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
 finally:pool.dispose();audio.dispose()
if __name__=='__main__':main('--restore'in sys.argv)
