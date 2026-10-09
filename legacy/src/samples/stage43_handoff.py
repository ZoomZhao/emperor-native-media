import datetime,hashlib,json
from pathlib import Path
R=Path(__file__).resolve().parents[1];cache={};checks=[]
def read(n):return json.loads((R/'evidence'/n).read_text())
def sha(p):
 p=Path(p)
 if str(p)not in cache:
  h=hashlib.sha256()
  with p.open('rb')as f:
   for b in iter(lambda:f.read(1048576),b''):h.update(b)
  cache[str(p)]=h.hexdigest()
 return cache[str(p)]
def check(p,wanted):
 got=sha(p);checks.append(dict(path=str(p),expected_sha256=wanted,sha256=got,unchanged=got==wanted))
prior=read('stage42-postflight.json')
for q in prior['checks']:check(q['path'],q['sha256'])
for q in read('stage42-result.json')['current_checkpoint_files']:check(R/q['path'],q['sha256'])
for n,source in [('stage43-build.json','stage43_input.m'),('stage43-cursor-build.json','stage43_cursor.m'),('stage43-pump-build.json','stage43_pump.m'),('stage43-window-build.json','stage43_window.m')]:
 q=read(n);check(q['library'],q['sha256']);check(R/'samples'/source,q['source_sha256'])
check(R/'samples/stage43_window.py',read('stage43-window-build.json')['loader_sha256'])
for q in read('stage43-stream-contract.json')['music_files']:check(q['path'],q['sha256'])
absences=[]
for q in prior['absences']:
 p=Path(q['path']);absences.append(dict(path=str(p),still_missing=not p.exists()and(not p.parent.is_dir()or not any(c.name.casefold()==p.name.casefold()for c in p.parent.iterdir()))))
links=[]
for q in prior['DOS_namespace_symlinks']:
 p=Path(q['path']);target=str(p.readlink());links.append(dict(path=str(p),target=target,unchanged=target==q['target']));assert target==q['target']
audit=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),checks=checks,hash_checks=len(checks),unique_paths=len(cache),all_frozen_inputs_unchanged=all(q['unchanged']for q in checks),absences=absences,all_observed_missing_inputs_still_missing=all(q['still_missing']for q in absences),DOS_namespace_symlinks=links)
(R/'evidence/stage43-postflight.json').write_text(json.dumps(audit,ensure_ascii=False,indent=2));assert audit['all_frozen_inputs_unchanged']and audit['all_observed_missing_inputs_still_missing']
j=read('stage43-final43.json');rest=read('stage43-restore.json');assert rest['source']=='final43'and rest['all_memory_CPU_four_threads_and_native_blobs_restored'];assert j['result']['boundary']['name']=='_AIL_open_stream@12'
assert j['native_audio']['source_frames']==0 and all(q['client']==q['roundtrip_client']for q in j['coordinate_roundtrip_cases'])
for q in [j['native_before'],j['native_diagnostic'],j['window_capture']['native_after'],rest['native_diagnostic']]:assert q['NSApp_isActive']==q['key_window']==q['activation_requests']==0 and q['frontmost_pid']!=q['current_pid']
assert j['startup_counts'].get('original_city_scheduler_entry',0)==0 and j['startup_counts'].get('city_load_root_entry',0)==0
d=R/'runs/stage12-state-final43';state=json.loads((d/'state.json').read_text());files=[dict(path=str(p.relative_to(R)),bytes=p.stat().st_size,sha256=sha(p))for p in sorted(d.iterdir())if p.is_file()]
labels=['stage43-build','stage43-input-startup','stage43-cursor-build','stage43-cursor-startup','stage43-refresh-static','stage43-nested-pump-static','stage43-pump-build','stage43-nested-pump','stage43-background-window-build','stage43-background-startup','stage43-music-metadata','stage43-background-prohibited-build','stage43-background-probe','stage43-stream-caller-static','stage43-final-startup','stage43-final-restore','stage43-stream-contract']
out=dict(created_utc=audit['created_utc'],outcome='Readonly actual cursor and native window coordinates + original nested queue continuation reach real menu MP3 open; menu and city remain unproven',current_checkpoint=str(d.relative_to(R)),current_checkpoint_files=files,main_PC=hex(state['registers']['eip']),mapped_memory_bytes=state['memory_bytes'],next_status=j['result']['status'],next_boundary=j['result']['boundary'],coordinate_roundtrip_cases=j['coordinate_roundtrip_cases'],original_input_calls=j['input_calls'],original_startup_counts=j['startup_counts'],original_startup_events=j['startup_events'],actual_queue_calls=j['pump_calls'],native_geometry=j['native_geometry'],native_diagnostic=j['native_diagnostic'],native_audio=j['native_audio'],window_capture=j['window_capture'],stream_contract=read('stage43-stream-contract.json'),full_checkpoint_restore=rest,protected_hash_checks=audit['hash_checks'],unique_protected_paths=audit['unique_paths'],all_frozen_input_hashes_unchanged=True,bounded_logs=[dict(label=n,**read(n+'.result.json'))for n in labels],failed_background_variant=dict(log='evidence/stage43-background-startup.stderr.log',reason='Accessory variant did not meet explicit non-frontmost assertion; duplicate native class warning also corrected',old_Regular_probe_observation='key=0 and activation_requests=0 did not guarantee process stayed non-frontmost',final_correction='Distinct S43Window and process-local Prohibited policy initialized before inherited native objects; actual probe/run/capture/restore verify non-frontmost'),original_code_patched=False,original_loop_skipped=False,original_input_modified=False,global_input_events_posted=False,nested_movie_substituted=False,original_MSS_Bink_M3D_exports_executed_this_stage=False,prior_DLL_approval_expanded=False,main_menu_passed=False,city_logic_passed=False,city_tick_proven=False,new_original_function_static_conversion_proven=False,Bink_decode_or_sound_factory_equivalence_proven=False,Windows_DPI_virtualization_or_shared_cursor_equivalence_proven=False,worth_continuing=True,remaining=['Nonempty MP3 stream decoder/resampling/loop/volume/EOS and persistence before startup can progress','Full message pump: positive keyboard, unknown events, blocking empty GetMessage, pending paint and timers remain unsupported','Coordinate mapping currently one screen, guest logical pixel per AppKit point; exact Windows DPI and shared cursor stack unmeasured','Missing video and Bink sound-open callback registration remain full recreation gaps','Menu/city and arm64 original function static conversion not established this stage'],next_authorized_work='Own stream implementation with trusted Apple decoder and explicit lifecycle contracts; new original DLL reference export executions still require fresh precise approval')
out['native_window_remained_visible_and_onscreen']=bool(j['native_diagnostic']['visible']and j['window_capture']['native_capture_observation'][5])
out['background_window_remaining_gap']='Prohibited policy keeps process non-frontmost, but full rendering left window visible=0 and CG_onscreen=0; window-ID capture shows only cached own loading surface, not an onscreen menu proof'
out['remaining'].insert(0,out['background_window_remaining_gap'])
(R/'evidence/stage43-result.json').write_text(json.dumps(out,ensure_ascii=False,indent=2));print(json.dumps({k:out[k]for k in ['current_checkpoint','main_PC','next_status','protected_hash_checks','unique_protected_paths']},ensure_ascii=False))
