# -*- coding: utf-8 -*-
"""Verify old evidence and seal this bounded development task."""
from core import *
import subprocess,struct,collections
baseline=json.loads((V/'evidence/protection-baseline.json').read_text());verify(baseline['hashes'])
ledger=ROOT/'evidence/stage55-batch-ledger.json';assert sha(ledger)=='4e499d8560cc4c7a9bc4a2e725e27f502fa13979de6366e9f66bd06f237edcc9';state=json.loads(ledger.read_text());assert len(state['executions'])==20 and state['remaining_processes']==80 and state['goal_closed'];assert len(list((ROOT/'stage83/runs').glob('batch-v1-round*/round.claim.json')))==1
frozen=json.loads((V/'runs/own-capture-v2/frozen.json').read_text());verify(frozen['hashes']);tests=json.loads((V/'evidence/component-tests-v2.json').read_text());assert tests['case_count']==25 and all(r['passed']for r in tests['cases']);cpu=json.loads((V/'evidence/cpu-restore.json').read_text());assert cpu['original_engine_emu_start_calls']==0 and cpu['authored_instructions']==12
full=json.loads((V/'evidence/full-size-memory-v2.json').read_text());assert full['platform_canonical_writes']==2475702;pre=json.loads((V/'evidence/capture-preflight.json').read_text());assert pre['status']=='ORIGINAL_CAPTURE_CLOSED_EXPLICIT_PRECHECK_BOUNDARIES'
# Fresh subprocess proves original refusal before any engine import or claim.
command=['/usr/bin/python3','-B','-c',"import sys;from pathlib import Path;sys.path.insert(0,str(Path('stage83/phase7-v1/tools').resolve()));import capture;sys.argv=['capture','--original-run'];\ntry:capture.main()\nexcept Exception as e:print(str(e));print('unicorn_imported='+str('unicorn' in sys.modules));raise SystemExit(2)"]
q=subprocess.run(command,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5);(V/'evidence/original-gate.stdout').write_bytes(q.stdout);(V/'evidence/original-gate.stderr').write_bytes(q.stderr);assert q.returncode==2 and b'unicorn_imported=False'in q.stdout
own=json.loads((V/'runs/own-capture-v2/snapshot.json').read_text());assert own['budget']['shared_steps']==55 and own['budget']['shared_writes']==108 and own['memory_bytes']==65536 and own['scheduled']==[4,1,2,4,1]
put(V/'evidence/result.json',{'status':'FOUR_THREAD_OWN_CAPTURE_AND_REAL_PLATFORM_DEVELOPMENT_PASS_ORIGINAL_PREFIX_STILL_CLOSED','old_pins_verified':baseline['count'],'ledger':'20 used / 80 remaining / closed','original_rounds_used':1,'remaining_rounds':9,'new_original_instructions_or_SR_game_functions':0,'official_original_CPU_restore_threads':4,'empty_clone_authored_probe_instructions':12,'joint_cases':25,'authored_capture_steps':55,'authored_capture_writes':108,'authored_scheduled_threads':[4,1,2,4,1],'authored_frozen_memory_bytes':65536,'full_size_platform_write_records':2475702,'full_size_native_memory_seconds':full['seconds'],'conditional_same_frame_guest_plus_platform_write_records':5622198,'conditional_same_frame_plus_Stage73_isolated_write_records':5672087,'conditional_pixel_instruction_lower_bound':14942208,'whole_original_prefix_closed':False,'preflight_blockers':pre['blockers'],'original_gate_fresh_process_engine_imported':False,'known_GS_BASE_noop_not_promoted':True,'failed_own_capture_v1_and_size_benchmark_v1_retained':True})
report='''本轮完成了四线程CPU恢复、有限线程/事件模型接入、自造捕获与无窗口平台内存验证；真实phase7前缀仍未闭合，原执行入口保持关闭，没有消耗剩9轮。

可运行接口（工作目录 <LOCAL_LEGACY_TASK>）：
  /usr/bin/python3 -B stage83/phase7-v1/tools/capture.py --preflight
  /usr/bin/python3 -B stage83/phase7-v1/tools/capture.py --self-capture --output stage83/phase7-v1/runs/NEW_OWN_OUTPUT
  /usr/bin/python3 -B stage83/phase7-v1/tools/restore_cpu.py
self-capture只允许自造代码，每新输出独占一次own claim，不消费原round；已有输出不得重跑。restore_cpu的既有结果在evidence/cpu-restore.json，输出独占，已有路径会拒绝覆盖。--original-run在任何Unicorn导入/claim前拒绝，即使修改参数也没有原运行通道。CPU恢复不是调度游戏指令。具体pins见configs/source.json、evidence/capture-preflight.json。

实质结果：四线程官方5608-byte context tuple恢复，5568-byte完整CPU payload相等，297,521,152字节原映射内存hash未变。4个空克隆合计12条自造FS读/栈push-pop通过；原上下文引擎emu_start=0。公开CPU：tid1 PC4af311/SP700ffd68，tid2 PC610005f0/SP710ffe4c，tid3 PC610005f0/SP712ffe74，tid4 PC5fc043/SP714ffe2c。GS_BASE在32位绑定的读是no-op/未定义项，保留警告、不宣称该字段通过；完整opaque payload仍由官方接口比对，未猜私有寄存器偏移。

既有ThreadKernel class正文逐字复用，去掉加载整个旧运行时的依赖；runner只加目标处即时冻结，选择/唤醒顺序保留：last=1、worker_first=true、prefer=None、quantum100000、wake-before-pick。真实C事件核心复用旧stage13_events.c；恢复blob、auto/manual reset、超时等待返回、每线程TLS/error、FS/TEB与递归临界区行为有联合负例。自造初态current2 waiting、1/4 runnable、3 waiting；实际调度4→1→2→4→1，55条自造指令、108条写入，到自然CALL的首条目标指令前停止。完成真实Lock→guest像素改写→Unlock→Blt→32位回调栈/返回绑定；GPU/窗口/音视频/网络和SR游戏库均未载入。新增指令和写额度始终全线程共享，未给worker免费预算；thread3自然可运行时停止、不改成假waiting。

25项联合正负例通过：等待/唤醒/超时/infinite wait，auto/manual reset与native blob roundtrip，FS/TLS，争用临界区/错误线程Leave，跨线程共享instruction/write/wall cap，整个Lock写计划超额在pixels/descriptor/native blob任何提交前拒绝，未知APC拒绝，callback目标、32位token、owner/SP返回与强设目标PC拒绝。own-capture-v2已封存四线程当前官方CPU、64KiB内存、events/surface真实状态和108条有序写入。此前own-capture-v1执行与保存通过但因相对路径relative_to在封存时失败，原目录保留；新版本统一绝对路径后在全新目录通过，没有重用旧own claim。

平台实现复用真实s23_lock/unlock、s29_lock/unlock及s34_is_lost函数。32bit Blt按既有s23裁剪顺序、真实snapshot overlap copy、pitch/padding实现；16bit Blt需要被禁止执行的SR转换函数，明确拒绝。Lock可处理555/565真实16bit表面，返回原108字节描述符与完整pixel同步。IsLost的HRESULT只由真实resident Surface内存决定；旧GPU存在项只是诊断，不参与HRESULT，因此新无窗口实现诚实返回GPU诊断0，未伪造设备成功。

全尺寸自造表面基准通过：1440x900x32 primary，1024x768x32 source，1024x768x555 input；两个Lock、两个Unlock、Blt、IsLost实测2,475,702条平台规范写记录，像素copy与全部padding相等。初版估算将16bit表面1,572,864 bytes除4少算一半，故原2,279,094 assertion失败；失败记录保留，v2按实际3,93216（393216）条16bit同步验证，未隐藏失败。准确分解为393216+27（Lock16）+786432+27（Lock32）+1296000（full-primary Blt）；Unlock/IsLost无guest写。范围在evidence/full-size-memory-v2.json。这是纯自造数据/真实native平台操作计时，不是原conversion执行性能。

真实最短前缀证据：当前worker4下一条CALL@5fc044读公共EBX=610000c0，明确绑定EnterCriticalSection，参数ESI=10bc428，原临界区depth0/ownerNone；初始callback计数0。但后续5fc139 CALL[EDX]来自可变队列/对象，不能由当前空队列证明全段恒空。原phase7到phase8仍只需1次productive advance、不wrap；源Watch保留entries3、advances2、tail0和2份pending frame，数据验证正确接受7→8、不清零。

只读原cycle61a→cycle61p07累积API日志前缀完全相同：后续记录780次GetTickCount、6次timeGetTime、F2/F5/F6三次GetAsyncKeyState、两Lock/两Unlock/一Blt/两IsLost。该窗口还包含推进至phase9的历史后缀，不把所有780次都误称CALL前。GetAsyncKeyState返回地址477fb9/477fce/477fe3，键113/116/117；其0返回由显式controlled keyboard设备产生，domain935b7dbc8d4775c10fddda87940098035658575c667d80d81b925b9d7c501629，不是物理键盘也不是stub许可。当前不扩键盘范围、不消费/重置该设备latch，捕获器明确失败。无窗口renderer的Lock/Blt/IsLost已闭合并测实内存，但可变真实callback目标域、live clock ABI和原source Watch code-hook绑定、原多store/REP precommit证书仍缺；自造单store与平台事务负例不能代替原多store故障证明。因此没有完整可运行原捕获器，也没有原自然phase8输入。

预算：若原同一全幅帧保持已观察路径，仅像素19条/786432个像素就有14,942,208条原指令，另有3,145,728条逐byte store+768条row counter store；加实测平台操作为5,622,198条记录，另加已通过孤立Stage73原切片49,889写为5,672,087（并发后仍是条件估计，不是全前缀认证）。纠正先前报告：214是六函数静态指令数，Stage73真实动态执行234,942步，不是214步；静态成果728bytes/214instructions保留，旧报告不改写。全原前缀指令/写/时间准确上限仍未证明；当前A6M/2M/8sec明确不足。自造平台几秒不能证明原像素循环能在8sec或role60sec（含prep/freeze）内完成，不能再提出40sec/20M不完整请求。

下一步最小工程为：只在现有受控输入domain内复用真实query/latch（若继续需明确该范围，不能改成常零或读取物理键盘）、复用真实clock语义并接入保留pending frame的Watch hook、确定worker可变callback域或在该准确CALL前拒绝、补原多store预留/fault证书。到这些独立验证完成后，才能给唯一一份新原取样的完整配置：三文件cycle61a新input pins；主线程1及worker2/4原调度许可，3自然ready则停；one advance7→8/no wrap/no整城；确切API/输入合同；经测量的shared A指令/写/guest和总role预算。当前不申请原执行，也不自动扩限。

保护复核：6651项旧pin全通过；旧20 used/80 remaining/closed与原SHA一致，原batch仍仅round01一claim（A失败，B/C没起）。本轮原指令、原参考和SR游戏函数调用0，剩9轮；任何新A前独占round claim，失败也用1整轮，worker共用A上限；不重试round01、不重开旧账本。没有安装、上传、提交、发布或系统设置改变。
'''
report=report.replace('3,93216（393216）','393,216')
(V/'reports/README.txt').write_text(report)
pins={str(p.relative_to(ROOT)):sha(p)for p in V.rglob('*')if p.is_file()and p.name!='manifest.json'};put(V/'manifest.json',{'status':'PHASE7_TOOL_DEVELOPMENT_SEALED_ORIGINAL_EXECUTION_CLOSED','files':len(pins),'hashes':pins});verify(pins);print(json.dumps({'status':'PHASE7_DEVELOPMENT_SEALED','files':len(pins),'old_pins':baseline['count'],'remaining_rounds':9,'manifest_sha256':sha(V/'manifest.json')}))
