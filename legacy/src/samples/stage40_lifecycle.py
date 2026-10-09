import hashlib,json,subprocess,sys
from stage37_audio import NativeAudio,R
from stage40_registry import NativeRegistry
def main(child=False):
 audio=NativeAudio();registry=NativeRegistry(audio)
 try:
  if child:
   spec=json.loads((R/'evidence/stage40-lifecycle-spec.json').read_text());a=(R/'runs/stage40-own-audio.bin').read_bytes();b=(R/'runs/stage40-own-registry.bin').read_bytes();assert hashlib.sha256(a).hexdigest()==spec['audio_sha256']and hashlib.sha256(b).hexdigest()==spec['registry_sha256'];audio.restore(a);registry.restore(b);result,cursor,h=registry.enumerate(None,0);assert (result,cursor)==(1,1)and h==spec['handle'];assert registry.name(h)==(0,spec['name']);assert registry.enumerate(b'MSS 3D audio services',0)==(0,0,0);out=dict(ok=True,independent_native_restore=True,capabilities=registry.capabilities(),Name_callback_restored=True,original_provider_executed=False);(R/'evidence/stage40-lifecycle-restore.json').write_text(json.dumps(out,indent=2));print(json.dumps(out));return
  assert registry.start()==2;audio.open();assert audio.start()==0;assert registry.start()==0
  result,cursor,h=registry.enumerate(None,0);assert (result,cursor)==(1,1);status,name=registry.name(h);assert status==0 and name=='Authored macOS CoreAudio PCM queue';assert registry.enumerate(None,cursor)==(0,0,0)
  before=registry.snapshot();assert registry.fn['register_mss3d'](registry.state)==5 and registry.snapshot()==before;assert registry.enumerate(b'MSS 3D audio services',0)==(0,0,0)
  assert registry.shutdown()==0 and registry.name(h)[0]==1;assert registry.start()==0;_,_,new=registry.enumerate(None,0);assert new!=h and registry.name(h)[0]==1
  assert audio.stop()==0;a=audio.snapshot();b=registry.snapshot();(R/'runs/stage40-own-audio.bin').write_bytes(a);(R/'runs/stage40-own-registry.bin').write_bytes(b)
  spec=dict(handle=new,name=name,old_handle=h,audio_sha256=hashlib.sha256(a).hexdigest(),registry_sha256=hashlib.sha256(b).hexdigest(),registry_bytes=len(b),capabilities=registry.capabilities(),unimplemented_3D_registration_refused_without_state_change=True)
  (R/'evidence/stage40-lifecycle-spec.json').write_text(json.dumps(spec,indent=2))
  with (R/'evidence/stage40-lifecycle-restore.stdout.log').open('wb')as o,(R/'evidence/stage40-lifecycle-restore.stderr.log').open('wb')as e:result=subprocess.run([sys.executable,__file__,'--restore'],stdout=o,stderr=e,timeout=5)
  assert result.returncode==0;out=dict(ok=True,spec=spec,restore=json.loads((R/'evidence/stage40-lifecycle-restore.json').read_text()),original_DLL_code_executed=False);(R/'evidence/stage40-lifecycle.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
 finally:registry.dispose();audio.dispose()
if __name__=='__main__':main('--restore'in sys.argv)
