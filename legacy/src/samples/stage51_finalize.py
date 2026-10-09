import json,hashlib,datetime
from pathlib import Path
R=Path(__file__).resolve().parents[1]
def load(n):return json.loads((R/'evidence'/n).read_text())
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
tag='campaign51';run=load('stage51-'+tag+'.json');ledger=load('stage51-batch-ledger.json');progress=load('stage51-batch-progress.json');audit=load('stage51-postflight.json');probe=load('stage51-environment-probe.json')
assert len(ledger['executions'])==5 and ledger['remaining_original_executions']==5 and all(q['status']=='finished'for q in ledger['executions'])
assert progress['unique_read_bytes']==168643 and progress['read_intervals']==[[0,168643]]and progress['all_read_slices_equal']
assert all(q['verified_outcome']['returncode']==0 and not q['verified_outcome']['timed_out']and q['verified_outcome']['duration_seconds']<60 for q in ledger['executions'])
assert run['result']['status']=='wall_budget'and not progress['city_loader_success_returned']
assert run['natural_state']['population']==28 and run['natural_state']['treasury']==1633 and run['native_audio']['source_frames']==0
assert probe['passed']and probe['cases']==19 and audit['all_frozen_inputs_unchanged']
files=[]
for n in ['context.bin','memory.bin','state.json']:
 p=R/'runs'/('stage12-state-'+tag)/n;files.append(dict(path=str(p.relative_to(R)),bytes=p.stat().st_size,sha256=sha(p)))
sources=[dict(path=str(p.relative_to(R)),sha256=sha(p))for p in sorted((R/'samples').glob('*51*'))if p.is_file()]
postload=load('stage51-postload51.json')
events=postload['path_events_this_run49'];assert any(q['pc']=='0x534b1b'for q in events)and any(q['pc']=='0x534b30'for q in events)and any(q['pc']=='0x534b76'for q in events)
rows=[]
for q in ledger['executions']:
 d=q['verified_outcome'];rows.append(dict(index=q['index'],source=q['resume'],checkpoint=q['tag'],purpose=q['purpose'],**d))
out=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),current_checkpoint=tag,current_checkpoint_files=files,current_sources=sources,original_execution_budget_maximum=10,original_executions_used=5,original_executions_remaining=5,same_ledger_must_be_preserved=True,batch_ledger='evidence/stage51-batch-ledger.json',batch_runs=rows,original_execution_method='fixed EN i386 EXE under existing Unicorn 2.1.4, own native platform adapters; no new original AOT conversion',environment_contract=load('stage51-environment-contract.json'),own_environment_probe=probe,actual_original_environment_calls=run['environment_calls51'],environment_state=run['environment_state51'],all_ST9BASE_bytes_original_ReadFile=True,actual_ST9BASE_ReadFile_calls=progress['actual_ST9BASE_ReadFile_calls'],actual_ST9BASE_read_bytes=progress['actual_ST9BASE_read_bytes'],unique_ST9BASE_read_bytes=168643,ST9BASE_unique_read_intervals=progress['read_intervals'],all_actual_read_slices_equal=True,original_deserializer_return_observed=True,original_deserializer_EAX=next(q['eax']for q in events if q['pc']=='0x534b1b'),original_save_close_observed=True,original_postload_finalizer_entered=True,original_loader_success_return_observed=False,original_city_loaded_through_loader_return=False,city_update_proven=False,natural_state=run['natural_state'],current_CPU_PC=run['current_CPU_PC'],pending_original_city_frames=run['city_frames49'],actual_window_capture=run['window_capture'],native_audio=run['native_audio'],protected_hash_checks=audit['hash_checks'],unique_protected_paths=audit['unique_paths'],all_frozen_inputs_unchanged=True,all_absences_and_DOS_symlinks_unchanged=audit['all_observed_missing_inputs_still_missing']and audit['all_namespace_symlinks_unchanged'],latest_checkpoint_separate_cold_restore_tested=False,previous_checkpoints_restored_during_bounded_runs=True,stop_reason='parent requested handoff after the current bounded command; do not reset the existing ten-execution ledger',current_technical_API_boundary=None,source_save_or_profile_modified=False,isolated_profile_modified=False,host_environment_changed=False,host_cwd_changed=False,game_fields_injected=False,original_code_patched=False,mkdir_executed=False,global_input_events_posted=False,global_cursor_moved=False,foreground_activation_requested=False,source_PCM_sent_to_device=False,original_DLL_exports_executed=False,files_uploaded=False,full_native_WinMain_proven=False,further_original_execution_started=False)
(R/'evidence/stage51-result.json').write_text(json.dumps(out,indent=2))
table='\n'.join(f"| {q['index']} | {q['source']} → {q['checkpoint']} | {q['duration_seconds']} | {q['actual_ST9BASE_read_bytes']} | {q['status']} |"for q in rows)
report=f'''Stage51，2026-10-07。已越过 SetEnvironmentVariableA 的真实 guest 状态边界，原 loader 已完整读取并反序列化 ST9BASE、关闭原存档，进入原加载后初始化。但加载根函数0x534a30尚未返回，城市更新及完整原生游戏未证。按父线程要求，在第5次有界命令完成后停止交接；本批上限10次，已用5次、剩5次，必须沿用 evidence/stage51-batch-ledger.json，不能重置预算。

| 次数 | 来源 → 检查点 | 整进程秒 | 本次ST9BASE原读字节 | 原执行结果 |
|---|---|---:|---:|---|
{table}

第1次实际调用SetEnvironmentVariableA '=C:'→'C:\\Emperor'，在兼容层更新guest变量和C驱动目录缓存，返回BOOL1；原CRT自身检查并自然返回EAX0，随后原city_root进入反序列化。五次全部正常完成、未超时且小于60秒；未知API仍拒绝。账本在原指令执行前扣计，保存逐次argv、实现源码SHA、结果及读取量。纯自写预算测试的十次claim保存在另一个fixture，不算原EXE执行。

CRT静态证据：固定EN EXE SHA 8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753，入口0x779fc1，先SetCurrentDirectoryA(IAT0x7a90f8)、再GetCurrentDirectoryA(IAT0x7a9254)，在原栈构造等号/大写驱动字母/冒号，最后SetEnvironmentVariableA(IAT0x7a919c，调用字节ff159c917a00，返回0x77a02d)。新适配对hidden setter检查原caller、原指令字节、EBP+8/EBP-264参数位置及实际native guest cwd；没有直接跳过原检查或写原游戏字段。微软将普通环境变量限定为进程状态，[SetEnvironmentVariableA](https://learn.microsoft.com/en-us/windows/win32/api/processenv/nf-processenv-setenvironmentvariablea)；特殊驱动变量的背景见微软 [=C: bookkeeping 说明](https://devblogs.microsoft.com/oldnewthing/20100506-00/?p=14133)。

实现保存于stage51_environment.py、checkpoint_startup51.py。环境只在guest对象内维护，不读取/继承宿主环境，不调用宿主setenv/chdir，不改变系统或其他进程环境。普通名称仅有限ASCII大小写不敏感，值保持ACP936字节；支持更新、已有项NULL删除、查询/小缓冲区长度、missing203；空非NULL值、删除不存在项、未知hidden驱动等未测行为明确拒绝，XP环境块最多32767字节。成功LastError保留是本有限合同，不宣称全Windows细节实测等价。[GetEnvironmentVariableA](https://learn.microsoft.com/en-us/windows/win32/api/processenv/nf-processenv-getenvironmentvariablea) 支持所用返回/长度规则。

现有NativePaths有真实只读目录FD，只支持 C:\\Emperor 和 dot，本轮不扩展任意目录或其他驱动。hidden =C:值必须与该目录一致；C:relative据C驱动缓存解析，C:\\absolute、rooted、plain relative仍共用49只读安全规范化与manifest/FD校验；驱动根与驱动相对形式保持区别。普通全路径计算不要求目标存在，依据 [GetFullPathNameA](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfullpathnamea)。每进程当前目录及驱动目录背景见 [Changing the Current Directory](https://learn.microsoft.com/en-us/windows/win32/fileio/changing-the-current-directory)。序列化是typed guest环境/驱动状态，恢复先核对native目录；旧检查点普通环境为空，符合原stub一直返回missing，驱动cache从已有native目录派生，hidden变量在原setter之前仍不存在。19项自造测试全部通过：纯bytearray guest内存、无Uc实例或原指令；覆盖stdcall32、参数/调用者保护、缓冲区、大小写/ACP、状态恢复/畸形快照拒绝、驱动相对路径/逃逸拒绝、真实native目录一致、资源及十次预算上限、宿主env/cwd不变。

本批ST9BASE原ReadFile {progress['actual_ST9BASE_ReadFile_calls']}次，累计{progress['actual_ST9BASE_read_bytes']}字节，唯一连续区间[0,168643)，全部片段与只读实际文件相等。第4次看到原反序列化返回0x534b1b(EAX={out['original_deserializer_EAX']})，原文件关闭后到0x534b30，并调用postload finalizer0x534b76。不是将提前SHA校验算作原读取。人口28、金库1633已由原反序列化写入，与既有stage9存档UI参照及stage18原writer观察一致；阶段5来自自然读取/执行，不视为完整scheduler周期。根loader成功返回仍false，挂起返回帧保存。当前PC {run['current_CPU_PC']}，状态wall_budget，没有当前未知API阻塞；仍需完成原postload。并未新转换原函数AOT，依然是获准i386参照执行加自造arm64平台适配。

当前检查点campaign51，三文件SHA/大小及源码SHA见stage51-result.json。此前env51、decoded51、citydata51、postload51均在后续有界执行前通过完整映射内存/CPU/四线程/native及新guest环境的restore assertions；campaign51本轮未单独冷恢复，不将此前恢复冒充它的验证。窗口截图仅本地，不能作为逻辑加载通过。设备source_frames=0，窗口非key/nonactive且无activation请求；原存档/源游戏/档案、旧源码和旧检查点保护审计{audit['hash_checks']}项/{audit['unique_paths']}独立路径全不变，缺失项和DOS链接全不变。无宿主环境修改、源/隔离档案写入、mkdir、全局输入/光标、原DLL执行、上传、安装、安全设置变更、提交或发布。

父任务接续建议：从campaign51恢复，先查看并保持原账本剩5次；使用同一checkpoint_startup51.py与run_bounded.py，每次原指令执行自动claim且整进程60秒硬限。目标仍是原loader自然成功返回或准确真实阻塞。本轮不会再开始第6次。
'''
(R/'reports/stage51-guest-environment-and-original-save-deserialization.md').write_text(report)
print(json.dumps(dict(checkpoint=tag,used=5,remaining=5,ST9BASE_unique_read_bytes=168643,loader_returned=False,hash_checks=audit['hash_checks'])))
