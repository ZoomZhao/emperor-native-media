"""Freeze a concrete executable and combined request, without issuing an approval."""
from probe_common import *
import subprocess,ast

def main():
 base=json.loads((N/'evidence/protection-baseline.json').read_text());verify(base['hashes']);check_old()
 assert base['count']==6960 and accounting()=={'used':1,'remaining':9,'maximum':10}
 for p in(N/'tools').glob('*.py'):ast.parse(p.read_text())
 tests=json.loads((N/'evidence/own-executor-tests-v7.json').read_text());assert tests['case_count']==15 and all(r['passed']for r in tests['cases'])
 # Final behavioral runs bind exactly the executor bytes being sealed.
 for row in tests['cases']:
  out=ROOT/'stage83/runs'/('phase7-probe-own-'+row['case']+'-v7')
  verify(json.loads((out/'worker.started.json').read_text())['executor_sources'])
 perf=ROOT/'stage83/runs/phase7-probe-own-full-performance-v3';p=json.loads((perf/'process.json').read_text());s=json.loads((perf/'snapshot.json').read_text());r=json.loads((perf/'result.json').read_text())
 assert p['elapsed_seconds']<60 and not p['timed_out']and r['snapshot_complete']and r['boundary']['kind']=='SHARED_GUEST_WALL_CAP'
 readonly=ROOT/'stage83/runs/phase7-probe-own-source-readonly-freeze-v3';a=json.loads((N/'evidence/full-host-sync-audit.json').read_text());assert a['host_fields']==178 and a['identical_binary_fields']==176 and a['original_engine_emu_start_calls']==0
 # Transitive runtime dependencies are existing, pinned self-authored/trusted helpers.
 reuse=[
  'stage83/phase7-v2/tools/core.py','stage83/phase7-v2/tools/models.py','stage83/phase7-v2/tools/thread_kernel.py','stage83/phase7-v2/tools/integrate.py',
  'stage83/phase7-v2/builds/clock.dylib','stage83/phase7-v2/evidence/clock-build.json',
  'stage83/phase7-v1/configs/source.json','stage83/phase7-v1/prepared-host/host.manifest.json',
  'stage83/phase7-v1/evidence/build.json','stage83/phase7-v1/evidence/build-v2.json',
  'stage83/phase7-v1/builds/events.dylib','stage83/phase7-v1/builds/surfaces-v2.dylib',
  'stage83/repair-v2/tools/common.py','stage83/repair-v2/tools/binary_host.py','stage83/repair-v2/tools/write_gate.py','stage83/repair-v2/configs/role-write-policy.sb',
  'samples/stage73_cpu.py','samples/auto_i386_analysis.py','samples/stage61_input.py','samples/stage60_controlled_input_plan.py','samples/stage54_watch.py',
  'samples/stage13_clock.c','samples/stage13_clock_profile.h','evidence/stage61-approved-input-domain.json',
  'vendor-tools/unicorn-2.1.4/unicorn/lib/libunicorn.2.dylib']
 pins={str(p.relative_to(ROOT)):sha(p)for p in(N/'tools').glob('*.py')};pins[str((N/'configs/probe.json').relative_to(ROOT))]=sha(N/'configs/probe.json')
 pins.update({n:sha(ROOT/n)for n in reuse})
 for p in(ROOT/'vendor-tools/unicorn-2.1.4/unicorn').rglob('*.py'):pins[str(p.relative_to(ROOT))]=sha(p)
 put(N/'runtime-pins.json',{'format':'PHASE7_PROBE_RUNTIME_PINS_V1','hashes':pins,'files':len(pins),'original_execution_authorized':False})
 c=json.loads((N/'configs/probe.json').read_text());command=['/usr/bin/python3','-B',str(N/'tools/probe.py'),'--run','--config',str(N/'configs/probe.json'),'--approval',str(ROOT/c['approval_path'])]
 implemented={'kernel32.dll':{'GetTickCount','QueryPerformanceCounter','QueryPerformanceFrequency','InterlockedIncrement','InterlockedDecrement','HeapAlloc','HeapReAlloc','HeapFree','HeapSize','SetEvent','ResetEvent',
  'TlsAlloc','TlsSetValue','TlsGetValue','TlsFree','GetCurrentThreadId','GetLastError','SetLastError','InitializeCriticalSection','EnterCriticalSection','LeaveCriticalSection','DeleteCriticalSection','WaitForSingleObject','WaitForSingleObjectEx','ExitThread','GetExitCodeThread','CloseHandle'},
  'winmm.dll':{'timeGetTime'},'user32.dll':{'GetAsyncKeyState'},'authored.dd':{'2.25','2.32','2.5','2.24'}}
 h=HostStore(ROOT/source_config()['host_manifest']);imports=h['imports'];supported=[{'address':hex(pc),'dll':dll,'name':name}for pc,(dll,name)in imports.items()if name in implemented.get(dll.lower(),set())]
 put(N/'evidence/automation.json',{'import_labels_restored_automatically':len(imports),'manually_patched_import_addresses':0,'implemented_import_endpoints':len(supported),'implemented_fraction':len(supported)/len(imports),'implemented_endpoints':supported,
  'generic_instruction_precommit':True,'source_EXE_or_guest_code_patches':0,'manual_opcode_or_callback_target_allowlist_patches':0,'unsupported_forms_stop_before_execution':True,'all_paths_or_all_inputs_coverage_claimed':False,'host_fields_retained':178,'readonly_binary_identical_fields':176})
 packet={'status':'REQUEST_ONLY_NOT_AUTHORIZATION','configuration_sha256':sha(N/'configs/probe.json'),'runtime_pins_sha256':sha(N/'runtime-pins.json'),
  'command':command,'cwd':str(ROOT),'output':c['original_output'],'approval_record_required_at':c['approval_path'],
  'old_source':'runs/stage12-state-cycle62p19/state.json','old_source_sha256':'78d049bed88a8c62f591f646d6f080b8c61524eae1440705188405631240e5c8',
  'new_source':c['source'],'source_pins':c['source_pins'],'EXE':c['original_EXE'],'EXE_sha256':c['original_EXE_sha256'],
  'requested_changes':{'replace_snapshot_with_cycle61a':True,'allow_emulator_guest_thread_scheduling':True,'raise_shared_A_caps':True,'one_advance_7_to_8_no_wrap':True,'unknown_before_execute_freeze':True},
  'shared_A_caps':c['proposed_shared_caps'],'old_A_caps':c['approved_A_caps_unchanged'],'next_round':2,'remaining_rounds_before':9,
  'failure_consumes_round':True,'B_or_C_started':False,'original_execution_this_turn':0,
  'stop_before_unknown_non_IAT_target_or_instruction':True,'stop_before_new_city_loader_or_selection':[hex(v)for v in (0x534a30,0x401cd0)],
  'saved_watch':{'phase':7,'entries':3,'advances':2,'tails':0,'pending_frames':2},
  'input_domain':c['input']['existing_domain'],'reset_existing_input_or_watch':False,
  'soft_role_freeze_seconds':50,'hard_role_process_group_seconds':60,
  'hard_kill_cannot_guarantee_kill_time_complete_snapshot':True,'complete_soft_cap_snapshot_verified':True,
  'performance':{'actual_final_guard_engine_own_steps_before45s':s['budget']['admitted_steps'],'own_writes':s['budget']['writes'],'process_seconds':p['elapsed_seconds'],'status':r['status'],'full_786432_pixel_loop_completed':False,
   'measurement_before_adding_city_loader_stop_guard':True,'previous_simple_12_7sec_result_does_not_bound_this_executor':True},
  'approval_record_schema':{'status':'AUTHORIZED_BY_USER','source_thread_id':'01a10af1-425e-75ae-bded-bf47f843a029','user_message_id':'actual future user approval ID required','user_message_time_UTC':'actual future approval time required','user_verbatim':'actual explicit approval reply required','approved_changes':'exact requested_changes object','configuration_sha256':'exact above hash','runtime_pins_sha256':'exact above hash','source_pins':'exact above source_pins','shared_A_caps':'exact above shared_A_caps','output':'exact above output','remaining_rounds_before':9,'next_round':2}}
 put(N/'approval-request.json',packet)
 pre=subprocess.run(['/usr/bin/python3','-B',str(N/'tools/probe.py'),'--preflight'],cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=10);assert pre.returncode==0
 put(N/'evidence/preflight.json',json.loads(pre.stdout));assert not json.loads(pre.stdout)['engine_imported']
 # Kernel denies an exclusive create outside the selected own output. No old file is opened for write.
 out=ROOT/'stage83/runs/phase7-probe-own-natural-v7';sentinel=ROOT/'stage83/phase7-probe-forbidden-new-file'
 assert not sentinel.exists()
 cmd=['/usr/bin/sandbox-exec','-f',str(ROOT/'stage83/repair-v2/configs/role-write-policy.sb'),'-D','OUT='+str(out),'/usr/bin/python3','-B','-c',
  "import os;os.close(os.open("+repr(str(sentinel))+",os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600))"]
 q=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5);assert q.returncode and not sentinel.exists()
 put(N/'evidence/kernel-write-gate.json',{'status':'OUTSIDE_SELECTED_OUTPUT_EXCLUSIVE_CREATE_DENIED','command':cmd,'returncode':q.returncode,'forbidden_file_created':False,'old_files_opened_for_write':False})
 (N/'evidence/kernel-write-gate.stderr').write_bytes(q.stderr)
 put(N/'evidence/failure-history.json',{'own_v2_test_failure':'write-cap fixture mislabeled its store as natural CALL; fixed fixture goal, old frozen failure retained',
  'own_v4_test_report_failure':'all behavioral cases passed; exclusive report output already existed; v4 results audited without re-execution',
  'own_full_performance_v1_failure':'self ROL not in conservative decoder; stopped before instruction; self workload changed to supported SHL, no original opcode support relaxed',
  'own_full_performance_v2_v3':'45-second cap reached; successful diagnostic freeze, not full-loop success',
  'all_failed_directories_retained':True,'original_instructions':0})
 report='''已实现可执行的有界原探测控制器；本轮只有自造执行和原状态只读恢复，原入口仍须新的合并批准。

接口：/usr/bin/python3 -B stage83/phase7-probe-v1/tools/probe.py --preflight
原命令的绝对路径、cwd、输入及哈希、写目录、配置/工具pins哈希全部见approval-request.json。--run没有批准在任何engine import、原round claim之前拒绝；旧8sec批准同样拒绝。本轮没有创建AUTHORIZED_BY_USER记录。最近“继续”延续代码整理，没有被当作三项新原授权。

执行器沿用固定Unicorn2.1.4、真实native Clock/Event/无窗口Surface及未改的ThreadKernel方法。初始last=1、wake-before-pick、worker_first=true、prefer=None、quantum100000；完整恢复cycle61a的四CPU context、FS/TEB、每线程TLS/error/events及HostStore全部178字段。tid1/2/4可自然执行，tid3自然ready即停止，不改成假waiting。C CLOCK_MONOTONIC与保存deadline同域，源tid2已过期会自然唤醒，不能固定让4先跑。真实scheduler入口显式绑定4ac2b0，保存Watch entries3/advances2/tails0及两pending frame不清零。

写入预留：通用保守i386 decoder在code hook前算单store的EA/width、全部读写权限和CALL栈位置，先检查共享instruction/write/wall，再预留；实际hook必须与计划一致。push、直接/已知IAT CALL、支持的单memory-destination是统一规划。平台输出整组预留，HeapReAlloc的fill+preserved copy一起计入，超额时堆地址/metadata/bytes均不变化。批量平台trace先准备，再检查时间，之后checked bulk memory提交与规范有序trace保存。未知REP/multi-store、prefix、x87/SIMD/未支持opcode、divide fault语义、非IAT间接目标，在该指令执行前冻结诊断。不是所有原指令的支持声明；未证明形式不偷偷解释。

未知CALL保存live ESI/ECX object、EDX table、target/slot、SP和stack以及完整CPU/内存。自造例中初始NULL槽被真正guest store变为对象，随后CALL[EDX]在执行前拒绝，callee marker未写、CALL返回地址未入栈，证明没有假定NULL恒定。原5fc139的具体注册/ABI仍未证明，探测可停在更早未知处。

自然资格仅限tid1确实从4ac338的e8732b0000 CALL进入4aeeb0，SP和4ac33d实际返回匹配，Watch真正7→8且只多一次advance。phase写入预判禁止9、回退和wrap；4ac4e2/4ac650、534a30新城市加载、401cd0新存档选择均在执行前拒绝。不设置原PC/SP、gate、队列、phase或输入；不恢复整城、不处理窗口/音视频/网络；已有controlled Device61精确恢复，down/pending共享latch/log/hash链保留。

15项最终自造行为测试通过，包含自然捕获、NULL→对象未知callback、REP、未知API、写cap、无效地址、第二次phase推进、wrap、HeapReAlloc整体预留负例、tid3、改code、instruction cap、guest/role软超时以及硬kill。正例也验证Interlocked、HeapAlloc、TLS、原日志prefix和计数递增。所有负例都是完整诊断且不被当成phase8成功，硬kill例例外：只保留先前完整checkpoint，不伪称终止瞬间已冻结。

原只读冻结实际297,521,152 bytes、四官方CPU payload、178 host字段通过：176字段binary hash完全同原；visited只是set重编码而typed value相同，guest_threads20只是官方context序列化header重建而四5568-byte payload与全部其它metadata相同；原全memory SHA相同。耗时约2.61sec（包含子进程）；emu_start=0。完整同步不丢源大日志或opaque未用字段。Snapshot独立format标注qualified/diagnostic与kind，不能直接当旧ABC接口兼容已完成。

重要实际性能限制：生产执行器自造786432-pixel/19-instruction/4-byte-store循环在45sec guest触发时间界并正常保存退出。准确admitted steps、writes和whole-role耗时见approval-request.json的performance与原输出full-performance-v3；该测量在增加城市loader stop guard之前，planner、写追踪和全部核心guard相同。比前版简化样例12.725sec慢得多，不能用简化结果声称本执行器能在45sec跑完全幅前缀。保持20M/7M/45s/60s作为未授权诊断上限，不再增加预算；自然成功不保证，可能很早就停止。

时间规则：guest最长45s，若准备已用更多时间则自动缩短剩余guest，使role50s开始保存，留10s至hard60；外部每个角色独立process group硬终止。软时间界/未知边界已验证完整冻结；硬SIGKILL或崩溃时不能保证终止瞬间完整状态，明确记录incomplete，校验并保留较早完整checkpoint和所有claim/log。若因此续存下一段，仍需新round/明确范围，live clock继续推进，不能声称与不中断原轨迹一致，不得免费重试。

合并审批材料仅申请一次第2轮A探测的三项变化：替换旧cycle62p19为固定cycle61a；允许模拟器内1/2/4自然调度；A共享预算由6M/2M/8s改20M/7M/45s。每role60s、原EXE/hash、固定引擎、设备domain、保护文件、未知即停仍保留。失败也消耗整轮，B/C不启动；本执行器从不自动开启后续轮或执行SR游戏函数。新capture即使自然达标，B/C链格式适配及真实phase8差分仍是后续工作，不能据此宣布SR native逻辑通过。

原始ledger仍20used/80remaining/closed，原batch仅round01一claim，9轮剩余不变；6960旧pin复核通过。没有安装/系统设置/上传/提交/发布；原指令、参考切片、SR游戏函数新增均0。成功Stage73六函数728bytes、214静态instruction、234942真实动态步/49889写仅原输入域结论，未泛化。
'''
 (N/'reports/README.txt').write_text(report)
 request_text='''是否批准一次第2轮有界原探测，同时批准这三项变化：
1. 将固定输入从cycle62p19替换为已冻结的cycle61a（phase7，保留Watch 3entries/2advances/0tails、两pending frame及原controlled keyboard domain）；
2. 允许固定Unicorn2.1.4内tid1/2/4按既有有限调度器自然运行，完整保留四线程；tid3自然ready即停；
3. A的全线程共享上限改为20M指令、7M规范写、最多45秒guest，整个role仍hard60，role50秒开始冻结。
只取最多一次phase7→8、无wrap的自然CALL前状态；未知callback/opcode/API或达到界限即停止保存，不运行未知目标。原回调对象可能改变，不假定当前NULL恒定。实际生产自造完整负载45秒未完成，所以不保证取样成功；失败也消耗1整轮，B/C不启动，不自动重试。软停止可完整冻结，硬kill不能保证终止瞬间完整snapshot，必须如实失败并保留已有证据。原20used/80remaining账本保持closed，新批次批准前仍1used/9remaining。没有窗口/音视频/全局键盘/原DLL/安装/上传/发布。
具体来源、所有哈希、固定命令、输出写门与停止条件见同目录approval-request.json；这是请求材料，不是授权记录。
'''
 (N/'reports/combined-approval-request.txt').write_text(request_text)
 verify(base['hashes']);check_old()
 put(N/'evidence/postflight.json',{'status':'PROTECTION_AND_CLOSED_ACCOUNTING_PASS','protected_pins':base['count'],'all_unchanged':True,'old_ledger_sha256':LEDGER_SHA,'old_ledger':'20 used / 80 remaining / closed','rounds':accounting(),'original_instructions':0,'original_reference':0,'SR_game_calls':0,'new_authorization_written':False})
 deliver={str(p.relative_to(ROOT)):sha(p)for p in N.rglob('*')if p.is_file()and p.name!='manifest.json'}
 for path in(ROOT/'stage83/runs').glob('phase7-probe-own-*'):
  for file in path.rglob('*'):
   if file.is_file():deliver[str(file.relative_to(ROOT))]=sha(file)
 put(N/'manifest.json',{'status':'EXECUTABLE_PROBE_SEALED_PENDING_THREE_CHANGE_APPROVAL','files':len(deliver),'hashes':deliver});verify(deliver)
 print(json.dumps({'status':'EXECUTABLE_PROBE_DELIVERED_ORIGINAL_GATE_CLOSED','configuration_sha256':packet['configuration_sha256'],'runtime_pins_sha256':packet['runtime_pins_sha256'],'manifest_sha256':sha(N/'manifest.json'),'protected_pins':base['count'],'files':len(deliver),'implemented_import_endpoints':len(supported),'import_labels_restored':len(imports),'performance':packet['performance'],'remaining_rounds':9}))

if __name__=='__main__':main()
