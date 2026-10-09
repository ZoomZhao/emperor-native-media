import json,hashlib,sys,datetime
from pathlib import Path
R=Path(__file__).resolve().parents[1];tag=sys.argv[1]
def read(n):return json.loads((R/'evidence'/n).read_text())
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
run=read('stage48-'+tag+'.json');audit=read('stage48-postflight.json');restore=read('stage48-restore.json');probe=read('stage48-list48-actual-game-window-recovery.json')
assert restore['source']==tag and restore['all_memory_CPU_four_threads_native_blobs_restored'] and probe['passed']
files=[]
for name in ['context.bin','memory.bin','state.json']:
 p=R/'runs'/('stage12-state-'+tag)/name;files.append(dict(path=str(p.relative_to(R)),bytes=p.stat().st_size,sha256=sha(p)))
logs=[dict(path=str(p.relative_to(R)),**json.loads(p.read_text()))for p in sorted((R/'evidence').glob('stage48-*.result.json'))]
metadata=[q for q in run['profile_bindings']if q.get('name')=='GetFileInformationByHandle'];compares=[q for q in run['profile_bindings']if q.get('name')=='CompareFileTime']
pre=read('stage48-'+tag+'-before-dispose.json');post=read('stage48-'+tag+'-after-dispose.json');assert pre['native_lifecycle']['actual_NSWindowWillClose_count']==0 and post['native_lifecycle']['actual_NSWindowWillClose_count']==1
sources=[dict(path=str(p.relative_to(R)),bytes=p.stat().st_size,sha256=sha(p))for p in sorted((R/'samples').glob('*48*'))if p.is_file()]
out=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),current_checkpoint=tag,current_checkpoint_files=files,current_sources=sources,result=run['result'],natural_state=run['natural_state'],original_startup_counts=run['startup_counts'],original_WndProc_entries=run['original_WndProc_entries'],owned_input_route=run['route'],missing_package_calls=run['missing_calls48'],actual_save_file_information=metadata,actual_CompareFileTime_calls=compares,actual_game_window_recovery_probe=probe,first_structural_recovery_capture_failed_visual_validation=read('stage48-first-game-recovery-image-check.json'),actual_owned_window_capture=run.get('window_capture'),native_before_dispose=pre,native_after_dispose=post,full_checkpoint_restore=True,native_audio=run['native_audio'],protected_hash_checks=audit['hash_checks'],unique_protected_paths=audit['unique_paths'],all_frozen_inputs_unchanged=audit['all_frozen_inputs_unchanged'],all_absences_and_DOS_symlinks_unchanged=audit['all_observed_missing_inputs_still_missing']and audit['all_namespace_symlinks_unchanged'],bounded_logs=logs,all_commands_under_60_seconds=all(not q['timed_out']and q['duration_seconds']<60 for q in logs),Load_Saved_Mission_original_MOVE_DOWN_UP_dispatched=True,ST9BASE_loaded_through_menu=False,city_update_proven=False,new_original_static_conversion_proven=False,full_native_WinMain_proven=False,original_DLL_exports_executed=False,source_profile_or_save_modified=False,isolated_profile_modified_this_stage=False,game_fields_injected=False,original_code_patched=False,global_input_events_posted=False,global_cursor_moved=False,foreground_activation_requested=False,source_PCM_sent_to_device=False,files_uploaded=False,approval_blocker=None,worth_continuing=True)
(R/'evidence/stage48-result.json').write_text(json.dumps(out,indent=2))
out['next_dialog_paths']=read('stage48-next-dialog-paths.json');assert all(not q['exists']for q in out['next_dialog_paths']);(R/'evidence/stage48-result.json').write_text(json.dumps(out,indent=2))
report=f'''Stage48，2026-10-07。从 ready47 有限续跑，原程序自然进入战役菜单，再经实际后台游戏窗口的 Load Saved Mission 按钮进入存档列表页面13。ST9BASE 尚未通过菜单选择和加载；没有城市更新或新的原函数 AOT 转换证据。当前检查点：{tag}，下一边界：{json.dumps(run['result'].get('boundary'),ensure_ascii=False)}。

缺失 Campaigns\\Audio.pak、Campaigns\\UNTITLED.pak 的实际分支已通过。GetFullPathNameA 在原调用点77c4f9按实际FD支持的虚拟当前目录 C:\\Emperor 构造路径、返回不含NUL的长度并写32位file-part指针；路径求全不要求文件存在，成功不人为清除LastError。CreateFileA 接受原只读 OPEN_EXISTING 参数和实际12字节 SECURITY_ATTRIBUTES，分别在任务基线与原安装目录用真实只读目录FD/openat得到ENOENT，映射 INVALID_HANDLE_VALUE/ERROR_FILE_NOT_FOUND=2。没有补包、空文件、素材替换或跳过原错误分支。仅两个已观察路径及原调用点被支持，一般Windows路径/权限语义未证。路径语义依据见 [Microsoft GetFullPathNameA](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfullpathnamea)，四次实际API记录在结果JSON。

实际游戏窗口恢复本轮得到验证：它是原继续运行使用的 NSPanel，不是自造独立样例。主机受控隐藏仅改变本窗口可见性，guest CPU、原主表面像素、私有鼠标FIFO和窗口几何全部保持相同；生命周期关闭通知在清理前0、清理后1。恢复后 WindowServer owner/onscreen 与真实进程一致，key/active/activation_requests=0，前台仍是另一进程。原先一张结构检查通过的恢复截图实际全透明，本轮保留文件和失败内容测量，不将它算作视觉通过；新增有限截图内容门槛后，第一次透明帧被拒绝，0.1秒等待及原主表面不变的重新呈现后第二次取得2184×1728、256/256采样点非黑且不透明的实际窗口截图。没有全局事件、鼠标移动或抢焦点。证据：stage48-list48-actual-game-window-recovery.json、stage48-list48-recovered-capture-content.json、stage48-first-game-recovery-image-check.json；通过截图SHA de8285a6b4104db20f63e0ca27341e19f1c1966ea919c104620ff00e38b6b4b8。采样门槛用于拒绝全透明/全黑，不证明每个像素等价；原主表面不变另有直接字节检查。

点击前同一窗口的有内容截图显示 Campaigns of zoom，Load Saved Mission 中心为客户端(512,440)；真实私有NSEvent队列 MOVE/DOWN/UP 均由原WndProc消费，原页面自然变为13。原12存档目录枚举证据沿用47，不重复大测试。现有冻结存档扩展绑定全部12个实际 Save/zoom 文件，使用原文件名/大小/SHA/只读FD，不将ST8BASE或档案旧路径替换成ST9BASE。原函数401310实际拼接文件名并打开存档，在401424获取52字节文件信息；原代码只读取输出offset20/24的实际修改时间。自造arm64组件对真实FD做fstat/fstatfs，读取真实创建/访问/修改时间、大小、inode等；APFS fsid等其他字段只是POSIX投影，不声称NTFS卷序号/完整属性等价。本轮实际信息查询{len(metadata)}次，CompareFileTime原调用点40149a比较实际guest两份8字节FILETIME，共{len(compares)}次。返回符号依据 [Microsoft CompareFileTime](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-comparefiletime)，没有硬编码顺序或合成时间。

最终自然状态：{run['natural_state']}；原循环观测：{run['startup_counts']}。下一边界dialog.loc在隔离基线与原安装目录根下均实际不存在，位置检查记录于stage48-next-dialog-paths.json；原OPEN_EXISTING只读调用尚被白名单拒绝，没有添加替代文件。当前仍是获准i386 Unicorn参照加自造arm64平台适配，不能将菜单、图片、文件时间排序或存档名称出现算作原加载/城市逻辑通过。下一轮应从准确API边界继续完成原列表绘制，以本地实际游戏窗口截图定位ST9BASE后才点击，不盲点或写入游戏字段。

当前检查点三文件哈希见 stage48-result.json，完整映射内存/CPU/四线程/native语义blob冷恢复通过。源保护审计{audit['hash_checks']}项/{audit['unique_paths']}独立路径全不变，原源存档/档案与既有检查点不变；两种缺包在两位置仍缺失，既有缺失项与DOS链接全保持。所有本轮运行命令受60秒硬限，结果日志汇总于JSON，均未超时={out['all_commands_under_60_seconds']}。设备source_frames={run['native_audio']['source_frames']}；原PCM仅离线消费，没有新第三方执行、原DLL export执行、安装器、全局依赖、安全设置变更、上传、提交或发布。本轮仅本地证据，无审批阻塞。

恢复命令：/usr/bin/python3 samples/run_bounded.py stage49-restore . /usr/bin/python3 samples/checkpoint_startup48.py --resume {tag} --restore-only。完整实际记录、失败截图、API边界及命令日志见 evidence/stage48-result.json；当前窗口截图：{run.get('window_capture',{}).get('path')}。
'''
(R/'reports/stage48-missing-packages-and-original-save-list.md').write_text(report)
print(json.dumps(dict(checkpoint=tag,result=run['result']['status'],boundary=run['result'].get('boundary'),protected_hash_checks=audit['hash_checks'],file_information_calls=len(metadata),CompareFileTime_calls=len(compares),full_restore=True,all_commands_under_60_seconds=out['all_commands_under_60_seconds'])))
