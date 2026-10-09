import hashlib,json,tempfile
from pathlib import Path
from stage37_audio import NativeAudio
from stage42_bink import NativeBink
R=Path(__file__).resolve().parents[1];a=NativeAudio();b=None
try:
 a.open();assert a.start()==0 and a.stop()==0;b=NativeBink(a)
 # Independent static original evidence: NULL factory fails; nonzero driver
 # must resolve the minimum-buffer-size entry, which this real pool lacks.
 assert b.set_sound(0,a.driver)[0]==1
 status,result=b.set_sound(0x61001a70,a.driver);assert(status,result)==(0,0)
 caps=b.capabilities();assert caps['real_audio_live']==caps['real_sample_allocate_entry']==1 and caps['real_minimum_buffer_size_entry']==caps['sound_open_registered']==0 and caps['factory_calls']==1
 blob=b.snapshot();assert b.set_sound(0x61001a70,a.driver)==(0,0)and b.capabilities()['factory_calls']==2;b.restore(blob);assert b.capabilities()['factory_calls']==1
 with tempfile.TemporaryDirectory(prefix='stage42-own-files-',dir=R/'runs')as root:
  root=Path(root);(root/'Binks').mkdir();assert b.open_missing(root,'Binks/emperor_intro_hiq.bik',0x08000000)==(0,0);assert b.error()==b'Error opening file.'and b.capabilities()['readonly_IO_attempts']==2 and b.capabilities()['last_errno']==2;missing_blob=b.snapshot()
  (root/'Binks/emperor_intro_hiq.bik').write_bytes(b'Authored present-file fixture; not a game movie.\n')
  assert b.open_missing(root,'Binks/emperor_intro_hiq.bik',0x08000000)[0]==5 and b.capabilities()['last_errno']==0
  # Existing input must refuse decoding rather than pretend to be missing.
  b.restore(missing_blob);assert b.error()==b'Error opening file.'and b.snapshot()==missing_blob
 out=dict(actual_native_backend_checked=True,mandatory_entry_real_lookup_absent=True,unknown_factory_rejected=True,actual_factory_callback_invoked=True,failed_registration_does_not_make_sound_open_table=True,existing_input_refuses_instead_of_fabricating_missing=True,real_missing_file_two_readonly_attempts_error_errno_verified=True,snapshot_restore_rebinds_actual_callback_and_module=True,image_bytes=len(b.snapshot()),image_sha256=hashlib.sha256(b.snapshot()).hexdigest(),original_Bink_or_Miles_executed=False,native_capabilities=b.capabilities(),source_frames=a.stats()['source_frames'])
 (R/'evidence/stage42-lifecycle.json').write_text(json.dumps(out,indent=2));print(json.dumps(out))
finally:
 if b:b.dispose()
 a.dispose()
