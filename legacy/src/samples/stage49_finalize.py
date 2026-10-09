import json,hashlib,sys,datetime
from pathlib import Path
R=Path(__file__).resolve().parents[1];tag=sys.argv[1]
def read(n):return json.loads((R/'evidence'/n).read_text())
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
run=read('stage49-'+tag+'.json');audit=read('stage49-postflight.json');restore=read('stage49-restore.json');assert restore['source']==tag and restore['all_memory_CPU_four_threads_native_blobs_restored']
files=[]
for name in ['context.bin','memory.bin','state.json']:
 p=R/'runs'/('stage12-state-'+tag)/name;files.append(dict(path=str(p.relative_to(R)),bytes=p.stat().st_size,sha256=sha(p)))
logs=[dict(path=str(p.relative_to(R)),**json.loads(p.read_text()))for p in sorted((R/'evidence').glob('stage49-*.result.json'))];sources=[dict(path=str(p.relative_to(R)),sha256=sha(p))for p in sorted((R/'samples').glob('*49*'))if p.is_file()]
chain=[];cursor=tag
while cursor!='sorted48':
 d=read('stage49-'+cursor+'.json');chain.append(d);cursor=d['source']
reads=[q for d in reversed(chain)for q in d.get('native_file_read_calls_this_run49',[])];target=[q for q in reads if q['guest_path'].lower().endswith('save\\zoom\\st9base.sav')];intervals=sorted((q['position_before'],q['position_after'])for q in target if q['bytes']);merged=[]
for lo,hi in intervals:
 if merged and lo<=merged[-1][1]:merged[-1][1]=max(hi,merged[-1][1])
 else:merged.append([lo,hi])
events=run.get('city_observations49',[]);returns=[q for q in events if q['kind']=='returned'];load_returned=any(q['label']=='city_load'and q['EAX']==0 for q in returns);tail_returned=any(q['label']=='city_cycle_tail'for q in returns);scheduler=[q for q in returns if q['label']=='city_scheduler'];loaded=load_returned and bool(target);updated=loaded and tail_returned and bool(scheduler)
pre=read('stage49-'+tag+'-before-dispose.json');post=read('stage49-'+tag+'-after-dispose.json');assert pre['native_lifecycle']['actual_NSWindowWillClose_count']==0 and post['native_lifecycle']['actual_NSWindowWillClose_count']==1
opened=any(q['name']=='CreateFileA'and q.get('actual_sha256')=='fd97f2f426e4b69e311d5a8c8596b170f7246df8ad102c4173fcb476d43689e1'and q['return_value']!=0xffffffff for q in run['readonly_calls49']);selected=read('stage49-original-selected-save.json');assert selected['original_selected_name']=='ST9BASE'and selected['original_selected_index']==3
out=dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),current_checkpoint=tag,current_checkpoint_files=files,current_sources=sources,result=run['result'],natural_state=run['natural_state'],original_startup_counts=run['startup_counts'],original_city_observations=events,pending_original_city_frames=run.get('city_frames49',[]),original_selected_save=selected,selected_save_actual_window_capture=read('stage49-city49.json')['window_capture'],actual_final_window_capture=run.get('window_capture'),readonly_contract=read('stage49-readonly-contract.json'),readonly_contract_probe=read('stage49-readonly-contract-probe.json'),readonly_calls=run['readonly_calls49'],stage49_checkpoint_chain=[q['state']for q in reversed(chain)],actual_native_ReadFile_this_stage=reads,actual_ST9BASE_ReadFile=target,actual_ST9BASE_read_bytes=sum(q['bytes']for q in target),actual_ST9BASE_unique_read_bytes=sum(hi-lo for lo,hi in merged),actual_ST9BASE_read_intervals=merged,all_actual_read_slices_equal=all(q['source_slice_equal']for q in reads),Load_Saved_Mission_list_drawn=True,ST9BASE_selection_original_buffer_and_visual_verified=True,ST9BASE_real_readonly_open_succeeded=opened,ST9BASE_loaded_through_menu=loaded,city_load_success_return_observed=load_returned,original_city_scheduler_returns=len(scheduler),original_city_cycle_tail_return_observed=tail_returned,city_update_proven=updated,new_original_static_conversion_proven=False,full_native_WinMain_proven=False,full_checkpoint_restore=True,native_before_dispose=pre,native_after_dispose=post,native_audio=run['native_audio'],protected_hash_checks=audit['hash_checks'],unique_protected_paths=audit['unique_paths'],all_frozen_inputs_unchanged=audit['all_frozen_inputs_unchanged'],all_absences_and_DOS_symlinks_unchanged=audit['all_observed_missing_inputs_still_missing']and audit['all_namespace_symlinks_unchanged'],bounded_logs=logs,all_commands_under_60_seconds=all(not q['timed_out']and q['duration_seconds']<60 for q in logs),source_profile_or_save_modified=False,isolated_profile_modified_this_stage=False,game_fields_injected=False,original_code_patched=False,global_input_events_posted=False,global_cursor_moved=False,foreground_activation_requested=False,source_PCM_sent_to_device=False,original_DLL_exports_executed=False,files_uploaded=False,approval_blocker=None,worth_continuing=True)
(R/'evidence/stage49-result.json').write_text(json.dumps(out,indent=2))
report=f'''Stage49，2026-10-07。从sorted48继续，已实际绘制原存档列表，原代码选择ST9BASE：选择缓冲区10ddcd0为ST9BASE、索引88ebe4为3，之后同一原游戏窗口顶部也显示ST9BASE，再点击To the city。当前检查点{tag}；ST9BASE真实只读打开成功={opened}；通过原菜单加载完成={loaded}，原城市更新完整周期观察={updated}。不将图片、选中名称或函数进入当作加载完成。

本轮建立统一只读文件合同，源码stage49_readonly.py/.c、checkpoint_startup49.py，合同stage49-readonly-contract.json。范围仅已冻结任务基线游戏树，现有文件还须属于48审计清单并匹配SHA/实际大小/device/inode，不增加来源或读取其他用户文件。路径先GBK解码，处理斜杠、重复分隔符、点和树内点点；绝对路径只能C:\\Emperor，相对路径只能相对实际FD支持的虚拟cwd。拒绝树外、UNC/device/ADS/含歧义尾点空格、未证DOS短名、通配符等路径。每个实际目录分量通过readonly openat/O_NOFOLLOW，使用真实目录项大小写不敏感匹配，拒绝冲突与符号链接；自造arm64模块直接dup已验证FD而不重新按路径打开，避免父目录替换时逃逸。Native数据按2MiB块读取，最多64MiB；host指针/FD不写语义检查点。

CreateFile仅GENERIC_READ/OPEN_EXISTING、flags0或128、template0及已证12字节SECURITY_ATTRIBUTES。真实ENOENT最后分量→2、缺父目录/ENOTDIR→3、EACCES/EPERM→5，另有有限资源错误映射；写入/create/delete/async请求继续明确拒绝。已有guest只读句柄的share冲突被检查；Windows跨进程分享锁、ACL、短名和完整NTFS元数据等价未证。API基本语义依据 [Microsoft CreateFileA](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea)、[路径命名](https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file)、[错误码](https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes--0-499-)；自造合同是有限兼容范围，不把安全拒绝伪装成Windows实测错误。21项独立检查包括规范路径、目录逃逸/设备等拒绝、真实缺文件/缺父路径、大小写不同的ST9BASE实际FD/完整SHA，以及自造临时目录中的父/叶符号链接逃逸拒绝，均通过。

CreateDirectoryA也统一通过只读解析确认真实已有目标才返回0/error183，没有mkdir；实际目录创建仍拒绝，依据 [Microsoft CreateDirectoryA](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createdirectorya)。原To the city调用save\\Multiplayer时该目录真实存在。dialog.loc通过统一合同真实只读openat得到ENOENT，原CreateFile返回ffffffff/error2后自然绘制Loading Campaign页面；没有逐个文件打补丁、替代文件或素材。第一遍恢复因清单使用data而实际目录名为Data导致冻结SHA索引未命中，7.816秒明确拒绝，未执行原继续逻辑；修正清单键的casefold并拒绝哈希冲突后42.387秒原续跑通过，失败日志保留。GBK中文存档名在English原游戏字体中仍显示乱码，本轮目标ASCII ST9BASE已清楚显示，完整DBCS字形/文件名兼容未证。

ST9BASE点击(400,380)的MOVE/DOWN先处理，UP在下一有限续跑消费；原选择缓冲区确为目标，旧截图顶部ST9AFTR来自尚在41904c原RGB像素转换的绘制过程，未误读为加载旧存档。随后city49截图顶部ST9BASE和黄色行都可见，To the city点击(650,518)由本窗口私有不可变NSEvent FIFO输送，后续继续已存UP，不添加重复输入。所有点击前检查实际WindowServer归属、可见性与非黑不透明截图，保持非key/nonactive，另一进程仍在前台；关闭通知清理前0、清理后1。窗口恢复沿用48实际游戏面板验证，不重复独立样例或上传截图。

原加载/调度/周期尾入口534a30/4ac2b0/4ac650被动观察记录真实返回PC和guest栈；只有同线程原cdecl返回使ESP恢复到entrySP+4才记returned，挂起帧保存为语义数据并在冷恢复重装观察hook。原静态scheduler4ac4c8自然将phase加1，到51归零后跳4ac650；没有改变原指令或游戏字段。当前成功加载返回={load_returned}，调度真实返回{len(scheduler)}次，周期尾返回={tail_returned}；实际ST9BASE ReadFile {len(target)}次、累计{out['actual_ST9BASE_read_bytes']}字节、唯一覆盖{out['actual_ST9BASE_unique_read_bytes']}/168643字节，每次guest片段与真实native FD读取相等={out['all_actual_read_slices_equal']}。文件提前SHA验证的整文件pread不算原ReadFile。具体调用/返回与覆盖区间见结果JSON。

最终自然状态：{run['natural_state']}。下一边界：{json.dumps(run['result'].get('boundary'),ensure_ascii=False)}，状态{run['result']['status']}。仍是获准i386 Unicorn参照加自造arm64平台接口，没有本轮新原函数AOT转换，也未证明全native WinMain或Windows API全集等价。值得继续：从该准确检查点处理实际边界，沿原加载/城市循环获取完整逻辑证据。

完整映射内存/CPU/四线程/native语义blob冷恢复通过，三文件SHA见结果JSON。源保护审计{audit['hash_checks']}项/{audit['unique_paths']}独立路径全不变；原源/冻结存档和档案、既有检查点、缺失项与DOS链接均不变。所有有界命令均在60秒内且未超时={out['all_commands_under_60_seconds']}，失败及wall_budget日志保留。设备source_frames={run['native_audio']['source_frames']}，源PCM仅离线；没有原DLL export执行、新第三方执行、上传、安装器、全局依赖或安全设置变更、提交、发布。没有当前审批阻塞。

恢复命令：/usr/bin/python3 samples/run_bounded.py stage50-restore . /usr/bin/python3 samples/checkpoint_startup49.py --resume {tag} --restore-only。主要交付：evidence/stage49-result.json、stage49-readonly-contract-probe.json、stage49-original-selected-save.json、stage49-{tag}.json、stage49-restore.json、stage49-postflight.json；截图evidence/stage49-city49-window.png和{run.get('window_capture',{}).get('path')}均仅本地。
'''
(R/'reports/stage49-readonly-contract-and-ST9BASE-selection.md').write_text(report)
print(json.dumps(dict(checkpoint=tag,result=run['result']['status'],boundary=run['result'].get('boundary'),ST9BASE_loaded=loaded,city_update=updated,read_calls=len(target),unique_read_bytes=out['actual_ST9BASE_unique_read_bytes'],protected_hash_checks=audit['hash_checks'],full_restore=True,all_commands_under_60_seconds=out['all_commands_under_60_seconds'])))
