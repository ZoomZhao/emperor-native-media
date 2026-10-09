# -*- coding: utf-8 -*-
import hashlib,json,datetime
from pathlib import Path
R=Path(__file__).resolve().parents[1]
def read(n):return json.loads((R/'evidence'/n).read_text())
def sha(p):
 h=hashlib.sha256()
 with p.open('rb')as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
run=read('stage45-pcm45.json');restore=read('stage45-restore.json');audit=read('stage45-postflight.json');wav=read('stage45-button-wav-review.json');upload=read('stage45-stage44-screenshot-library.json');assert restore['source']=='pcm45'and restore['all_memory_CPU_four_threads_native_blobs_restored'];assert restore['route']['owned_pending_events']==1 and len(restore['miles_file_allocations'])==1
checkpoint=[]
for name in ['context.bin','memory.bin','state.json']:
 p=R/'runs/stage12-state-pcm45'/name;checkpoint.append(dict(path=str(p.relative_to(R)),bytes=p.stat().st_size,sha256=sha(p)))
bounded=[]
for p in sorted((R/'evidence').glob('stage45-*.result.json')):
 q=json.loads(p.read_text());bounded.append(dict(path=str(p.relative_to(R)),**q))
out=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),outcome='原菜单的 MOVE/DOWN 实际派发与按钮文件读取已通过；非空 PCM 样本附件明确拒绝；未完成菜单命令或加载城市',current_checkpoint='pcm45',current_checkpoint_files=checkpoint,current_CPU_PC=run['current_CPU_PC'],boundary=run['result']['boundary'],actual_original_WndProc_entries=run['original_WndProc_entries'],owned_input_route=run['route'],full_checkpoint_restore=restore['all_memory_CPU_four_threads_native_blobs_restored'],restored_pending_mouse_up=restore['route']['owned_pending_events'],actual_WAV_read=run['miles_file_calls'],WAV=wav,original_startup_counts=run['startup_counts'],natural_state=run['natural_state'],native_audio=run['native_audio'],protected_hash_checks=audit['hash_checks'],unique_protected_paths=audit['unique_paths'],all_frozen_input_hashes_unchanged=audit['all_frozen_inputs_unchanged'],all_absences_and_DOS_symlinks_unchanged=audit['all_observed_missing_inputs_still_missing']and audit['all_namespace_symlinks_unchanged'],bounded_logs=bounded,all_bounded_commands_under_60_seconds=all(not q['timed_out']and q['duration_seconds']<60 for q in bounded),Library_screenshot_upload=upload,menu_command_completed=False,city_load_hook_observed=False,city_tick_proven=False,new_original_function_static_conversion_proven=False,full_native_WinMain_proven=False,original_MSS_Bink_M3D_exports_executed_this_stage=False,original_code_patched=False,gameplay_state_injected=False,global_input_events_posted=False,global_cursor_moved=False,foreground_activation_requested=False,source_PCM_sent_to_device=False,remaining=['真实非空 PCM 样本绑定、播放/结束和资源所有权合同','菜单命令及单人档案、读档选择链','ST9BASE 实际文件路径绑定及原城市更新','Windows GlobalAlloc 锁注册/错误状态、精确 Miles 混音与 Windows 全局鼠标等价未证','整体静态重编译与长期后台 AppKit 生命周期未证'],approval_blocker=None,worth_continuing='可继续一个约20分钟的离线非空 button.wav 样本合同试验；在该边界解决前，不能用强行改状态或空样本成功绕过菜单音效')
(R/'evidence/stage45-result.json').write_text(json.dumps(out,indent=2));print(json.dumps(dict(checkpoint=checkpoint,boundary=out['boundary'],hash_checks=audit['hash_checks'],bounded_commands=len(bounded),all_bounded_commands_under_60_seconds=out['all_bounded_commands_under_60_seconds'],library_file_id=upload['library_file_id'])))
report=f'''本轮 Stage45（2026-10-07，约20分钟）从 stable44 原菜单检查点出发，真实窗口内输入已进入原版 WndProc 和按钮处理，完整原版 button.wav 已通过实际只读本机文件接口进入 guest32 内存。菜单命令、ST9BASE 读档和原城市更新尚未通过。当前停止点是 `_AIL_set_sample_file@12`，thunk `0x61001960`、原返回位置 `0x76275e`，实参为样本 `0x5e000101`、WAV 指针 `0x1c16d080`、block0。新检查点 `runs/stage12-state-pcm45` 已冷恢复，全部内存、CPU、四线程及本机快照相等，待派发的鼠标松开事件仍为1。

WindowMoved 已用实际 AppKit 事件核实：type13/subtype4，事件窗口和任务窗口均为15967，实际原生框架矩形保持(208,38,1024,796)。仅该同窗口、未变几何的事件交给 AppKit 自身处理，不将未知宿主事件假装为空。原版窗口移动消息已有前序真实几何回调。实际客户区为1024×768、原点(208,94)、backing scale2；本次点击(360,336)转换为屏幕(568,430)，不重复乘缩放。

最终输入方式是任务自有、保留实际不可变 NSEvent 对象的窗口 FIFO；没有向系统发送全局输入，不移动物理光标，不激活或抢占前台。FIFO 事件记录随检查点保存，恢复时在新任务窗口重建本机事件。独立样例对 MOVE/DOWN/UP 和 FIFO 3→2→1→0、MSG32 lParam、坐标及冷恢复通过。实际原游戏已接收 WM_MOUSEMOVE0x200 和 WM_LBUTTONDOWN0x201；WM_LBUTTONUP0x202 尚在队列中。DefWindowProc 的客户区鼠标默认合同仅覆盖本次实际 MSG 和无 Alt/F10 菜单锁的窗口内测试域；GetCursorPos 为隔离 guest 指针域，不声称等价于 Windows 全局物理光标。

初次 NSApp postEvent 方案只派发了 MOVE，DOWN/UP 不在快照中，已以失败日志保留并更换为上述可持久化的实际 NSEvent FIFO。旧2600字节 route 前缀仅保留作历史证据；后续从3372字节 route 的 pcm45 恢复。最初自动截图失败也有日志；后来本地 buttons45 窗口截图成功，与 Stage44 同哈希，原 UI 尚未完成新的重绘，不能当作交互或加载通过的证据。

按钮文件6120字节，SHA256 {wav['sha256']}；原版与冻结副本逐字节相等。PCM 为单声道、22050 Hz、16位、3001帧（6002字节）。原 `_AIL_file_read(name,0)` 返回位置0x7626d6，实际只读 native pread 完整读入，并分配6124字节的任务 guest32 所有内存，RIFF 原始字节位于返回基址。现有固定合法 mss32.dll 的只读反汇编21110280证明 dest0/-1 分配文件大小+4，只有 dest=-1 才有长度前缀；本次为0，没有虚构前缀。未执行任何原 DLL 导出。Windows/Miles 的 GlobalAlloc 锁注册、完整错误状态及释放合同未证，已在接口证据中标为 false；本轮只实现受限成功读取与可恢复分配。

原代码读取后自然调用 `_AIL_init_sample@4`，再进入非空 `_AIL_set_sample_file@12`。此处明确停止，未用现有空样本接口冒充真实 PCM 绑定或播放。原菜单的按键逻辑因此仍在按钮音效调用中。城市 load534a30、scheduler4ac2b0、cycle tail4ac650 无观测入口；gate、city_phase、人口和国库均为0。没有直接调用原城市入口或写游戏状态。

音乐延续 Stage44 的真实 PCM 离线队列消费。实际音频设备 render_frames={run['native_audio']['render_frames']}、source_frames=0、zero_frames={run['native_audio']['zero_frames']}、bad_buffers=0；游戏 PCM 没有送到设备。本轮无原安装器、原实现、全局依赖、安全设置、提交或发布操作。

冻结审计：{audit['hash_checks']} 项哈希、{audit['unique_paths']} 个独立路径全部不变，缺失输入与 DOS 目录符号链接不变。原固定 EXE、原资源/存档、SR 工具及前序冻结检查点均未变。运行命令均由60秒硬限包装；结果详见 stage45-*.result.json，已知拒绝为真实合同边界，并非超时。程序仍为原 i386 Unicorn 参照加任务自造 arm64 平台接口，本轮没有新增原函数静态转换，也没有证明整条 WinMain 原生或城市逻辑通过。前序 0x4190xx 应称 RGB555 像素转换循环，不能作为菜单扫描或城市逻辑通过证据。

截图上传按用户2026-10-07 01:22:13 UTC 明确“允许”执行，仅一张 evidence/stage44-stable44-window.png。Library 精确同名搜索无已有项；创建成功且返回 xattrs 已用当前 Library 官方 helper 应用并验证，截图内容哈希不变。library_file_id={upload['library_file_id']}，不扩大上传范围、不分享链接、不上传游戏代码或数据。Python3.9 以 trusted compile 的 future annotations 兼容标志执行未改动的官方 helper。

可继续一个有限的离线非空 button.wav 样本绑定试验，验证真实3001帧数据、播放/结束状态、持久化及释放，再重新走菜单读档。无需为任务自造代码再申请第三方执行许可；如需调用新的原 DLL 导出或新第三方工具，必须逐次交回精确审批。本轮无新的审批阻塞，剩余是未实现且未证的技术合同。

主要证据：evidence/stage45-result.json、stage45-pcm45.json、stage45-restore.json、stage45-postflight.json、stage45-pump-probe.json、stage45-button-wav-review.json、stage45-stage44-screenshot-library.json；本机输入实现 samples/stage45_pump.m，续跑 samples/checkpoint_startup45.py。下一步冷恢复命令：/usr/bin/python3 samples/run_bounded.py stage46-restore . /usr/bin/python3 samples/checkpoint_startup45.py --resume pcm45 --restore-only。先实现非空样本附件合同，再续跑，避免跨过当前拒绝点。
'''
(R/'reports/stage45-owned-menu-input-and-button-boundary.md').write_text(report)
