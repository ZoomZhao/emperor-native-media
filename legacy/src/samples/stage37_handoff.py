# -*- coding: utf-8 -*-
"""Read-only preservation audit and one consolidated Stage37 handoff."""
import datetime,hashlib,json
from pathlib import Path
R=Path(__file__).resolve().parents[1]
prior=json.loads((R/'evidence/stage36-file-audit.json').read_text())
audit={}
for key in ['checks','additional','providers']:
 audit[key]=[]
 for row in prior[key]:
  p=Path(row['path']);h=hashlib.sha256(p.read_bytes()).hexdigest()
  audit[key].append(dict(path=str(p),sha256=h,unchanged=h==row['sha256']))
flags={'protected147_unchanged':len(audit['checks'])==147 and all(r['unchanged']for r in audit['checks']),
 'previous190_extra_checks_unchanged':len(audit['additional'])==190 and all(r['unchanged']for r in audit['additional']),
 'MSS_provider8_paths_unchanged':len(audit['providers'])==8 and all(r['unchanged']for r in audit['providers'])}
assert all(flags.values());audit.update(flags)
(R/'evidence/stage37-file-audit.json').write_text(json.dumps(audit,indent=2))
proof=json.loads((R/'evidence/stage37-integrated.json').read_text())
build=json.loads((R/'evidence/stage37-build.json').read_text())
probe=json.loads((R/'evidence/stage36-MSS-pending-probe.json').read_text())
assert proof['ok'] and not proof['original_game_advanced'] and not proof['mss36_probe_executed']
assert hashlib.sha256(Path(build['library']).read_bytes()).hexdigest()==build['sha256']
assert not(R/'evidence/stage36-MSS-execution-approval.json').exists()
assert not(R/'evidence/stage36-MSS-execution.json').exists()
checkpoint=[]
for p in sorted((R/'runs/stage12-state-final35').rglob('*')):
 if p.is_file():checkpoint.append(dict(path=str(p.relative_to(R)),bytes=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
result=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),outcome='自写 arm64 PCM FIFO、真实输出回调、32位间接 stdcall、停止竞态与独立进程部分队列恢复通过；保留 final35，等待既定 MSS36 具体批准',
 native_architecture='arm64',library=build,integrated_proof='evidence/stage37-integrated.json',
 producer_accepted_before_stop=proof['producer_accepted_before_stop'],producer_rejected_after_stop=proof['producer_rejected_after_stop'],
 partial_buffer_B_cursor_frames=proof['frozen']['partially_consumed_buffer_B_cursor'],
 frozen_stats=proof['frozen']['frozen_stats'],resumed_stats=proof['independent_restore']['after'],
 source_hash_matches_authored_full_queue=True,completion_events_preserved_exactly_once=True,
 own_i386_indirect_stdcall_count=proof['guest_indirect_stdcall_count']+proof['independent_restore']['guest_indirect_stdcall_count']+13,
 guest_handles='driver 0x5c000101; buffer 0x5dGGGGii; no native pointers serialized',
 original_game_checkpoint='runs/stage12-state-final35',checkpoint_current_hashes=checkpoint,
 main_PC='0x610018e0',pending_original_return_PC='0x761e55',original_game_advanced=False,
 original_MSS_or_provider_executed=False,Miles_EOS_stream_3D_status_contract_verified=False,
 approval_scope='MSS36_STARTUP_DEVICE_V1',approval_request='reports/stage36-minimal-MSS-execution-approval.md',approval_received=False,
 exact_pending_command='/usr/bin/python3 samples/run_bounded.py stage36-MSS-approved . /usr/bin/python3 samples/run_stage36_mss_pending.py --approval-file evidence/stage36-MSS-execution-approval.json',
 probe_sha256=probe['executable_sha256'],automatic_approval_rejected=False,
 failure_log='evidence/stage37-integrated.stderr.log',failure_cause='首次自造 i386 样例覆盖同一 code page 而未失效 Unicorn translation block，错用上一条 open；补 ctl_remove_cache 后通过，未涉及原游戏或 DLL',
 successful_command_result='evidence/stage37-integrated-fixed.result.json',
 no_original_audio_assets_or_original_installers_executed=True,no_system_volume_device_focus_security_settings_changed=True,
 remaining_uncertainties=['原 Miles/provider startup 与旧驱动失败条件未实测','Miles sample/status/EOS/stream/3D 合同未绑定','本次是 own i386 在已批 Unicorn 上跨 ABI 调用 arm64 模块，并非 SR 静态转换或 Emperor 原函数新证据','系统渲染缓冲区得到正确 PCM；未录制物理输出','单次有界停止交错，不是完整线程压力测试；快照只支持此版本/同一当前设备'],worth_continuing=True,**flags)
(R/'evidence/stage37-result.json').write_text(json.dumps(result,indent=2,ensure_ascii=False))
print(json.dumps({k:result[k]for k in ['outcome','producer_accepted_before_stop','producer_rejected_after_stop','original_game_advanced','approval_received','protected147_unchanged']},ensure_ascii=False))
