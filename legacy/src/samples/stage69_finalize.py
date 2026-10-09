# -*- coding: utf-8 -*-
"""Static-only handoff with unchanged protected artifacts and falsifiable scope."""
import ast,json
from pathlib import Path
from stage54_batch import LEDGER,APPROVAL,sha,validate
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence';R=ROOT/'runs/stage69-static'
EXPECTED='4e499d8560cc4c7a9bc4a2e725e27f502fa13979de6366e9f66bd06f237edcc9'
def main():
    assert sha(LEDGER)==EXPECTED
    l=json.loads(LEDGER.read_text());validate(l);assert l['goal_closed'] and len(l['executions'])==20 and l['remaining_processes']==80
    prior=json.loads((E/'stage64-postflight.json').read_text())
    for row in prior['checks']:assert sha(row['path'])==row['expected_sha256'],row['path']
    for path,h in l['pinned_files'].items():assert sha(path)==h,path
    for row in l['executions'][18]['checkpoint_files']:assert sha(ROOT/row['path'])==row['sha256']
    closure=json.loads((E/'stage69-static-closure.json').read_text());states=json.loads((E/'stage69-saved-state-diff.json').read_text())
    readiness=json.loads((E/'stage69-existing-decoder-readiness.json').read_text());stack=json.loads((E/'stage69-continuation-stack-contract.json').read_text())
    assert not closure['unknown'] and closure['function_count']==6 and closure['instruction_count']==214 and closure['code_bytes']==728
    assert not readiness['existing_full_pipeline_conversion_ready'] and readiness['recovered_instruction_count']==15
    assert all(all(p['success'] for p in q['pointer_chain']['pointers']) for q in states['states'])
    writes=[dict(name='two label tables',address='0xeb8f00 and 0xec5a10',bytes=103968),
        dict(name='five work scalars plus 1000-entry ring',address='0xc81b70..0xc82b23',bytes=4020),
        dict(name='ten id/size pairs',address='0x1312588..0x13125d7',bytes=80),
        dict(name='scan cursor',address='0x101c560',bytes=4)]
    # End inclusive of 4020 bytes beginning c81b70 is c82b23.
    assert 0xc81b70+4020-1==0xc82b23
    plan=dict(status='PROPOSED_NOT_EXECUTION_APPROVAL',name='Actual phase7 grid marking continuation',
        source_sha256=l['source_EXE_sha256'],historical_checkpoint='cycle61a',entry_PC='0x4af311',entry_SP_from_verified_stack='0x700ffd68',
        end_PC_before_instruction='0x4ac333',end_SP='0x700ffd8c',phase_remains=7,
        original_root='0x4af350',root_call_site='0x4ac32e',functions=[q['entry'] for q in closure['functions']],
        code_bytes=728,instructions=214,memory_write_conservative_spans=writes,memory_write_span_bytes_excluding_stack=sum(q['bytes']for q in writes),
        read_only_pointer_chain='signed tile index -> [0x8c7638]+4*index -> 32-bit object address -> signed word at object+0x14',
        snapshot_indices_resolved=42,static_index_bounds_proven=False,heap_writes=False,host_callbacks=False,
        imports=False,x87=False,SEH=False,rep_ES_base_and_DF_contract_unverified=True,
        CPU_context='Extract main tid1 opaque5608-byte context already saved, not worker tid2/current context. Use verified engine-context schema/API only after approved test scope; do not guess offsets.',
        native_memory='Preserve uint32 guest addresses and region permissions; translate data access through guest memory base/region mapping. Do not cast guest object pointers to ordinary 64-bit host pointers or call initialize_cpu to erase original stack/flags.',
        proposed_steps=[
            'Close the nine unsupported decode forms and CFG/flag summaries; produce explicit six-function native lift with no calls outside the byte-backed list.',
            'Obtain verified tid1 general registers, EFLAGS/DF and segment ES base from its saved opaque context; check PC/SP against saved stack. Preserve x87 state unchanged.',
            'In separately authorized headless fixed-EXE reference scope, restore historical guest regions and main tid1 only, run from4af311 until first4ac333 atSP700ffd8c. Stop before dispatcher increment; unexpected code target/import/fault/timeout fails.',
            'Run native continuation from identical guest bytes and CPU state with same stop contract, record all writes, and compare every mapped guest byte plus return CPU/flag contract to isolated original reference.',
            'Compare resulting labels and top10 against saved cycle61p07 only as a secondary historical consistency check; phase8 and refresh prevent using it as the exact oracle.',
            'Serialize native guest state midway, resume in a fresh native process, and require the same final bytes and CPU state. Exercise existing actual and generated adversarial inputs only with approved original reference, including ring wrap, label byte wrap, DF/REP continuation and negative boundary indices.'
        ],
        pass_requirements=['Non-empty original/native equal write log and exact output bytes; preserve all other guest bytes.',
            'Exact PC/SP and all defined/live return CPU flags, preserved callee GPRs, segments and x87 unchanged; undefined flags require documented masking based on proven producer semantics, not a blanket mask.',
            'Every read obeys original guest width/sign and every address obeys uint32 arithmetic and mapping; unexpected pointer/index/fault never receives a fabricated success.',
            'Cold native continuation after state persistence equals uninterrupted native and isolated original reference; no helper/UI required.'],
        remaining_blockers=['Current closed batch requires latest_checkpoint cycle62p19; historical cycle61a replay is outside that continuation rule. Do not silently reopen it, roll back progress or claim old snapshot as new city progress.',
            'The historical comparison needs explicitly authorized separate reference scope/accounting before any original instruction execution; current task authorizes static analysis only.',
            'Full native translation, REP/width/flags support, CPU-context adapter and fault-faithful guest memory are not implemented or validated in this round.',
            'No isolated original return checkpoint/write trace exists yet; later saved state includes phase8/refresh and aliases can write the same memory.',
            'Snapshot42 resolved indices do not prove valid bounds for all city states; ring capacity1000, byte label truncation and signed index behavior need preservation.'],
        cost_estimate='Judgment, not benchmark: 3–5 focused engineering days for a maintainable decoder/CPU/memory lift plus approved isolated differential and persistence harness; stop/reassess after first one-day tranche if CPU-context or fault model does not close. Full city migration cannot be extrapolated from728bytes.',
        alternative='A manually reviewed instruction-level LLVM/C lift may avoid extending the old limited decoder, but still needs identical guest CPU/memory contracts and original differential proof.',
        today_original_instruction_execution=0,today_native_city_logic_execution=0,full_city_AOT_proven=False)
    (E/'stage69-falsifiable-native-city-slice-plan.json').write_text(json.dumps(plan,indent=2)+'\n')
    protected=dict(all_protected_passed=True,protected_hash_checks=len(prior['checks']),source_pins_checked=len(l['pinned_files']),
        ledger_sha256=sha(LEDGER),approval_sha256=sha(APPROVAL),used=20,remaining=80,closed=True,latest_checkpoint='cycle62p19',
        checkpoint_unchanged=True,no_run21=True,no_original_EXE_or_DLL_or_Unicorn_execution=True,no_old_Swift_read=True,
        UI_environment_not_retested_or_fixed=True,no_permissions_or_global_settings_changed=True)
    (E/'stage69-postflight.json').write_text(json.dumps(protected,indent=2)+'\n')
    result=dict(status='bounded_static_city_slice_identified_NATIVE_NOT_YET_PROVEN',candidate='phase7 0x4af350 grid marking and rank update',
        shortest_real_saved_continuation='cycle61a tid1 PC4af311/SP700ffd68 -> first4ac333/SP700ffd8c',
        code_bytes=728,instructions=214,functions=6,static_direct_call_sites=5,static_indirect_calls=0,static_indirect_jumps=0,
        imported_APIs=0,host_callbacks=0,x87=0,SEH=0,heap_reads=True,heap_writes=False,
        static_CFG_closed=True,automatic_instruction_support='205/214=95.794%',automatic_byte_support='690/728=94.780%',
        actual_existing_full_pipeline='15/214=7.009%, stops at REP, conversion_ready=false',
        unresolved_static_code_calls=0,unresolved_execution_contracts=['guest32 pointer semantics','tid1 opaque CPU conversion','ES/DF/REP','width/flags','fault behavior','isolated oracle','cold persistence'],
        snapshot_pointer_indices_resolved=42,static_pointer_bounds_proven=False,actual_label_diff='one byte in each51984-byte table: offset32044 0->3',
        actual_top10_diff='0x1312588 offsets16,20: pair(0,0)->(3,1)',exact_existing_single_phase_oracle=False,
        first_analysis_wrong_top10_literal_preserved=True,corrected_top10='0x1312588; initial0x1302588 artifacts superseded',
        no_new_city_AOT_code_generated_or_executed=True,original_instructions_executed=0,no_Unicorn_import_or_run=True,
        no_original_run21=True,ledger_unchanged=True,used=20,remaining=80,closed=True,
        recommendation='Worth the isolated phase7 experiment. Do not resume platform API accumulation or present decode coverage as native execution.',
        cost_estimate_judgment=plan['cost_estimate'])
    (E/'stage69-result.json').write_text(json.dumps(result,indent=2)+'\n')
    own=['stage69_dump.py','stage69_static.py','stage69_decoder_pipeline.py','stage69_finalize.py']
    for name in own:ast.parse((ROOT/'samples'/name).read_text())
    (E/'stage69-own-artifacts.json').write_text(json.dumps(dict(sources={str(ROOT/'samples'/n):sha(ROOT/'samples'/n)for n in own},
        disassembly_sha256=sha(R/'fixed-text.disasm'),function_windows={q['entry']:q['sha256']for q in closure['functions']},
        installed_Apple_LLVM_only=True,no_new_tool_installation_or_execution_source=True),indent=2)+'\n')
    report='''最短有实际存档依据的城市更新切片已找到：阶段7的网格标记/区域大小排序函数0x4af350，选择cycle61a保存的主线程PC0x4af311继续到首次0x4ac333（阶段子函数返回，尚未执行dispatcher收尾）。这是实际城市网格更新，不是早期税率leaf；本轮只做静态和存档字节分析，没有原EXE/Unicorn/DLL或生成后的游戏逻辑执行，完整原生等价尚未通过。屏幕问题保持未解决，不继续UI迁移。

固定EmperorEN.exe SHA8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753；仅用已安装Apple llvm-objdump和任务自写stdlib分析工具。原阶段5→22观察与固定原跳表逐项对应，阶段7 dispatch为0x4ac32e→call0x4af350。阶段14、15、19直达计数收尾，明确未选用。只读原字节/本地既有存档与任务分析代码，未读旧Swift实现、增加工具来源或运行第三方构建脚本。

| 六函数闭包 | 字节 | 指令 | 作用（根据字节推断，未认定具体城市子系统名称） |
|---|---:|---:|---|
| 0x4af350 |315|95|扫描当前112×112网格，清零两张51984字节表、标记满足谓词的四邻域区域、按大小维护十组id/size |
| 0x4af310 |59|18|由tile的signed16索引走容器→32位对象指针→对象+0x14字段，调用type谓词，最终判断tile_flags&0x44==0x40 |
| 0x4af490 |312|86|四邻域遍历、写两张byte标记表、1000项环队列及计数；无调用 |
| 0x47f1b0 |15|4|根据this容器和signed索引算32位slot地址，ret4 |
| 0x4f8210 |4|2|读取容器+4中的32位begin指针 |
| 0x543d90 |23|9|比较类型59/60返回布尔；结果被caller覆盖仍保留调用及CPU效果 |

合计728字节/214指令/5处直接call。所有相对分支终点逐字节匹配固定EXE，每个callee的全部分支与ret均在审查窗口内，静态代码调用闭包无未知目标、无间接call/jump、Win32导入、host回调、x87或FS/SEH路径。它不依赖图形/音频API。此前口头5函数不含4字节容器访问callee，完整计数是6。这里只声称这些已核实代码路径的静态闭合；动态42个已解析对象不作为任何vtable/间接调用全闭包证据。

自动比例有两个不同量度：既有自写decoder逐条205/214=95.794%、690/728=94.780%可识别；9条缺口为3处REP STOSD、2处MOVSX word、1处0x66的16位读、3处TEST编码。实际原自动Analysis流程从root运行静态扫描只得到15/214=7.009%，在0x4af380的首个REP停住，flags unknown successor/no closed return，conversion_ready=false。本轮不加规则、不生成转换代码；高单条覆盖率不能当作自动重编译成功率。LLVM解码与完整CFG验证已通过，需要审查的功能边界仍是人工确认的原字节窗口。

状态边界：输出为0xec5a10和0xeb8f00各51984字节、0xc81b70..0xc82b23的5个scalar/1000项ring、0x1312588..0x13125d7的10组id/size、0x101c560扫描cursor；总保守输出span108072字节另加guest栈。输入包括tile flags32、tile object indices16、neighbor flags16、固定四邻域delta[-228,+1,+228,-1]和网格参数[112,112,13282,116]。这些只是从代码/存档识别的字段用途，尚未恢复原字段名称。全表42种signed tile index在三份真实存档中均可解析至已记录heap分配：容器begin0x100028e8、end0x10006768（4000 slots/16000bytes），对象字段读取在分配范围内。闭包不写heap、不分配/释放；仍需原样读取slot/object链，不能省略unused type call而改变fault或CPU结果。42项解析成功不证明任意城市索引、负索引或边界均有效。

真实差分：frame61a（阶段6，refresh中）两表均有89个非零byte，前三个id/size为(1,87),(2,1),(3,1)。cycle61a（阶段7中途）两表88个非零byte、第三组清成(0,0)。cycle61p07（阶段9，经过阶段7、8与refresh）两表恢复89个；每张表仅offset32044从0变3，前十表offset16/20补成(3,1)。这些显示真实持久状态更新，但p07不是准确阶段返回oracle，不能把差分归因于单callee或用它通过全内存/CPU等价。全.text literal xrefs还发现0x4b0950附近有另一reset入口，且别名/间接写全集未分析；既有晚存档只作辅助对照。

初次自写分析把前十表地址写成0x1302588，核对原字节立即修正为0x1312588；错误初稿两个JSON保留为*-initial-top10-address-error.json，v2结果和本报告使用正确地址。没有修改原存档、原源或批准运行证据。

精确继续点：cycle61a当前top-level context属于tid2，顶层EIP是工作线程API thunk，不能用作主线程。主线程tid1完整5608字节context已无损提取到runs/stage69-static/cycle61a-main-thread-context.bin；原轨迹两次记录PC0x4af311，已经执行predicate的push ESI。根据实际prologue与保存栈，SP0x700ffd68，saved ESI=0x70；0x700ffd6c=0x4af3cc，参数tile index=0x6c44（27716）；0x700ffd88=0x4ac333，0x700ffd8c=0x5371c4。候选结束应在执行0x4ac333之前、SP0x700ffd8c，phase仍7。本轮未猜测Unicorn内部寄存器offset；native adapter还需用已核实schema/API取得tid1 GPR/EFLAGS/ES基址，检查DF与REP宽度。不得拿native_cpu.h的initialize_cpu清空真实栈、flags和guest地址。

可证伪验证方案：先修复或绕开现有有限decoder的9种指令缺口，明确生成上述六函数的指令级LLVM/C lift及uint32 guest-memory访问。随后在明确获准的独立无图形参考scope中从同一historical tid1 CPU/全region内存运行固定原字节到4ac333；PC/SP、所有定义/活跃flags和保存GPR、ES/其它segment、未触碰x87状态必须符合契约。原参考与native均只跑主线程这一个切片，遇越界、fault、未列call目标/导入或时间限即失败，不能填返回或改规则。对比每个mapped guest byte及实际写日志，要求非空且严格一致；p07两格/第三组结果仅为辅助检查。运行中native保存完整CPU/guest state，在另一个native进程恢复并继续，必须和不暂停及原参考结果一致。进一步输入需覆盖ring wrap1000、byte label截断、signed index、DF/REP恢复及边界mask，原算法每31左右节点变化分组、环队列覆盖和51984迭代上限不能改成自认为更合理的flood fill。

具体未闭合点为tid1 opaque context→native CPU的可靠适配、guest32指针与错误内存行为、REP的ES/DF和partial-state、16bit宽度/符号/flags、独立精确原返回oracle、跨进程native state。没有平台API闭包，但这些CPU/内存契约会主导验证成本。判断性估计：维护性实现与上述对照/恢复harness约3–5个专注工程日，第一日若context/fault模型无法闭合就交回重估；此估计不是实测工时，不能由728字节外推整个城市或游戏。可考虑直接审查指令级lift代替扩展旧分析器，仍须相同oracle。

授权与预算：本轮只读授权不包含任何新原参考执行。当前100次批次CLOSED、20used/80remaining且只允许最新cycle62p19续跑；historical cycle61a实验不是当前进度回滚，不能静默改latest/reopen/消耗或重置原预算。若下一轮要取得原oracle，需明确允许历史检查点的独立范围与计数，然后才执行；本方案JSON明确标记PROPOSED_NOT_EXECUTION_APPROVAL。本轮未启动21、未安装/全局设置/权限改变、提交发布上传或旧Swift读取。

保护复核3482hash、550 source pins及cycle62p19三文件通过；账本SHA4e499d8560cc4c7a9bc4a2e725e27f502fa13979de6366e9f66bd06f237edcc9保持不变。自写分析实际18.724秒、修正后18.961秒、pipeline4.876秒，均hard60秒内。结论：值得这个明确的小范围城市网格AOT实验，现有pipeline不能直接完成；完整城市AOT仍未证明，不再逐API堆积UI层。

核心交付：stage69-result.json、stage69-static-closure.json、stage69-saved-state-diff.json、stage69-existing-decoder-readiness.json、stage69-continuation-stack-contract.json、stage69-falsifiable-native-city-slice-plan.json、stage69-postflight.json；runs/stage69-static有固定原反汇编、六函数窗口与三份opaque main thread CPU。分析源码stage69_dump.py、stage69_static.py、stage69_decoder_pipeline.py。
'''
    (ROOT/'reports/stage69-bounded-city-grid-AOT-closure.md').write_text(report)
    print(json.dumps({k:result[k]for k in ['status','code_bytes','instructions','functions','actual_existing_full_pipeline','exact_existing_single_phase_oracle','used','remaining','no_original_run21']}),flush=True)
if __name__=='__main__':main()
