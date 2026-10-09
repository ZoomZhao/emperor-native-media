import json,hashlib,datetime
from pathlib import Path
R=Path(__file__).resolve().parents[1]
def load(n):return json.loads((R/'evidence'/n).read_text())
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
tag='returnwatch51';run=load('stage51-'+tag+'.json');ledger=load('stage51-batch-ledger.json');progress=load('stage51-batch-progress.json');audit=load('stage52-postflight.json');resources=load('stage52-'+tag+'-resources.json')
assert len(ledger['executions'])==9 and ledger['remaining_original_executions']==1 and ledger['goal_closed']
assert run['result']['status']=='original_loader_returned'and run['result']['no_caller_instruction_or_city_tick_after_return']
root=run['result']['original_loader_return'];assert root['EAX']==0 and root['original_cdecl_return_stack_verified']
cpu=resources['saved_CPU_registers'];assert cpu['eax']==0 and cpu['eip']==root['return_PC']==0x401aa8 and cpu['esp']==root['owner_SP']+4
assert not run['city_frames49']and progress['unique_read_bytes']==168643 and progress['all_read_slices_equal']
assert any(q['pc']=='0x534b7b'and q['eax']=='0x1'for q in run['path_events_this_run49'])
assert run['native_audio']['source_frames']==0 and audit['all_frozen_inputs_unchanged']
rows=[]
for q in ledger['executions']:
 d=q['verified_outcome'];assert d['returncode']==0 and not d['timed_out']and d['duration_seconds']<60;rows.append(dict(index=q['index'],source=q['resume'],checkpoint=q['tag'],**d))
files=[]
for n in ['context.bin','memory.bin','state.json']:
 p=R/'runs'/('stage12-state-'+tag)/n;files.append(dict(path=str(p.relative_to(R)),bytes=p.stat().st_size,sha256=sha(p)))
pre=load('stage51-'+tag+'-before-dispose.json');post=load('stage51-'+tag+'-after-dispose.json')
assert pre['native_lifecycle']['actual_NSWindowWillClose_count']==0 and post['native_lifecycle']['actual_NSWindowWillClose_count']==1
out=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),current_checkpoint=tag,current_checkpoint_files=files,current_sources=[dict(path=str(p.relative_to(R)),sha256=sha(p))for p in sorted((R/'samples').glob('*5[12]*'))if p.is_file()],maximum_original_execution_budget=10,original_executions_used=9,original_executions_remaining_unused=1,batch_goal_closed=True,same_ledger='evidence/stage51-batch-ledger.json',batch_runs=rows,actual_original_loader_success_return=root,actual_saved_CPU=cpu,original_postload_finalizer_success_return_observed=True,original_SaveFile_deserialized_and_closed=True,original_ST9BASE_read_calls=progress['actual_ST9BASE_ReadFile_calls'],original_ST9BASE_read_bytes=progress['actual_ST9BASE_read_bytes'],unique_ST9BASE_read_bytes=168643,read_intervals=progress['read_intervals'],all_actual_read_slices_equal=True,original_reference_city_loader_proven=True,caller_instruction_after_loader_return_executed=False,extra_city_tick_after_return_executed=False,original_city_scheduler_or_full_cycle_proven=False,city_view_drawn=False,natural_state=run['natural_state'],pending_original_city_frames=[],object_and_resource_metadata=resources,semantic_original_city_object_graph_fully_decoded=False,actual_window_capture=run['window_capture'],native_audio=run['native_audio'],native_before_dispose=pre,native_after_dispose=post,protected_hash_checks=audit['hash_checks'],unique_protected_paths=audit['unique_paths'],all_frozen_inputs_unchanged=True,all_absences_and_DOS_symlinks_unchanged=audit['all_observed_missing_inputs_still_missing']and audit['all_namespace_symlinks_unchanged'],latest_checkpoint_separate_cold_restore_tested=False,prior_checkpoints_restored_before_bounded_runs=True,source_save_or_profile_modified=False,isolated_profile_modified=False,host_environment_changed=False,host_cwd_changed=False,game_fields_injected=False,original_code_patched=False,global_input_events_posted=False,global_cursor_moved=False,foreground_activation_requested=False,source_PCM_sent_to_device=False,original_DLL_exports_executed=False,files_uploaded=False,new_original_AOT_conversion_proven=False,full_native_WinMain_proven=False,approval_blocker=None,current_unimplemented_API_boundary=None,stop_reason='approved root-loader goal reached; paused before caller first instruction; same ledger sealed with one unused execution; no further original instructions run',loader_stop_own_probe=load('stage51-loader-stop-probe.json'),goal_closed_own_probe=load('stage52-budget-goal-closed-probe.json'),metadata_failure_log_retained='evidence/stage52-objects51-readonly-resource-metadata.stderr.log')
(R/'evidence/stage52-result.json').write_text(json.dumps(out,indent=2))
table='\n'.join(f"| {q['index']} | {q['source']} → {q['checkpoint']} | {q['duration_seconds']} | {q['status']} |"for q in rows if q['index']>=6)
report=f'''Stage52，2026-10-07。本批目标已达到：固定EN原EXE的城市加载根函数0x534a30自然成功返回EAX=0，实际调用者PC0x401aa8，ESP={hex(cpu['esp'])}=入口ownerSP+4，原cdecl栈恢复已核对。原postload finalizer在0x534b7b也已自然返回EAX1。不是根据菜单、图片、字段变化或函数进入宣布通过。

原函数仍在获准Unicorn 2.1.4中解释执行，平台接口为本任务自写arm64适配。本轮证明的是原参照loader路径成功，不是root函数AOT转换、全原生WinMain、完整城市模拟或Windows API全集等价。

| 原预算次数 | 来源 → 检查点 | 整进程秒 | 原执行结果 |
|---|---|---:|---|
{table}

沿用原stage51-batch-ledger.json，未新建/重置原预算：最多10次，累计已用9次、剩1次未用。第6至8次分别推进原postload到4b07b3、469498、76aa3a；第9次设脚本已有的18秒内层预算，整个进程仍硬限60秒，实际45.844秒在目标处结束。九次原执行都正常完成且小于60秒，未知API仍拒绝。本轮没有重跑已完成的存档读取。

为遵守“root返回即停止、不得额外city tick”，给已有被动city return observer添加停止边界：只有同线程、真实返回PC且ESP=entrySP+4的原返回才记录EAX并停止；此时CPU还没执行调用者第一条指令。没有改原指令或跳过postload。三项纯自造hook状态测试验证错误栈不误停、成功和失败自然返回都停，无Uc实例/原EXE执行。原运行段返回该已命名的停止边界后，报告明确分类original_loader_returned，区分API未实现拒绝。原postload完整执行后才达到这一停止点。实际saved CPU EAX0/EIP401aa8/ESP{hex(cpu['esp'])}另经检查点只读检视确认。city_frames49已清空。

goal_closed标记已写回同一原账本；剩1次只是未用次数，不用于额外绘图或模拟。claim现在拒绝已关闭目标，在独立自造ledger fixture中已验证拒绝且文件不变。没有开始第10次或其他原指令执行。调用者后续切屏/城市绘制/调度tick全部未执行；screen_mode仍13，因此不宣称城市画面或完整simulation cycle已通过。

此前ST9BASE原ReadFile546次，累计/唯一覆盖168643字节、区间[0,168643)，每段与真实只读文件内容相等；原deserializer返回、存档关闭均已观察，postload现已完成。人口28、金库1633、阶段5均为原自然状态，不注入字段或强制gate。文件SHA fd97f2f426e4b69e311d5a8c8596b170f7246df8ad102c4173fcb476d43689e1；固定EXE SHA 8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753。

对象/资源证据是只读检查点元数据：returnwatch51实际跟踪guest live heap块{resources['guest_tracked_live_heap_blocks']}个、{resources['guest_tracked_live_heap_bytes']}字节，完整映射内存{resources['guest_mapped_memory_bytes']}字节；owned DirectDraw COM对象6个（1 device/4 surface/1 clipper），冻结文件身份绑定517项。此前objects51为18132块/188959736字节/413项绑定，显示原postload资源状态的实际变化。它们不是城市实体数，也不证明已解码整个原对象图。来源、guest对象地址、资源SHA/大小/身份和live文件位置见stage52-returnwatch51-resources.json。第一次摘要因bytes路径直接JSON序列化失败，6.545秒返回1，失败日志保留；转为显示路径后只读提取成功，不是原游戏失败、不计原预算。

当前检查点returnwatch51三文件SHA/大小及源码SHA见结果JSON。它本轮未另做冷恢复；campaign51、objects51、postobjects51、finalizer51在后续有界原执行前已通过完整映射内存/CPU/四线程/native和guest环境restore assertions。停止后仅自写账本保护、只读检查点分析、源保护审计与本地报告，未调用原指令执行或原DLL。

保护审计{audit['hash_checks']}项/{audit['unique_paths']}独立路径全部不变：原EXE/MSS、游戏源/冻结数据/存档/档案、旧代码、旧检查点及campaign51；缺失项和DOS链接全不变。设备source_frames=0，活动回调/queue/pending均在保存后停止合同下归零；window非key/nonactive/activation请求0，关闭通知清理前0、清理后1。无源/隔离档案写入、宿主环境或cwd改变、原代码补丁、全局输入/光标、前台激活、原DLL执行、上传、安装、安全设置变更、提交或发布。

交付：evidence/stage52-result.json、stage52-returnwatch51-resources.json、stage52-postflight.json、stage52-budget-goal-closed-probe.json、stage51-returnwatch51.json、stage51-batch-ledger.json；代码samples/checkpoint_startup51.py（只读返回观察停止）、stage51_batch.py（相同预算及goal seal）、stage52_resource_snapshot.py/.audit.py/.finalize.py。下一阶段若要实际城市绘制/调度或原函数AOT对照，需要另行明确目标和执行范围；本轮到原loader成功返回即交回。
'''
(R/'reports/stage52-original-city-loader-success-and-stop.md').write_text(report)
print(json.dumps(dict(checkpoint=tag,loader_return_EAX=0,caller_PC=hex(cpu['eip']),caller_SP=hex(cpu['esp']),used=9,remaining_unused=1,goal_closed=True,extra_city_tick=False,hash_checks=audit['hash_checks'])))
