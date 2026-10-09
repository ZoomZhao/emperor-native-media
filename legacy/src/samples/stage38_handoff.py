# -*- coding: utf-8 -*-
import datetime,hashlib,json,struct
from pathlib import Path
R=Path(__file__).resolve().parents[1]
approval=json.loads((R/'evidence/stage36-MSS-execution-approval.json').read_text());probe=json.loads((R/'evidence/stage38-MSS-decoded.json').read_text());execution=json.loads((R/'evidence/stage36-MSS-execution.json').read_text());device=json.loads((R/'evidence/stage38-device38.json').read_text());restore=json.loads((R/'evidence/stage38-restore.json').read_text());audit=json.loads((R/'evidence/stage38-postflight.json').read_text());build=json.loads((R/'evidence/stage38-native-build.json').read_text())
assert approval['approved']and approval['scope_id']=='MSS36_STARTUP_DEVICE_V1'
assert probe['probe_completed']and probe['device_open_success']and execution['returncode']==0
assert restore['all_memory_CPU_four_threads_and_native_blobs_restored']and audit['all_frozen_inputs_unchanged']
calls=device['miles38_calls'];assert [r['name']for r in calls]==['_AIL_startup@0','_AIL_set_preference@8','_AIL_set_preference@8','_AIL_waveOutOpen@16']
assert [r['return_value']for r in calls]==[1,0,0,0]
assert calls[0]['preference42_after']==probe['preference42_before_set_after_set_after_cold_after_restart'][2]==8
comparison=[dict(native_call=r,reference_operation=probe['operations'][i],returned_value_equal=r['return_value']==probe['operations'][i]['EAX_u32'])for r,i in zip(calls,[12,15,16,17])]
assert all(r['returned_value_equal']for r in comparison)
checkpoint=[]
for p in sorted((R/'runs/stage12-state-device38').rglob('*')):
 if p.is_file():checkpoint.append(dict(path=str(p.relative_to(R)),bytes=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
state=json.loads((R/'runs/stage12-state-device38/state.json').read_text())
out=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),outcome='一次明确批准的 MSS36 探针成功；原启动参考链以自写 arm64 观测子集和真实 CoreAudio 推进到首个 sample 分配，device38 完整恢复通过；主界面未到',
 approval_scope='MSS36_STARTUP_DEVICE_V1',approval_record='evidence/stage36-MSS-execution-approval.json',approval_user_verbatim=approval['user_verbatim'],approval_user_timestamp_utc=approval['user_message_timestamp_utc'],
 original_DLL_probe_executed_once=True,approved_probe_exact_command='/usr/bin/python3 samples/run_bounded.py stage36-MSS-approved . /usr/bin/python3 samples/run_stage36_mss_pending.py --approval-file evidence/stage36-MSS-execution-approval.json',
 probe_exitcode=execution['returncode'],probe_elapsed_seconds=execution['elapsed_seconds'],probe_output_sha256=execution['output_sha256'],
 cold_repeated_restart_startup=probe['cold_repeated_restart_startup'],preference42_observations=probe['preference42_before_set_after_set_after_cold_after_restart'],waveOutOpen_result=probe['waveOutOpen_result'],original_reference_driver_pointer=hex(probe['driver_handle_u32']),providers=probe['provider_load_unload'],
 subset_native_reference_comparison=comparison,original_probe_does_not_prove_native_provider_registry=True,
 native_Miles_state_library=build,native_audio_library='runs/stage37-native/libstage37_audio.dylib',Stage37_module_not_retested_or_modified_this_stage=True,
 current_checkpoint='runs/stage12-state-device38',current_checkpoint_files=checkpoint,current_main_PC=hex(state['registers']['eip']),next_boundary=device['result']['boundary'],
 original_startup='Unicorn original i386 unpatched instructions',full_native_WinMain_proven=False,new_original_function_static_conversion_proven=False,
 native_audio_stats=device['audio_stats'],complete_native_Miles_startup_provider_mixer_equivalence_proven=False,
 checkpoint_full_memory_CPU_four_threads_and_all_native_blobs_restored=True,native_restore=restore,original_final35_checkpoint_preserved=True,
 background_window=dict(key_window=device['native_diagnostic']['key_window'],active=device['native_diagnostic']['NSApp_isActive'],activation_requests=device['native_diagnostic']['activation_requests'],frontmost_pid=device['native_diagnostic']['frontmost_pid']),
 main_menu_passed=False,city_logic_passed=False,original_IJL_or_SEH_dispatcher_executed_this_stage=False,
 original_MSS_sample_stream_3D_calls_executed=False,original_game_audio_assets_played=False,recording_performed=False,
 probe_private_prefix_cleanup_verified=True,old_baseline_Wine_processes_left_untouched=True,all_frozen_input_hashes_unchanged=True,automatic_approval_rejected=False,
 next_static_contract=dict(original_call='0x761799 -> _AIL_allocate_sample_handle@4',return_PC='0x76179f',requested_handle_count=8,
 core_allocate='0x211138f0: scan driver+52 pool, driver+56 count, 644-byte stride, status1 free, then call AIL_init_sample',
 core_initialize='0x21113980: reset playback buffers/cursors/callback fields, rate11025, preference5 volume, pan conditional on driver+24, install processor2 from driver+500 and processor1 from driver+396, finally status2',
 unresolved='driver mixer/filter interfaces and sample processor side effects are not bound by current CoreAudio FIFO; no fake handle allocation',
 allowed_next_work='read fixed disassembly and author own host sample/mixer contracts; no further original DLL call is authorized by MSS36'),
 worth_continuing=True,remaining_uncertainties=['native provider/RIB registry and builtin mixer equivalence','sample allocation/init processors, sample status/EOS callbacks','stream/MP3/3D and service timing','ImageIO versus original IJL pixel equivalence and Windows font metrics','full static-native original startup, main menu and city logic'])
(R/'evidence/stage38-result.json').write_text(json.dumps(out,indent=2,ensure_ascii=False));print(json.dumps({k:out[k]for k in ['outcome','probe_exitcode','current_checkpoint','next_boundary','all_frozen_input_hashes_unchanged']},ensure_ascii=False))
