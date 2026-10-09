import json,hashlib,sys,datetime
from pathlib import Path
from stage12_state import decode
R=Path(__file__).resolve().parents[1];tag=sys.argv[1]
def read(n):return json.loads((R/'evidence'/n).read_text())
def sha(p):
 h=hashlib.sha256()
 with p.open('rb')as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
run=read('stage47-'+tag+'.json');audit=read('stage47-postflight.json');restore=read('stage47-restore.json');assert restore['source']==tag and restore['all_memory_CPU_four_threads_native_blobs_restored']
files=[]
for name in ['context.bin','memory.bin','state.json']:
 p=R/'runs'/('stage12-state-'+tag)/name;files.append(dict(path=str(p.relative_to(R)),bytes=p.stat().st_size,sha256=sha(p)))
m=json.loads((R/'runs'/('stage12-state-'+tag)/'state.json').read_text());host=decode(m['host']);baseline=decode(json.loads((R/'runs/stage12-state-stable46/state.json').read_text())['host']);old_reads=baseline['native_file_read_calls'];assert host['native_file_read_calls'][:len(old_reads)]==old_reads;reads=[q for q in host['native_file_read_calls'][len(old_reads):]if 'campaigns' in q.get('guest_path','').lower() or '.sav' in q.get('guest_path','').lower()]
logs=[dict(path=str(p.relative_to(R)),**json.loads(p.read_text()))for p in sorted((R/'evidence').glob('stage47-*.result.json'))]
failure=read('stage47-lifecycle47-visibility.json');preclose=read('stage47-'+tag+'-before-dispose.json');postclose=read('stage47-'+tag+'-after-dispose.json');probe=json.loads((R/'evidence/stage47-owned-visibility-probe-v2.stdout.log').read_text());assert probe['passed'] and probe['close_notifications_before_cleanup']==0 and probe['close_notifications_after_cleanup']==1
out=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),current_checkpoint=tag,current_checkpoint_files=files,result=run['result'],natural_state=run['natural_state'],original_startup_counts=run['startup_counts'],original_WndProc_entries=run['original_WndProc_entries'],owned_input_route=run['route'],window_failure_reproduced=failure,owned_visibility_recovery_probe=probe,actual_visibility_recoveries=run['visibility_recoveries47'],native_before_dispose=preclose,native_after_dispose=postclose,actual_owned_window_capture=run.get('window_capture'),full_checkpoint_restore=True,save_paths=read('stage47-existing-save-paths.json'),Find_calls=run['find_calls'],file_bindings=run['profile_bindings'],actual_campaign_and_save_ReadFile=reads,native_audio=run['native_audio'],protected_hash_checks=audit['hash_checks'],unique_protected_paths=audit['unique_paths'],all_frozen_inputs_unchanged=audit['all_frozen_inputs_unchanged'],all_absences_and_DOS_symlinks_unchanged=audit['all_observed_missing_inputs_still_missing']and audit['all_namespace_symlinks_unchanged'],bounded_logs=logs,all_commands_under_60_seconds=all(not q['timed_out']and q['duration_seconds']<60 for q in logs),zoom_highlight_and_Proceed_native_window_verified=True,Proceed_original_MOVE_DOWN_UP_dispatched=True,ST9BASE_name_returned_from_actual_directory=True,ST9BASE_loaded_through_menu=False,city_update_proven=False,new_original_static_conversion_proven=False,full_native_WinMain_proven=False,original_DLL_exports_executed=False,source_profile_or_save_modified=False,isolated_profile_modified_this_stage=False,game_fields_injected=False,original_code_patched=False,global_input_events_posted=False,global_cursor_moved=False,foreground_activation_requested=False,source_PCM_sent_to_device=False,files_uploaded=False,approval_blocker=None,worth_continuing=True)
(R/'evidence/stage47-result.json').write_text(json.dumps(out,indent=2))
report=f'''Stage47，2026-10-07。本轮先重现并定位了窗口截图失败，已取得实际非激活任务窗口中的 zoom 黄色高亮及 Proceed 截图，并通过原 WndProc 的 MOVE/DOWN/UP 推进原菜单。ST9BASE 已由真实目录枚举返回；尚未通过原菜单选择并加载 ST9BASE，城市更新尚无证据。当前可续检查点：{tag}。

截图失败的长续跑为44.294秒，未触发60秒硬限。原生记录显示在cleanup关闭之前：NSWindow.visible=0、CGWindowIsOnscreen=0、owner缺失；AppKit已finishedLaunching，策略已为Accessory1，key/active/activation_requests均0，前台仍是另一进程。旧s44_visible只在Prohibited→Accessory的当次分支orderFront，此后已是Accessory而被隐藏的面板不再重排。短续跑单次检查通过；长续跑20次有限重试仍不可见，不能靠主表面像素声称窗口通过。已证明的直接原因是启动完成后面板仍隐藏而旧助手缺少恢复路径；具体系统内部隐藏事件未全部追踪，不能把重现当作Stage46历史断言的完整事件回放。

任务自造arm64 stage47_visibility.m仅接当前真实NSPanel指针；要求主线程、非激活Panel、不能成为key/main、没有miniaturize、guest生命周期仍visible、AppKit实际启动完成且已Accessory，才对本窗口orderFront:nil。没有activate/makeKey/全局输入调用。独立样例首先未处理NSApplication生命周期队列，实际20秒内finishedLaunching=false，返回3，失败日志保留；修正为只处理本进程AppKitDefined事件后2.182秒通过。受控隐藏只作用于自造主机面板：guest_visible=0保持隐藏；=1恢复，WindowServer onscreen和owner一致，key/active仍0且前台PID不变。关闭通知清理前0、清理后1。此受控主机故障并非原游戏状态注入。原继续运行的修复动作观测数为{len(run['visibility_recoveries47'])}，独立样例与实际原窗口截图分开记录。

真实任务窗口显示Choose an Emperor，zoom为黄色选中；Proceed客户端坐标(650,518)，客户端1024×768。点击前在同一进程校验原生窗口可见性/WindowServer归属并仅截该窗口，之后在其私有NSEvent FIFO排MOVE/DOWN/UP；原WndProc接收全部三类事件。原程序自然只读检查档案所记save\\zoom\\ST9AFTR.sav，未重映射成ST9BASE；隔离基线zoom.dat的旧路径本来如此，本轮不改它。安装源zoom.dat与隔离基线不同且分别冻结，安装源没有ST9BASE；任务基线Save/zoom/ST9BASE.sav真实存在，168643字节，SHA fd97f2f426e4b69e311d5a8c8596b170f7246df8ad102c4173fcb476d43689e1。本轮没有改源档案、源存档或隔离档案。

原代码接着FindFirst(save\\zoom\\*.sav) return43bbcd；实际只读目录FD/os.scandir返回全部12个现有名称，含GBK名称与ST9BASE，没有目标筛选、假条目或人为排序。NativeSavedFile17提供实际元数据，320字节FindData中的名字偏移44；搜索索引/记录/guest句柄可恢复，重新打开目录并验证名字/SHA/device/inode，原FindClose实际关闭FD。一般DOS通配符、NTFS短名/ACL和完整Windows元数据等价未证。

原流程继续Campaigns\\*，return43bd5e/43beb3。只读EXE字节43bd20验证名字消费者跳过首字节'.'，调用GetFileAttributesA并只保留属性16的目录；返回全部33个实际非dot目录项，包括Audio与UNTITLED，原代码自行筛选。原拼接可产生Campaigns\\\\name双分隔符，来自实际检查点字符串，有限适配将已冻结同名项映射到同一真实文件/目录，没有开放一般路径归一化。新增目录元数据组件只读open(O_DIRECTORY|O_NOFOLLOW)/fstat，用已审核任务代码换算birth/access/write FILETIME，宿主指针/FD不写guest快照。原代码随后开始读取Campaigns的.pak作者数据。本轮实际原ReadFile相关记录{len(reads)}项，详情及片段SHA见stage47-result.json，不把适配器提前验证的整文件pread误算作原代码读档。

最终原状态：{run['natural_state']}；result={run['result']['status']}，边界{run['result'].get('boundary',{})}。尚无原load534a30、scheduler4ac2b0或cycle-tail4ac650通过证据；没有新原函数AOT转换，仍是获准i386 Unicorn参照与任务自造arm64平台接口。当前检查点全部映射内存/CPU/四线程及语义native blob冷恢复通过，三文件SHA记录于evidence/stage47-result.json。

源保护审计{audit['hash_checks']}项/{audit['unique_paths']}独立路径全不变，缺失输入与DOS符号链接全不变。所有stage47有限命令均有60秒硬限及stdout/stderr/result日志，失败、API拒绝与wall_budget分别保留。游戏PCM只离线消费，设备source_frames={run['native_audio']['source_frames']}；没有上传、提交、发布、运行安装器、第三方新执行、全局输入、抢焦点或系统设置/全局依赖变更。没有当前审批阻塞。

恢复命令：/usr/bin/python3 samples/run_bounded.py stage48-restore . /usr/bin/python3 samples/checkpoint_startup47.py --resume {tag} --restore-only。下一步从该检查点接明确停下的只读API边界，让原菜单完成绘制并取得实际窗口截图，再窗口内选择Load Game/ST9BASE；不得把目录枚举包含ST9BASE或城市字段为0算作读档通过。主要证据为stage47-result.json、stage47-{tag}.json、stage47-restore.json、stage47-lifecycle47-visibility.json、stage47-owned-visibility-probe-v2.stdout.log及stage47-postflight.json。当前本地窗口截图见{run.get('window_capture',{}).get('path')}，本轮未上传。
'''
(R/'reports/stage47-window-lifecycle-and-save-enumeration.md').write_text(report)
print(json.dumps(dict(checkpoint=tag,result=run['result']['status'],boundary=run['result'].get('boundary'),protected_hash_checks=audit['hash_checks'],actual_campaign_and_save_ReadFile=len(reads),full_restore=True,all_commands_under_60_seconds=out['all_commands_under_60_seconds'])))
