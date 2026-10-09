# -*- coding: utf-8 -*-
import hashlib,json,datetime,sys
from pathlib import Path
from stage12_state import decode
R=Path(__file__).resolve().parents[1]
def read(n):return json.loads((R/'evidence'/n).read_text())
def sha(p):
 h=hashlib.sha256()
 with p.open('rb')as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
tag=sys.argv[1];run=read('stage46-'+tag+'.json');restore=read('stage46-restore.json');audit=read('stage46-postflight.json');probe=read('stage46-PCM-probe.json');profile=read('stage46-profile-files.json');assert restore['source']==tag and restore['all_memory_CPU_four_threads_native_blobs_restored']
checkpoint=[]
for name in ['context.bin','memory.bin','state.json']:
 p=R/'runs'/('stage12-state-'+tag)/name;checkpoint.append(dict(path=str(p.relative_to(R)),bytes=p.stat().st_size,sha256=sha(p)))
m=json.loads((R/'runs'/('stage12-state-'+tag)/'state.json').read_text());host=decode(m['host']);reads=[v for v in host['native_file_read_calls']if v.get('guest_path','').lower()=='c:\\emperor\\save\\zoom.dat'];assert sum(v['bytes']for v in reads)==48470 and all(v['source_slice_equal']for v in reads)
logs=[dict(path=str(p.relative_to(R)),**json.loads(p.read_text()))for p in sorted((R/'evidence').glob('stage46-*.result.json'))]
out=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),outcome='真实按钮PCM播放/结束、鼠标松开、档案枚举与完整只读zoom.dat读取通过；城市读档仍未完成',current_checkpoint=tag,current_checkpoint_files=checkpoint,result=run['result'],natural_state=run['natural_state'],original_startup_counts=run['startup_counts'],original_WndProc_entries=run['original_WndProc_entries'],owned_input_route=run['route'],PCM_actual_calls=run['PCM_calls'],PCM_actual_stats=run['PCM_stats'],PCM_probe=probe,full_checkpoint_restore=restore['all_memory_CPU_four_threads_native_blobs_restored'],Find_actual_calls=run['find_calls'],profile_bindings=run['profile_bindings'],profile_inputs=profile,profile_real_reads=reads,profile_total_read_bytes=sum(v['bytes']for v in reads),native_audio=run['native_audio'],local_window_capture=run.get('window_capture'),protected_hash_checks=audit['hash_checks'],unique_protected_paths=audit['unique_paths'],all_frozen_inputs_unchanged=audit['all_frozen_inputs_unchanged'],all_absences_and_DOS_symlinks_unchanged=audit['all_observed_missing_inputs_still_missing']and audit['all_namespace_symlinks_unchanged'],bounded_logs=logs,all_bounded_commands_under_60_seconds=all(not q['timed_out']and q['duration_seconds']<60 for q in logs),button_PCM_contract_passed=True,original_mouse_up_dispatched=True,original_menu_selected_internal_screen47=run['natural_state']['screen_mode']==47,ST9BASE_menu_load_completed=False,city_update_proven=False,new_original_function_static_conversion_proven=False,full_native_WinMain_proven=False,original_DLL_exports_executed_this_stage=False,original_code_patched=False,gameplay_fields_injected=False,global_input_events_posted=False,foreground_activation_requested=False,source_PCM_sent_to_device=False,files_uploaded_this_stage=False,approval_blocker=None,remaining=['让原RGB555刷新循环完成并检查真实档案画面，再通过窗口内输入继续','通过真实菜单选择ST9BASE，需新增限定目录枚举/读档路径绑定','原城市load534a30和scheduler4ac2b0/cycle4ac650尚无观测','精确Miles混音增益、GlobalAlloc锁/错误状态、非空回调、ASI/DSP、NTFS短名/ACL及一般DOS通配符未证'],worth_continuing=True)
(R/'evidence/stage46-result.json').write_text(json.dumps(out,indent=2))
report=f'''Stage46，2026-10-07，约20分钟。真实按钮音效合同、窗口内鼠标松开、原档案枚举和完整 zoom.dat 只读读取已推进。当前检查点 {tag} 已冷恢复：全部映射内存、CPU、四线程、样本本机快照和文件身份相等。ST9BASE 城市读档与原城市更新仍未完成；没有新增原函数静态转换，执行仍为原 i386 Unicorn 参照加任务自造 arm64 平台接口。

本轮仅新写任务自身实现 samples/stage46_samples.c/.py、checkpoint_startup46.py、stage46_find.py、checkpoint_profiles46.py 与审计/报告脚本。未修改或使用 lzjq 原实现，未执行原 DLL 新导出，未安装第三方工具或全局依赖，无提交、发布、上传、全局输入或抢焦点。

固定原 DLL 只读代码21114890解析WAV，21114ca0→21114c30设置实际类型、播放率、数据地址与长度并返回1。button.wav是单声道22050Hz、16位、数据偏移44、6002字节/3001帧；完整文件SHA e628e609d51b9d35a12193cf6ccd973bf02c17332b94c8638df6cb0dfbbf1e3c。初始化默认11025、volume127、pan64、loop1，附件改为22050。原实际调用随后设loop1、pan64、volume20、reverb三个0，再start。静态21114450证明零reverb写628/632/636。start真实进入PLAYING4，实际离线队列消费3001帧后DONE2；{run['PCM_stats'][str(1577058561)]['nonzero']}个非零混合样本，EOS1。当前空回调来自默认初始化与静态set_file路径，不伪造非空回调成功。

任务自造arm64组件保留真实mono PCM与1024帧环形队列，使用已打开的静音AudioUnit真实帧钟消费；样本默认值仍使用前序固定合法DLL静态证据和任务自造registry。没有游戏PCM送入设备。独立样例验证3000帧仍PLAYING且剩1帧，最后1帧消费后DONE；两遍共6002帧，loop0持续；暂停不消费，恢复/结束、init清附件并还原默认、独立native对象恢复、实际文件释放与generation过期句柄拒绝均通过。错误格式和截断输入无状态改动。首个样例未启动实际driver便尝试分配，真实后端拒绝，日志保留；启动静音driver后同场景通过。

原WndProc实际接收MOVE/DOWN/UP，待派发UP从1变0。原代码自然进入save\\*.dat枚举，FindFirst return43bbcd，FindNext43bc80，FindClose43bc93。任务实际readonly目录FD加os.scandir及native17元数据返回项羽.dat、zoom.dat，GBK往返契合既有ACP936；未隐去任何匹配名称或人为排序。原消费者43bb90只消费WIN32_FIND_DATAA的name偏移44。本次限定普通.dat名称，NTFS短名、DOS通配符、Windows ACL/archive属性完整等价未证。[Microsoft FindFirstFileA](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findfirstfilea)与[WIN32_FIND_DATAA](https://learn.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-win32_find_dataa)仅支持平台合同，实际行为证据来自本地原调用和本机I/O。

原代码自然将内部screen_mode从0改为47，随后检查现有Save/项羽与Save/zoom目录。实际open(O_DIRECTORY|O_NOFOLLOW)/fstat证明目录存在，并按有限POSIX投影返回目录属性。打开zoom.dat实际参数是GENERIC_READ、share3、OPEN_EXISTING3、flags128、SECURITY_ATTRIBUTES32=[12,0,1]；没有写权限或创建请求。同一原CRT路径函数77c4d0返回77c53c仅查root C:\\、最大组件及flags mask6；对唯一新的ASCII档案路径复用前序实际APFS能力查询，没有伪造完整Windows文件系统。

原程序对冻结隔离zoom.dat进行了6次native pread/ReadFile，4096+28672+4096+4096+4096+3414=48470字节，各guest片段SHA与实际源片段相等。该隔离基线zoom.dat SHA95a81d5d6c4ed74299324a4755ddc0592d7bf06a95ebdb89ed42a053a9152c6a，当前安装源zoom.dat SHA4860d96ac229c3e99ed87744293bee2882d94abfaf4b45966154a5e1190dbc64，本来不同，分别冻结和审计，未声称一致；项羽.dat两者一致。没有写入任何源档案或存档。本轮未读ST9BASE作为菜单文件，既有冻结输入仍不变。

最终状态{run['natural_state']}；原load534a30、scheduler4ac2b0、cycle-tail4ac650无入口观测。最后原UI刷新是否完成以本轮运行日志、实际原Blt与本地截图共同判断，内部屏幕47不等于已加载城市。截图仅本地存储，host恢复呈现记录明确与原Blt区分；不能用旧菜单像素冒充新档案画面。最终result={run['result']['status']}，从检查点可续跑，无新许可阻塞；若要新原DLL导出或新第三方软件仍需单独审批。

设备最后render_frames={run['native_audio']['render_frames']}、source_frames=0、zero_frames={run['native_audio']['zero_frames']}、bad_buffers=0。冻结审计{audit['hash_checks']}项哈希、{audit['unique_paths']}独立路径均不变，缺失输入和DOS符号链接不变。所有有限运行均有60秒硬限与stage46-*.result.json；命令失败/拒绝与wall_budget各自保留，不把限时退出当作逻辑通过。前序0x4190xx是RGB555像素转换循环；本轮仍可能在该循环中有限续跑，没有修改、跳过原循环或直接写游戏状态。

主要证据：evidence/stage46-result.json、stage46-{tag}.json、stage46-restore.json、stage46-PCM-probe.json、stage46-static-contract.json、stage46-MSS-PCM-contract-readonly.log、stage46-profile-files.json、stage46-postflight.json。恢复命令：/usr/bin/python3 samples/run_bounded.py stage47-restore . /usr/bin/python3 samples/checkpoint_profiles46.py --resume {tag} --restore-only。建议先继续有限刷新并检查真实档案画面，再通过相应本窗口点击选择/继续zoom档案，继而接真实ST9BASE读档合同。精确Miles增益、非空回调、完整GlobalAlloc锁/错误状态及一般Windows文件系统仍是独立未证项。
'''
(R/'reports/stage46-real-button-PCM-and-profile-menu.md').write_text(report);print(json.dumps(dict(checkpoint=tag,checkpoint_files=checkpoint,native_profile_read_bytes=48470,hash_checks=audit['hash_checks'],all_bounded_commands_under_60_seconds=out['all_bounded_commands_under_60_seconds'])))
