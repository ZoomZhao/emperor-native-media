# -*- coding: utf-8 -*-
"""Freeze authorized tool repairs; no engine import or original execution."""
import json,sys
from pathlib import Path
sys.dont_write_bytecode=True
from common import ROOT,S,V,put,sha,verify
sys.path.insert(0,str(ROOT/'stage83/batch-v1/tools'))
baseline=json.loads((V/'evidence/protection-baseline.json').read_text());verify(baseline['hashes'])
ledger=json.loads((ROOT/'evidence/stage55-batch-ledger.json').read_text());assert len(ledger['executions'])==20 and ledger['remaining_processes']==80 and ledger['goal_closed']
claims=[p for p in(S/'runs').glob('batch-v1-round*/round.claim.json')];assert len(claims)==1
cfg=json.loads((V/'configs/authored-final.json').read_text());verify(cfg['pins'])
fixture=json.loads((S/'runs/repair-v2-authored-final/result.json').read_text());assert fixture['status']=='EXACT_KNOWN_STATE_AND_ORDERED_WRITES_MATCH'and not fixture['mismatches'];progress=json.loads((S/'runs/repair-v2-authored-final/reference/progress-observed.json').read_text());assert progress['observed']['write_records']==93
host=json.loads((V/'evidence/prepared-host-roundtrip.json').read_text());assert host['full_field_equalities']==178
cost=json.loads((V/'evidence/lazy-runtime-cost.json').read_text());component=json.loads((V/'evidence/component-tests.json').read_text());unlock=json.loads((V/'evidence/unlock-tests-v2.json').read_text());prefix=json.loads((V/'evidence/fixed-prefix-proof.json').read_text());ready=json.loads((V/'evidence/ready-gate-tests.json').read_text());assert component['case_count']==22 and len(unlock['cases'])==7 and ready['cases']==3
assert json.loads((S/'evidence/repair-v2-unlock-cli-ubsan.result.json').read_text())['returncode']==0
put(V/'evidence/result.json',{'status':'TOOL_REPAIR_OWN_VALIDATION_COMPLETE_ORIGINAL_EXECUTION_CLOSED','new_original_logic_or_SR_game_execution':0,'old_protected_hashes_verified':baseline['count'],'old_ledger':'20/80/closed','rounds_used':1,'remaining_rounds':9,'full_host_field_roundtrip':host,'lazy_freeze_measurement':cost,'component_cases':22,'ready_gate_cases':3,'real_C_Unlock_cases':7,'native_Unlock_UBSan_CLI_passed':True,'latest_authored_ABC':fixture,'live_authored_reference_progress_write_records':progress['observed']['write_records'],'current_fixed_prefix':prefix,'original_reference_or_native_game_execution_validated_by_this_repair':False,'unclosed_original_prefix_remains_explicit':True})
report='''Stage83 repair-v2：工具修复已验证，原执行仍关闭

最新接口：tools/pipeline_final.py --config stage83/repair-v2/configs/authored-final.json --all-fixture；也支持 --role capture/reference/native 分段调用，各role独占claim且60秒硬限，失败不得继续。已执行fixture路径不能重跑；调用记录见该目录及stage83/runs/repair-v2-authored-final。--describe-original只读，返回原执行CLOSED。engine_final.py对original kind在加载引擎前拒绝。所有旧批准保持原字节，没有40秒/2000万新授权，原轮次仍1 used/9 remaining。

二进制快照：自写版本化TLV codec（无pickle/任意对象构造），保存dict/list/tuple/set/deque、整数、float64、Unicode、raw bytes与deque.maxlen。固定cycle62p19的178个host字段都经完整decode→binary→reload逐字段相等检验，含opaque CPU/context原字节。每字段完整SHA256与原state/memory/context SHA保留。全host二进制210,079,418字节；一次离线制备6.858秒，完整重载验证23.888秒，不能把完整重载称为更快。实际运行采用HostStore按需加载11字段0.105秒、冻结0.033秒、全部字段hash校验0.108秒。未变化字段引用已固定内容；可变字段流式二进制重写，新增472,732字节，而非589MB漂亮JSON。源数据不被改写；未变字段读取后用只读容器拒绝嵌套修改。格式S83_FROZEN_NATURAL_SNAPSHOT_V2仍输出完整memory.bin、4份context、公开CPU、regions、heap/TLS/locks、API和有序写轨迹。未写的巨大平台字段保持完整内容引用与hash，实际guest CPU/内存不被原初host sidecar替代。

最终自造ABC：原参考端只执行自造x86 bytes，原生端只有自造LLVM函数。两端68条指令、93条有序写入、16,789,504字节内存、24个支持公开CPU字段及API/heap/TLS/locks一致。未知GS_BASE仍不升级为证明，原生opaque context仍明确是initial sidecar。无新SR游戏函数调用。多个版本保留，用于追溯快照/loader/门禁整理，最终以engine_final/pipeline_final和authored-final为准。

门禁：role_process.py每role创建独立进程组与独占claim，角色60秒涵盖其准备、执行和落盘，超时杀整个进程组。三个role不共用一个60秒包装器；纯控制包装最多190秒，对每个role仍60秒。自造三个短角色累计超单role限但各自通过；超时、重复claim、失败A不能启动B/C通过。Progress可配置初始phase/累计advances/entries/tail、原entry/return/SP、写上限；cross_cycle_allowed=true始终拒绝，本版不授权跨周期。code hook连接自然entry/return/tail观察，并输出独立progress-observed sidecar；旧Watch数据明确为initial sidecar，不清零或伪称已更新。最终自造参考live write计数93。平台写入先预留整个记录额度，再写；3字节平台末块支持。原版Unicorn多写指令中current-store停止语义尚没有运行证明，不能把自造门禁通过泛化为所有原指令。原执行入口关闭，未用它绕过该剩余证据缺口。Apple sandbox继承禁止网络和选定输出目录外写，C输出绑定仍固定。

22项组件测试、3项ready门禁及7项真实C Unlock测试通过。Unlock复用原ok/s23_unlock/obj/s29_unlock函数原文本与结构定义，出处hash见unlock-source-provenance.json；只重组无窗口state/像素存储。真实复制guest pixels并解除锁，二次Unlock返回失败，错误对象/指针与Blt、window、audio方法拒绝；无AppKit/Metal/原转换对象依赖。整套NativeDD constructor需要NSWindow、Metal及original转换模块，不能整体载入。首次UBSan dylib被Apple Python的sanitizer加载政策拒绝，日志与产物保留，未改安全设置。新普通C dylib用于own桥接，独立UBSan C executable检验同一真实函数通过。A中guest-only Unlock分支已准备，但原调用CPU/flags/动态目标环境未运行验证；仅自造桥接测试通过，不声称原完整平台移植成功。Blt后续闭包仍明确缺失。

固定cycle62p19前缀：完整剩余conversion到RET为7,559,981指令/1,591,309写记录；第一个未接调用Unlock→Unlock→Blt。只读栈/全局指针推导Blt源rect[0,0,1024,768]、dest[208,94,1232,862]，源RGB32 1024x768、primary RGB32 1440x900。若Blt遵循已验证旧平台成功语义，最小rect同步786,432条规范4B记录，累计2,377,741>2M；旧平台full-primary sync是1,296,000条，累计2,887,309>2M。此处未执行Blt或承诺其未来成功，但说明不能仅改指令/time cap。源phase22到phase8还需跨wrap，当前禁止；完整下游前缀没有闭合，报告明确whole_prefix_dependency_closure_complete=false。没有提出新的原执行审批。

现有历史14个cycle元数据中有cycle61a phase7（纠正把它说成phase5或没有7的推断），main最后轨迹4af311，挂起phase7 scheduler frame存在、官方5608/arch4/mode4 CPU tuple存在。但checkpoint current_tid=2（waiting）、thread4=runnable；不同源hash，不能强选main、把4改waiting或直接修改PC。该候选尚未授权/资格化；没有对其原context调用Unicorn或运行原逻辑。phase7之后真实渲染/线程/API前缀仍须审核。因此没有当前scope下已证明能执行的phase8 input。最经济的目标输入是自然CALL4ac338前、完整兼容的Unicorn2.1.4 checkpoint加实际caller/return/SP轨迹证据；原版源不变，真实活动对象不能手工构造。此输入目前尚缺，另需明确不同checkpoint的输入许可。

保护：6029项旧文件hash通过，旧批准/账本/检查点、Stage73成功和两次claim以及Stage83首轮失败/日志全部保持。没有提交、发布、上传、依赖安装或宿主安全设置改变。后续只读调度/输入路线比较另记，不继续扩展本版功能。
'''
(V/'reports/README.txt').write_text(report)
files=[p for p in V.rglob('*')if p.is_file()];files.extend(p for d in(S/'runs').glob('repair-v2-*')for p in d.rglob('*')if p.is_file());files.extend(p for d in(S/'builds').glob('repair-v2-*')for p in d.rglob('*')if p.is_file());files.extend(p for p in(S/'evidence').glob('repair-v2-*')if p.is_file());pins={str(p.relative_to(ROOT)):sha(p)for p in files};put(V/'manifest.json',{'hashes':pins,'old_protected_hashes_verified':baseline['count'],'original_execution':0,'rounds_used':1,'rounds_remaining':9});verify(pins);print(json.dumps({'status':'TOOL_REPAIR_SEALED','files':len(pins),'old_hashes':baseline['count'],'manifest_sha256':sha(V/'manifest.json'),'original_execution':0}))
