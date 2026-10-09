# -*- coding: utf-8 -*-
import datetime,hashlib,json
from pathlib import Path
R=Path(__file__).resolve().parents[1]
def read(name):return json.loads((R/'evidence'/name).read_text())
cache={}
def sha(path):
 path=Path(path)
 if str(path)not in cache:
  h=hashlib.sha256()
  with path.open('rb')as f:
   for b in iter(lambda:f.read(1048576),b''):h.update(b)
  cache[str(path)]=h.hexdigest()
 return cache[str(path)]
rows=[]
def check(path,expected):
 path=Path(path);digest=sha(path);rows.append(dict(path=str(path),expected_sha256=expected,sha256=digest,unchanged=digest==expected))
for q in read('stage40-postflight.json')['checks']:check(q['path'],q['sha256'])
for q in read('stage40-result.json')['current_checkpoint_files']:check(R/q['path'],q['sha256'])
bink=read('stage41-Bink-contract.readonly.json');bm=read('stage41-Bink-input.readonly.json')
for key in ['original_path','copy_path']:check(bm[key],bm['sha256'])
for q in bink['movie_files']:check(q['path'],q['sha256'])
check(R/'runs/stage37-native/libstage37_audio.dylib','41d5bcee16088849a831f5d8510f805cbcd88229af807e29fbcaddd2b341b755')
for name in ['stage39-native-build.json','stage40-build.json']:
 q=read(name);check(q['library'],q['sha256'])
absences=[]
for q in read('stage40-postflight.json')['absences']+bink['requested_path_absences']:
 p=Path(q['path']);absences.append(dict(path=str(p),still_missing=not p.exists()and not any(c.name.casefold()==p.name.casefold()for c in p.parent.iterdir())))
audit=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),all_frozen_inputs_unchanged=all(q['unchanged']for q in rows),hash_checks=len(rows),unique_paths=len(cache),checks=rows,all_observed_missing_inputs_still_missing=all(q['still_missing']for q in absences),absences=absences)
(R/'evidence/stage41-postflight.json').write_text(json.dumps(audit,ensure_ascii=False,indent=2));assert audit['all_frozen_inputs_unchanged']and audit['all_observed_missing_inputs_still_missing']
run=read('stage41-stream41.json');returns=run['scan_returns'];text=read('stage41-text-contract-corrected.json');restore=read('stage41-restore.json')
assert restore['source']=='stream41'and restore['all_memory_CPU_four_threads_and_native_blobs_restored']
assert len(returns)==4916 and all(q['EDX']==0 and q['within_text_allocation']for q in returns)
assert text['shadow_expected_pointer']==returns[0]['EAX']and text['entire_defined_guest_text_payload_equals_original_file']and text['entire_guest_group_table_equals_original_file']
dir=R/'runs/stage12-state-stream41';state=json.loads((dir/'state.json').read_text());files=[dict(path=str(p.relative_to(R)),bytes=p.stat().st_size,sha256=sha(p))for p in sorted(dir.iterdir())if p.is_file()]
logs=[dict(label=n,**read(n+'.result.json'))for n in ['stage41-text-contract','stage41-text-contract-corrected','stage41-progress','stage41-progress-next','stage41-stream-boundary','stage41-final-restore','stage41-bink-static','stage41-bink-static-reviewed','stage41-bink-contract','stage41-bink-contract-observed']]
result=dict(created_utc=audit['created_utc'],outcome='Original text scan terminates and advances through startup; current real boundary is Bink sound-system callback factory',current_checkpoint=str(dir.relative_to(R)),current_checkpoint_files=files,mapped_memory_bytes=state['memory_bytes'],main_PC=hex(state['registers']['eip']),current_guest_registers=state['registers'],next_status=run['result']['status'],next_boundary=run['result']['boundary'],text_scan=dict(entries=len(run['scan_entries']),returns=len(returns),first_return=returns[0],last_return=returns[-1],all_return_EDX_zero=True,all_return_pointers_in_live_text_allocation=True,first_return_matches_shadow_original_predicates=True,initial_stage40_wall_budget_was_finite_work=True,observed_groups=[min(q['group_id']for q in returns),max(q['group_id']for q in returns)],entry_EAX_is_prior_value_not_scan_pointer=True),text_input_contract=text,original_startup_helper_returns=[q for q in run['provider_events']if q['label']=='startup_535bc0_return'],NULL_stream_close_calls=run['stream_calls'],Bink_contract=bink,native_audio=run['native_audio'],full_checkpoint_restore=restore,protected_hash_checks=audit['hash_checks'],unique_protected_paths=audit['unique_paths'],all_frozen_input_hashes_unchanged=True,bounded_logs=logs,initial_transport_disconnection='exec-server disconnected before creating first process; harmless pwd retry recovered, no approval rejection',test_expectation_failures_preserved=['tail asserted as NUL sentinel; corrected to compare defined payload only, observed unused cdcd retained','new Bink readonly input absent from prior protected manifest; compared legal source and local copy then froze new manifest','direct expected movie path is absent; case-insensitive searches found high-quality file only in nested directory; actual ENOENT preserved'],original_code_patched=False,original_loop_skipped=False,original_input_modified=False,original_MSS_Bink_M3D_exports_executed_this_stage=False,MSS36_approval_not_reused_or_expanded=True,Miles3D_equivalence_proven=False,native_3D_unavailable_remains_temporary_gap=True,Bink_sound_factory_or_decode_equivalence_proven=False,nonempty_Miles_mixing_stream_EOS_equivalence_proven=False,main_menu_passed=False,city_logic_passed=False,full_native_WinMain_proven=False,new_original_function_static_conversion_proven=False,original_audio_video_assets_played=False,automatic_approval_rejected=False,worth_continuing=True,remaining_uncertainties=['BinkSetSoundSystem calls BinkOpenMiles immediately; native factory must own callback table and actual driver binding before this API can be claimed','BinkOpenMiles attempts 24 distinct Miles dynamic symbols including nonempty buffers and EOB callback; existing empty sample subset is insufficient','Requested startup high-quality movie path is absent; existing nested movie must not be silently moved or substituted','Temporary native 3D unavailable result remains an unimplemented capability for complete recreation','Menu and original city loading have not been reached in this native platform experiment','Unicorn original i386 reference continuation is separate from arm64 static conversion proof'],next_authorized_work='Read-only Bink/Miles caller and callback table review, own native adapter and synthetic lifecycle tests using installed trusted tools; original DLL dynamic benchmark requires a new exact probe and per-execution approval before running')
(R/'evidence/stage41-result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2));print(json.dumps({k:result[k]for k in ['current_checkpoint','main_PC','next_status','protected_hash_checks','unique_protected_paths','mapped_memory_bytes']},ensure_ascii=False));print(json.dumps(dict(scan_returns=len(returns),Bink_Miles_dynamic_symbols=len(bink['original_Miles_symbols_required']),selected_movie=bink['filename_original_call_argument'])))
