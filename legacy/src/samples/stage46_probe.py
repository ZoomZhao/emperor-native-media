import json,time,hashlib
from pathlib import Path
from stage37_audio import NativeAudio,R
from stage46_samples import NativeSamples
def main():
 audio=NativeAudio();pool=NativeSamples(audio)
 try:
  driver=audio.open();assert audio.start()==0;status,h=pool.allocate(driver);print(json.dumps(dict(driver=driver,allocate_status=status,handle=h,audio=audio.stats())),flush=True);assert status==0;defaults=pool.description(h);assert defaults['rate']==11025 and defaults['status']==2 and defaults['volume']==127 and defaults['pan']==64 and defaults['loop_count']==1;assert audio.stop()==0
  spec=json.loads((R/'evidence/stage45-button-wav-review.json').read_text());body=Path(spec['copy_path']).read_bytes();assert hashlib.sha256(body).hexdigest()==spec['sha256']
  before=pool.full_snapshot();bad=bytearray(body);bad[20:22]=b'\x11\x00';assert pool.bind(h,0x17000000,bytes(bad))==5 and pool.full_snapshot()==before;assert pool.bind(h,0x17000000,body[:43])==1 and pool.full_snapshot()==before
  assert pool.bind(h,0x17000000,body)==0;_,bound=pool.stats(h);assert bound['frames']==3001 and bound['PCM_bytes']==6002 and bound['data_offset']==44 and bound['rate']==22050 and bound['status']==2
  assert pool.call('start_empty',h)==0;assert pool.call('pull',3000)==0;_,penultimate=pool.stats(h);assert penultimate['status']==4 and penultimate['consumed']==3000 and penultimate['queued']==1
  snapshot=pool.full_snapshot();pool.full_restore(snapshot);assert pool.call('pull',1)==0;_,done=pool.stats(h);assert done['status']==2 and done['consumed']==3001 and done['queued']==0 and done['nonzero']>0 and done['EOS']==1
  assert pool.call('set',h,17,2)==0 and pool.call('start_empty',h)==0 and pool.call('pull',6002)==0;_,twice=pool.stats(h);assert twice['status']==2 and twice['consumed']==6002 and twice['EOS']==1
  assert pool.call('set',h,17,0)==0 and pool.call('start_empty',h)==0 and pool.call('pull',6200)==0;_,loop=pool.stats(h);assert loop['status']==4 and loop['consumed']==6200
  assert pool.call('command',h,1)==0;paused=pool.stats(h)[1];assert pool.call('pull',300)==0 and pool.stats(h)[1]['consumed']==paused['consumed'];assert pool.call('command',h,2)==0 and pool.call('command',h,3)==0
  assert pool.call('init',h)==0 and pool.description(h)==defaults and pool.stats(h)[1]['attached']==0
  assert pool.bind(h,0x17000000,body)==0 and pool.call('start_empty',h)==0;assert audio.start()==0;time.sleep(.24);assert pool.call('service')==0;assert audio.stop()==0;_,actual=pool.stats(h);assert actual['status']==2 and actual['consumed']==3001 and actual['nonzero']>0 and audio.stats()['source_frames']==0
  blob=pool.full_snapshot();audio_blob=audio.snapshot();(R/'runs/stage46-probe-samples.bin').write_bytes(blob);(R/'runs/stage46-probe-audio.bin').write_bytes(audio_blob)
  independent=NativeAudio();restored=NativeSamples(independent)
  try:independent.restore(audio_blob);restored.full_restore(blob);assert restored.full_snapshot()==blob and restored.stats(h)[1]==actual
  finally:restored.dispose();independent.dispose()
  assert pool.call('drop_file',0x17000000)==0 and pool.stats(h)[1]['attached']==0;assert pool.call('release',h)==0 and pool.query(h,2)[0]==1;status,new=pool.allocate(driver);assert status==0 and new!=h;assert pool.stats(new)[1]['attached']==0
  out=dict(defaults=defaults,bound=bound,penultimate=penultimate,done=done,twice=twice,loop=loop,paused=paused,actual_device_clock_consumption=actual,audio=audio.stats(),independent_native_owner_restore=True,malformed_inputs_refused_without_mutation=True,file_drop_and_generation_release=True,source_PCM_sent_to_device=False,original_DLL_exports_executed=False,exact_original_Miles_mixer_equivalence=False)
  (R/'evidence/stage46-PCM-probe.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
 finally:pool.dispose();audio.dispose()
if __name__=='__main__':main()
