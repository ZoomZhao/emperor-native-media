import json,hashlib,datetime
from pathlib import Path
R=Path(__file__).resolve().parents[1]
def read(n):return json.loads((R/'evidence'/n).read_text())
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
tag='closed50';run=read('stage50-'+tag+'.json');restore=read('stage50-restore.json');audit=read('stage50-postflight.json');probe=read('stage50-close-probe.json');blocker=read('stage50-approval-blocker.json')
assert restore['source']==tag and restore['all_memory_CPU_four_threads_native_blobs_restored']
assert restore['city_frames49']==run['city_frames49'] and restore['city_observations49']==run['city_observations49']
assert probe['passed'] and len(probe['events'])==8 and audit['all_frozen_inputs_unchanged']
files=[]
for n in ['context.bin','memory.bin','state.json']:
 p=R/'runs'/('stage12-state-'+tag)/n;files.append(dict(path=str(p.relative_to(R)),bytes=p.stat().st_size,sha256=sha(p)))
sources=[dict(path=str(p.relative_to(R)),sha256=sha(p))for p in sorted((R/'samples').glob('*50*'))if p.is_file()]
logs=[dict(path=str(p.relative_to(R)),**json.loads(p.read_text()))for p in sorted((R/'evidence').glob('stage50-*.result.json'))]
close=run['stream_close_calls50'];assert len(close)==1
actual=close[0];assert actual['result']==0 and actual['real_AudioUnit_stop_result']==actual['ExtAudioFileDispose_OSStatus']==0 and actual['closed']['generation']==2 and actual['old_generation_handle_rejected']
assert actual['audio_quiescent']['active_callbacks']==actual['audio_quiescent']['queued_buffers']==actual['audio_quiescent']['pending_events']==0
assert actual['closed']['handle']==actual['closed']['decoder_file_open']==actual['closed']['PCM_allocation_live']==actual['closed']['queued_frames']==0
slots=read('stage50-original-stream-slots.json');assert slots['original_music_slots']==[0,0,0,0]
reads=run['native_file_read_calls_this_run49'];target=[q for q in reads if q['guest_path'].lower().endswith('save\\zoom\\st9base.sav')]
events=run['city_observations49'];returns=[q for q in events if q['kind']=='returned'];load_returned=any(q['label']=='city_load'and q['EAX']==0 for q in returns)
pre=read('stage50-'+tag+'-before-dispose.json');post=read('stage50-'+tag+'-after-dispose.json')
assert pre['native_lifecycle']['actual_NSWindowWillClose_count']==0 and post['native_lifecycle']['actual_NSWindowWillClose_count']==1
out=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),current_checkpoint=tag,current_checkpoint_files=files,current_sources=sources,result=run['result'],current_CPU_PC=run['current_CPU_PC'],natural_state=run['natural_state'],original_city_observations=events,pending_original_city_frames=run['city_frames49'],actual_original_stream_close=close,own_stream_close_probe=probe,original_music_slots=slots,actual_native_ReadFile_this_stage=reads,actual_ST9BASE_ReadFile=target,actual_ST9BASE_read_bytes=sum(q['bytes']for q in target),ST9BASE_real_readonly_open_succeeded=True,ST9BASE_loaded_through_menu=load_returned and bool(target),city_load_success_return_observed=load_returned,city_update_proven=False,original_city_frames_restored=True,full_checkpoint_restore=True,native_before_dispose=pre,native_after_dispose=post,native_audio=run['native_audio'],protected_hash_checks=audit['hash_checks'],unique_protected_paths=audit['unique_paths'],all_frozen_inputs_unchanged=audit['all_frozen_inputs_unchanged'],all_absences_and_DOS_symlinks_unchanged=audit['all_observed_missing_inputs_still_missing']and audit['all_namespace_symlinks_unchanged'],bounded_logs=logs,all_completed_commands_under_60_seconds=all(not q['timed_out']and q['duration_seconds']<60 for q in logs),source_profile_or_save_modified=False,isolated_profile_modified_this_stage=False,mkdir_executed_in_stage49_or50=False,readonly_file_contract_implemented_in_stage49=True,game_fields_injected=False,original_code_patched=False,global_input_events_posted=False,global_cursor_moved=False,foreground_activation_requested=False,source_PCM_sent_to_device=False,original_DLL_exports_executed=False,files_uploaded=False,new_original_static_conversion_proven=False,full_native_WinMain_proven=False,original_MSS_callback_equivalence_proven=False,approval_blocker=blocker,worth_continuing_after_explicit_approval=True,changes_after_original_run='Restore replacement now checks old decoder release; verified in eight own tests and closed50 cold restore. Original close path is unchanged.')
(R/'evidence/stage50-result.json').write_text(json.dumps(out,indent=2))
report=f'''Stage50，2026-10-07。AIL_close_stream 的实际停止、资源释放和本任务回调生命周期合同通过；原 EXE 已从 mkdir49 继续进入城市加载根函数 0x534a30，但尚未出现本轮 ST9BASE ReadFile、成功返回或城市更新。当前检查点 closed50，PC {run['current_CPU_PC']}，状态 wall_budget；城市加载的真实返回帧仍挂起。菜单显示或函数入口都不算加载完成。

原 EXE 只读反汇编确认：新关闭调用返回点 0x762e1d，原音乐 owner=0x1156bc8，slot=0；原指令先取出槽中 0x5f000101，再调用 AIL_close_stream，随后自己把槽清零。适配器检查原 IAT 指令和原 guest 槽值，不改游戏字段，保留该 void/undefined EAX，而不伪造有意义的完成值。关闭后的只读检查点检视，owner+476 的四个槽实际都为零。静态证据 stage50-original-stream-close-caller-static.txt、stage50-MSS-close-stream-core.readonly.log；后者只读分析原 MSS，不运行任何 DLL export。

本任务新 arm64 模块 stage50_close_stream.c 在原任务 stream44 基础上增加 checked ExtAudioFileDispose 和原始诊断；不改旧 44/37/49 源码或动态库。活动 stream 的关闭先实际调用已验证的 stage37 AudioOutputUnitStop：本次 rc=0，phase=3，active_callbacks/queued_buffers/pending_events 全为0；停止后两次完整 audio 语义 snapshot 相等，才进入 native 资源释放。ExtAudioFileDispose 实测 OSStatus=0；decoder file、PCM allocation、ring/queued、handle 全清零；generation 1→2，旧句柄 status=-1。原来运行的时钟之后实际重启 rc=0，但设备只有零 PCM，source_frames 始终0。AudioUnit 渲染回调从来不能触及解码器/stream 指针，stream 只由当前 owner 线程服务；因此没有跳过仍活跃的本任务音频回调。原 Miles mixer 内部回调、回调注册和全部生命周期等价仍未证，不能把这一有限合同扩展到未知 MSS 用法。

八项独立自造 WAV 测试通过：空流NULL、真实设备仍运行时拒绝释放、NULL不影响其他活动流、错误代际拒绝、实际停止/静止/检查释放/静音重启、重开新代际且旧token失效、暂停流恢复前检查旧 decoder 释放后再语义恢复并关闭、generation 65536 退休不可wrap/reopen。真实 AudioUnit 回调计数>0，所有测试 source_frames=0。恢复保持保存的 guest generation，因为它重建同一检查点的逻辑句柄，不表示将旧句柄分配给新的 live 对象。恢复时强制 audio stopped/队列回调归零；检查释放失败即拒绝，非成功吞错。own probe stage50-close-probe.json；原实际 close 的全部前后诊断 stage50-closed50.json。

mkdir49 只是检查点标签，上一轮及本轮都没有执行 mkdir，没有写任何隔离目录或源目录。stage49 的 CreateDirectoryA 有限合同只通过真实只读解析确认 Save/Multiplayer 和 Save/zoom 已存在，返回0/error183；真正新建目录或写/create/delete 请求继续拒绝。统一只读合同已经实现：GBK/大小写不敏感安全路径，限制冻结游戏树，逐目录 openat/O_NOFOLLOW，拒绝符号链接/逃逸/歧义，文件 manifest SHA/大小/device/inode 匹配，native 直接dup已验证FD，真实 errno 映射，read sharing 检查；不只是给写操作换名字。21项合同测试在49已通过，本轮未改变该实现。原 ST9BASE 已在49真实只读打开，大小168643，SHA fd97f2f426e4b69e311d5a8c8596b170f7246df8ad102c4173fcb476d43689e1；提前整文件SHA检查不计作原ReadFile。

本轮原城市加载 entry 已观察1次，选择缓冲区是 ST9BASE.sav，索引3，返回PC/guest SP保存于 pending_original_city_frames；加载成功返回={load_returned}，本轮目标 ReadFile={len(target)}次/{out['actual_ST9BASE_read_bytes']}字节，自然人口/金库/phase仍0。没有注入人口、金库、城市阶段、存档数据或补丁原代码。本轮没有新原函数 AOT 转换，仍是获准 Unicorn i386 参照与自造 arm64 平台接口；完整原生 WinMain 未证。

closed50 全映射内存/CPU/四线程/native语义blob冷恢复通过，挂起城市返回观察帧也相等。恢复命令使用 --restore-only，不调用 thread_run/emu_start，不解释原指令、不启动音频时钟、不输送输入。源保护审计 {audit['hash_checks']} 项/{audit['unique_paths']} 独立路径全部不变，包括源 EXE/MSS、基线游戏/存档/档案和既有检查点；缺失输入和DOS链接全不变。实际窗口关闭通知清理前0、清理后1。所有已执行命令硬限60秒，已完成命令均未超时且小于60秒={out['all_completed_commands_under_60_seconds']}；wall_budget 和测试日志保留。没有上传、全局输入/光标/前台激活、源写入、原DLL export执行、全局依赖、安全设置变更、安装器、提交或发布。

剩余阻塞是审批范围冲突，不是新技术异常。继续从 closed50 原城市加载的提权命令在创建进程前被自动审批拒绝，理由原文：{blocker['reason_verbatim']}。初始指令只允许只读原 EXE 分析；后续任务要求继续实际加载，但自动审查认为授权不足。已提出明确批准请求，未重试或绕过；独立 native 测试、无原指令执行的冷恢复和审计继续完成。需要父线程/用户明确批准用已批准的 Unicorn 2.1.4 解释原 EXE i386 指令、只读冻结源和 ST9BASE、自造 arm64 适配、后台窗口、设备零PCM、无全局输入/上传/原DLL执行、每条60秒硬限，然后才能推进。风险是实际执行原游戏逻辑会触及尚未适配的API；未知写请求仍须在边界拒绝。

待批准的精确命令：{blocker['exact_command']}。来源为用户合法原 EXE 及已批准的 Unicorn 2.1.4，执行脚本和 native 模块为本任务自写；影响仅上述有限进程/后台窗口/静音设备。值得获准后继续：当前已越过真实音乐释放，并进入原加载根函数，下一目标是原 ST9BASE 实际 ReadFile 与 city_load 成功返回；未批准则停留 closed50。

主要交付：evidence/stage50-result.json、stage50-close-probe.json、stage50-closed50.json、stage50-restore.json、stage50-postflight.json、stage50-original-stream-slots.json、stage50-approval-blocker.json；源码 samples/stage50_close_stream.c/.py、stage50_close_probe.py、checkpoint_startup50.py。本地截图仅作窗口证据，不作逻辑通过证据；三文件SHA和源码SHA见结果JSON。
'''
(R/'reports/stage50-checked-stream-close-and-city-load-boundary.md').write_text(report)
print(json.dumps(dict(checkpoint=tag,actual_stream_closed=True,own_probe_cases=8,city_load_entered=True,ST9BASE_loaded=out['ST9BASE_loaded_through_menu'],full_restore=True,hash_checks=audit['hash_checks'],approval_blocked=True)))
