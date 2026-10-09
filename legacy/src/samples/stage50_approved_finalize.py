import json,hashlib,datetime
from pathlib import Path
R=Path(__file__).resolve().parents[1]
def load(n):return json.loads((R/'evidence'/n).read_text())
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
run=load('stage50-read50.json');old=load('stage50-closed50.json');audit=load('stage50-approved-postflight.json');log=load('stage50-original-ST9BASE-read-and-load.result.json');env=load('stage50-read50-environment-boundary.json')
assert log['command']==['/usr/bin/python3','samples/checkpoint_startup50.py','--resume','closed50','--tag','read50','--seconds','14','--capture']
assert log['returncode']==0 and not log['timed_out'] and log['duration_seconds']<60
assert run['result']['status']=='refused' and run['result']['boundary']['name']=='SetEnvironmentVariableA'
assert env['guest_name']=='=C:'and env['guest_value']=='C:\\Emperor'
assert audit['all_frozen_inputs_unchanged']and audit['all_observed_missing_inputs_still_missing']and audit['all_namespace_symlinks_unchanged']
assert run['native_audio']['source_frames']==0 and run['native_file_read_calls_this_run49']==[]
assert run['city_frames49']==old['city_frames49'] and run['city_observations49']==old['city_observations49']
pre=load('stage50-read50-before-dispose.json');post=load('stage50-read50-after-dispose.json')
assert pre['native_lifecycle']['actual_NSWindowWillClose_count']==0 and post['native_lifecycle']['actual_NSWindowWillClose_count']==1
files=[]
for n in ['context.bin','memory.bin','state.json']:
 p=R/'runs/stage12-state-read50'/n;files.append(dict(path=str(p.relative_to(R)),bytes=p.stat().st_size,sha256=sha(p)))
approval=dict(source='explicit user approval transcript supplied by parent thread',source_thread_id='01a10af1-425e-75ae-bded-bf47f843a029',user_approval_utc='2026-10-07 05:14:13 UTC',user_reply='允许',scope='one exact command, existing Unicorn i386 city-load reference, 14-second run and 60-second process; read-only frozen game/ST9BASE; task-only checkpoint/logs/own-window capture; background zero PCM; no original DLL export execution/upload/source changes; user informed elevated Python relies on adapter read-only contract',exact_command='/usr/bin/python3 samples/run_bounded.py stage50-original-ST9BASE-read-and-load . /usr/bin/python3 samples/checkpoint_startup50.py --resume closed50 --tag read50 --seconds 14 --capture',sandbox_permissions='require_escalated',preflight_verified_hashes=17,approved_original_execution_count=1,approval_scope_consumed=True,no_further_original_execution=True,prior_local_elicitation_response={'accepted':True},prior_elicitation_server_request_or_message_ID=None)
out=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),approval=approval,source_EXE='<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe',source_EXE_sha256='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753',execution_method='Unicorn 2.1.4 UC_ARCH_X86/UC_MODE_32 interprets original EXE instructions with own arm64 platform adapters; no full EXE OS launch',current_checkpoint='read50',current_checkpoint_files=files,run_log=log,result=run['result'],exact_environment_boundary=env,natural_state=run['natural_state'],actual_native_ReadFile_this_run=[],actual_ST9BASE_ReadFile_calls=0,actual_ST9BASE_read_bytes=0,original_loader_success_return_observed=False,pending_original_city_frames=run['city_frames49'],original_city_observations=run['city_observations49'],original_city_loaded=False,city_update_proven=False,city_view_drawn=False,actual_window_capture=run['window_capture'],actual_window_visual='Loading Campaign save-list screen with ST9BASE selected; no city view',native_audio=run['native_audio'],native_before_dispose=pre,native_after_dispose=post,protected_hash_checks=audit['hash_checks'],unique_protected_paths=audit['unique_paths'],all_frozen_inputs_unchanged=True,all_absences_and_DOS_symlinks_unchanged=True,original_code_patched=False,game_fields_injected=False,source_save_or_profile_modified=False,isolated_profile_modified=False,mkdir_executed=False,host_environment_changed=False,global_input_events_posted=False,global_cursor_moved=False,foreground_activation_requested=False,source_PCM_sent_to_device=False,original_DLL_exports_executed=False,files_uploaded=False,new_original_AOT_conversion_proven=False,full_native_WinMain_proven=False,approval_blocker_for_this_execution=None,remaining_technical_boundary='unimplemented SetEnvironmentVariableA at original return PC 0x77a02d; guest name =C:, value C:\\Emperor',further_execution_started=False,new_boundary_adapter_implemented=False,new_checkpoint_cold_restore_tested=False)
(R/'evidence/stage50-approved-read-result.json').write_text(json.dumps(out,indent=2));(R/'evidence/stage50-single-execution-approval.json').write_text(json.dumps(approval,indent=2))
report=f'''Stage50 获准单次续跑，2026-10-07。用户于05:14:13 UTC明确回答“允许”，父线程提供了包括提权影响、只读合同非系统沙箱、14秒/60秒限额在内的知情批准转录。只读核对固定EXE、closed50、既有源码及native库共17项后，重试此前同一精确命令一次。本次审批放行，进程正常完成，returncode=0，耗时{log['duration_seconds']}秒，没有超时；原参考运行结果是 refused，因为遇到未实现API，不代表原loader成功。

准确技术边界：KERNEL32.dll!SetEnvironmentVariableA，原返回点0x77a02d，当前API trampoline PC 0x61000290。从read50保存的guest内存只读提取实参：name地址0x700edf20，字符串 =C:；value地址0x700ede10，字符串 C:\\Emperor。该API未执行，宿主环境未改变；没有新增适配、假返回或继续原指令。参数证据 stage50-read50-environment-boundary.json，完整分段日志 stage50-read50.json。

本次真实原ReadFile为0次；ST9BASE原读取0字节。此前49的只读打开和提前SHA校验不算原loader读取。本轮继续的是closed50已挂起的city_load 0x534a30帧，原返回PC/栈帧仍待返回，未观察成功EAX=0返回。自然screen_mode=13，gate/population/treasury/city_phase均0；同一实际后台窗口截图仍为Loading Campaign存档列表，ST9BASE选中，没有城市画面或城市更新证据。截图非黑/不透明检测通过，PNG SHA 0c3a82ab07b567fa7da997b669ebfd21877f8a1fc3d7872dbe9850fe42a7d827；仅本地保存，不上传。

新检查点read50三文件SHA及大小见结果JSON。它被本次save_state实际保存，但本轮未另外执行其冷恢复，不将closed50已有冷恢复验证冒充read50验证。没有扩大为无限续跑；本次单次授权已用完。没有本轮新原函数AOT转换或全原生WinMain证明。

保护检查{audit['hash_checks']}项/{audit['unique_paths']}独立路径全部不变，包含原EXE/原游戏源/冻结数据/存档/档案、既有检查点及本次获准执行代码；缺失项与DOS链接全不变。固定EXE来源 <LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe，重新读取SHA 8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753。实际AudioUnit source_frames=0；窗口非key/nonactive/activation_requests=0，其他进程仍前台；窗口关闭通知清理前0、清理后1。任务内新写入为read50检查点、日志/JSON/自身窗口截图。没有源存档或profile写入、mkdir、全局输入/光标、宿主环境改动、原DLL export执行、上传、下载/安装、安全设置变更、提交或发布。

此前自动审批拒绝文件stage50-approval-blocker.json和历史stage50-result.json保留不改，作为历史记录。本次有效授权在stage50-single-execution-approval.json，本次结果stage50-approved-read-result.json，保护审计stage50-approved-postflight.json。此前“请求已发出”是本地functions.request_user_input_async仅返回accepted:true，无服务器request/message ID；本次推进依据是父线程传来的明确用户批准，而非该本地elicitation或泛化旧授权。

后续工作的准确起点是read50的上述环境变量API边界；本次没有实现它，也没有从read50再次执行原EXE。可据实际参数研究有限guest环境合同，但不能把未知Windows行为或宿主环境写入当作已经验证。
'''
(R/'reports/stage50-approved-single-city-load.md').write_text(report)
print(json.dumps(dict(checkpoint='read50',duration_seconds=log['duration_seconds'],ST9BASE_read_bytes=0,original_loader_returned=False,boundary='SetEnvironmentVariableA =C: -> C:\\Emperor at 0x77a02d',protected_hash_checks=audit['hash_checks'],original_executions=1)))
