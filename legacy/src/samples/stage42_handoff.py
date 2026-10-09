# -*- coding: utf-8 -*-
import datetime,hashlib,json
from pathlib import Path
R=Path(__file__).resolve().parents[1]
def read(n):return json.loads((R/'evidence'/n).read_text())
cache={};rows=[]
def sha(p):
 p=Path(p)
 if str(p)not in cache:
  h=hashlib.sha256()
  with p.open('rb')as f:
   for b in iter(lambda:f.read(1048576),b''):h.update(b)
  cache[str(p)]=h.hexdigest()
 return cache[str(p)]
def check(p,wanted):
 got=sha(p);rows.append(dict(path=str(p),expected_sha256=wanted,sha256=got,unchanged=got==wanted))
prior=read('stage41-postflight.json')
for q in prior['checks']:check(q['path'],q['sha256'])
for q in read('stage41-result.json')['current_checkpoint_files']:check(R/q['path'],q['sha256'])
for n in ['stage42-build.json','stage42-pump-build.json']:
 q=read(n);check(q['library'],q['sha256'])
absences=[]
for q in prior['absences']+[dict(path='/Binks/emperor_intro_hiq.bik')]:
 p=Path(q['path']);missing=not p.exists()and(not p.parent.is_dir()or not any(c.name.casefold()==p.name.casefold()for c in p.parent.iterdir()));absences.append(dict(path=str(p),still_missing=missing))
links=[]
for game in [R/'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]',Path('<LOCAL_MACHINE_PATH>]')]:
 p=game.parent/'dosdevices/z:';target=str(p.readlink());links.append(dict(path=str(p),target=target,unchanged=target=='/'));assert target=='/'
audit=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),checks=rows,hash_checks=len(rows),unique_paths=len(cache),all_frozen_inputs_unchanged=all(q['unchanged']for q in rows),absences=absences,all_observed_missing_inputs_still_missing=all(q['still_missing']for q in absences),DOS_namespace_symlinks=links)
(R/'evidence/stage42-postflight.json').write_text(json.dumps(audit,ensure_ascii=False,indent=2));assert audit['all_frozen_inputs_unchanged']and audit['all_observed_missing_inputs_still_missing']
run=read('stage42-loop42.json');restore=read('stage42-restore.json');assert restore['source']=='loop42'and restore['all_memory_CPU_four_threads_and_native_blobs_restored']
paint=read('stage42-paint-state-readonly.json');assert all(q['paint_dirty']==q['paint_active']==q['erase_pending']==0 for q in paint)
assert any(q['label']=='original_movie_open_failed_return'and q['AL']==0 for q in run['bink_events'])
assert any(q['label']=='original_startup_movie_call_return'and q['AL']==0 for q in run['bink_events'])
assert not any(q['label']=='original_movie_open_success_branch'for q in run['bink_events'])
assert run['native_audio']['source_frames']==0 and run['native_diagnostic']['key_window']==0 and run['native_diagnostic']['activation_requests']==0
summary=next(q for q in run['bink_calls']if q['name']=='_BinkGetSummary@8');assert summary['output_sha256_before']==summary['output_sha256_after']
d=R/'runs/stage12-state-loop42';s=json.loads((d/'state.json').read_text());files=[dict(path=str(p.relative_to(R)),bytes=p.stat().st_size,sha256=sha(p))for p in sorted(d.iterdir())if p.is_file()]
logs=[dict(label=n,**read(n+'.result.json'))for n in ['stage42-static','stage42-build','stage42-lifecycle','stage42-startup','stage42-startup-z-root','stage42-post-movie','stage42-wheel-driver','stage42-pump-build','stage42-event-loop','stage42-real-sleep','stage42-final-restore','stage42-paint-state-readonly']]
result=dict(created_utc=audit['created_utc'],outcome='Actual missing movie failure returns through unchanged original caller into startup event loop; complete menu/city recreation remains unproven',current_checkpoint=str(d.relative_to(R)),current_checkpoint_files=files,mapped_memory_bytes=s['memory_bytes'],main_PC=hex(s['registers']['eip']),current_guest_registers=s['registers'],next_status=run['result']['status'],next_boundary=run['result'].get('boundary'),original_movie_failure_and_return_events=run['bink_events'],Bink_native_capabilities=run['bink_native_capabilities'],Bink_calls=run['bink_calls'],message_and_sleep_calls=run['pump_calls'],native_geometry=run['native_geometry'],native_audio=run['native_audio'],native_diagnostic=run['native_diagnostic'],full_checkpoint_restore=restore,protected_hash_checks=audit['hash_checks'],unique_protected_paths=audit['unique_paths'],all_frozen_input_hashes_unchanged=True,bounded_logs=logs,original_code_patched=False,original_loop_or_movie_skipped=False,original_input_modified=False,nested_movie_substituted=False,manufactured_success_or_sound_callback_table=False,original_MSS_Bink_M3D_exports_executed_this_stage=False,MSS36_approval_not_reused_or_expanded=True,main_menu_passed=False,city_logic_passed=False,full_native_WinMain_proven=False,new_original_function_static_conversion_proven=False,Bink_sound_factory_or_decode_equivalence_proven=False,Miles3D_equivalence_proven=False,original_audio_video_assets_played=False,automatic_approval_rejected=False,worth_continuing=True,remaining_uncertainties=['Own native audio factory correctly rejects absent minimum-buffer entry; this does not establish equivalence to original Windows DLL registry success','Missing-video failure path is implemented; video decoding, sound-open callback table and actual movie playback remain recreation gaps','Actual AppKit empty queue was queried; Win32 keyboard, paint, timers, sent-message semantics and positive message callback behavior remain incomplete','Finite actual Sleep(1) host wait is implemented; Windows scheduler/timing equivalence is unproven','Menu and city logic have not been validated','Unicorn i386 reference continuation is separate from arm64 original-function static conversion','Temporary 3D unavailable capability and nonempty Miles mixing/stream EOS remain gaps'],next_authorized_work='Read-only investigation of the next actual startup boundary and own adapter work; every new original Bink/MSS export execution requires fresh exact approval')
result['native_paint_state_readonly_checks']=paint
(R/'evidence/stage42-result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2));print(json.dumps({k:result[k]for k in ['current_checkpoint','main_PC','next_status','next_boundary','protected_hash_checks','unique_protected_paths']},ensure_ascii=False))
