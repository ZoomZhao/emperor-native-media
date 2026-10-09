"""Static/build-only audit and explicit next differential proposal."""
import hashlib,json,re,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb')as f:
        for b in iter(lambda:f.read(1048576),b''):h.update(b)
    return h.hexdigest()
def main():
    ledger=E/'stage55-batch-ledger.json';assert sha(ledger)=='4e499d8560cc4c7a9bc4a2e725e27f502fa13979de6366e9f66bd06f237edcc9'
    l=json.loads(ledger.read_text());assert l['goal_closed'] and len(l['executions'])==20 and l['remaining_processes']==80
    protected=json.loads((E/'stage64-postflight.json').read_text())
    for row in protected['checks']:assert sha(row['path'])==row['expected_sha256'],row['path']
    for p,h in l['pinned_files'].items():assert sha(p)==h,p
    for row in l['executions'][18]['checkpoint_files']:assert sha(ROOT/row['path'])==row['sha256']
    closure=json.loads((E/'stage69-static-closure.json').read_text());actual=json.loads((ROOT/'runs/stage70-city/analysis.json').read_text())
    decoded={q['pc']:q for q in actual['instructions']};count=0
    for f in closure['functions']:
        for line in (ROOT/'runs/stage69-static'/(f['entry'][2:]+'.asm')).read_text().splitlines():
            m=re.match(r'^\s*([0-9a-f]+):\s*((?:[0-9a-f]{2}\s+)+)\s*([a-z][a-z0-9]*)\s*(.*)$',line)
            if not m:continue
            pc=int(m[1],16);b=bytes.fromhex(m[2]);assert pc in decoded and decoded[pc]['end']==pc+len(b),hex(pc);count+=1
    assert count==214
    build=json.loads((E/'stage70-build-result.json').read_text());inputs=json.loads((E/'stage70-real-input-contract.json').read_text())
    tests=json.loads((E/'stage70-self-byte-tests.json').read_text())
    plan=dict(status='PROPOSAL_ONLY_NO_EXECUTION_APPROVAL',scope='One real phase7 write-state continuation, no UI or other modules',
        entry=inputs['native_entry'],stop=inputs['native_stop'],checkpoint='cycle61a tid1, not current tid2',
        accounting='Closed20/80 ledger remains unchanged. Historical checkpoint reference requires a separate explicit scope/accounting; never reopen old ledger or create run21 silently.',
        prerequisites=['Read saved opaque tid1 CPU through verified Unicorn2.1.4 context API/schema after scoped approval; check observed entry PC/SP and flat DS/SS/ES and DF, carry original x87/segment state unchanged.',
            'Implement fresh-process input loader from immutable full297521152-byte historical guest mapping and original permissions; copy-on-write native buffers retain uint32 addresses. Do not substitute only extracted spans.',
            'Add ordered write trace and defined/live-flag comparison; current runtime counts stores only. Validate contiguous cross-region and partial memory-fault behavior before claiming fault fidelity.',
            'Finish native state serialization including CPU, guest memory, pending REP PC/ECX/EDI, budget and mapping; current helper pause/resume test stays in one process.'],
        exact_reference='One future authorized headless original-i386 process: restore only saved main tid1 with all guest memory, run unchanged fixed bytes from4af311 to first4ac333/SP700ffd8c, stop before caller instruction. Any target outside214-address closure, import, fault, timeout or worker scheduling fails.',
        exact_native='One separately authorized generated-arm64 process with identical input and stop contract. Compare nonempty ordered stores, every mapped guest byte, PC/SP, all GPRs, defined/live flags, segments and preserved opaque x87. Undefined AF is producer-specific, never blanket-masked.',
        persistence='After baseline passes, two additional separately authorized native processes: pause inside REP after an element commit, save complete state, exit; fresh process restore and finish. Require same final mapped bytes, write trace and CPU as uninterrupted baseline and exact i386 reference.',
        proposed_budget=dict(original_reference_processes_max=1,native_processes_max=3,guest_wall_seconds_max_each=8,guest_instruction_or_REP_element_steps_max_each=5000000,hard_process_seconds_each=60,unexpected_fault_retries=0),
        budget_is_proposal_not_new_authorization=True,late_snapshot_exact_oracle=False,
        secondary_check='cycle61p07 label/top10 diff only; phase8 and refresh occurred, so it cannot be exact expected output.',
        stop_conditions=['CPU layout/segments cannot be verified','Execution leaves known closure','No nonempty equal write-state result','Any mapped byte or live CPU mismatch','Persistence resume mismatch'],
        no_current_original_or_native_city_execution=True)
    (E/'stage70-next-differential-plan.json').write_text(json.dumps(plan,indent=2)+'\n')
    post=dict(all_passed=True,protected_hash_checks=len(protected['checks']),source_pins_checked=len(l['pinned_files']),original_instruction_windows_sha_unchanged=True,
        repaired_instruction_lengths_match_independent_LLVM=count,ledger_sha256=sha(ledger),used=20,remaining=80,closed=True,latest_checkpoint='cycle62p19',no_run21=True,
        original_EXE_Unicorn_original_DLL_or_generated_city_execution=False,generated_city_dylib_loaded=False,installed_global_dependencies=False,modified_SR_vendor=False)
    (E/'stage70-postflight.json').write_text(json.dumps(post,indent=2)+'\n')
    out=dict(status='REAL_CITY_SLICE_BUILD_ONLY_BEHAVIOR_UNPROVEN',code_bytes=728,instructions=214,closed_functions=6,
        original_code_byte_edits=0,old_protected_source_edits=0,generic_rep_width_flags_fix=True,address_specific_instruction_replacements=False,
        instruction_lengths_independent_LLVM_matched=214,SR_synthetic_REP_to_arm64_object_build=True,
        real_city_lift_route='Own generic decoder -> instruction-specific C -> Apple clang arm64; real city was not passed through SRW/llasm.',
        real_city_native_artifact=build['native_artifact'],native_sha256=build['sha256'],native_city_executed=False,original_or_Unicorn_executed=False,
        runtime_self_cases=30,self_decoder_tests=tests,real_input_spans=len(inputs['extracted_spans']),resolved_tile_indices=inputs['pointer_indices'],unique_heap_objects=inputs['unique_allocated_objects'],
        first_failures=['Initial own condition switch used case0 labels; fixed to case 0 and added all16 condition tests. Initial14-case pass did not validate branches.',
            'SR synthetic llasm returned78 on empty relocation segment. Synthetic format normalizer removes label-only empty segment; vendor SR source unchanged. Builds v1–v3 failed, v4/v5 passed.'],
        SR_rep_partial_state_limit='Existing x86_rep_stosd keeps local count/destination and writes ECX/EDI at helper end, not per-element. Own checked helper commits after each successful store; exact original differential still pending.',
        remaining=['Verified tid1 opaque CPU-context adapter','Original flat DS/SS/ES and DF confirmation','Memory cross-region/fault equivalence','Ordered actual write traces','Exact isolated original return oracle','Fresh-process native state persistence'],
        recommendation='Continue only with a separately scoped one-reference-process differential after CPU/input/trace adapters close; stop on first unexplained mismatch. No whole-city or playable-game claim.',
        cost='Observed this tranche: about20 minutes of generic decode/runtime/build and actual input preparation; builds under1s and selftest about1.6s, input extraction5.376s. Remaining engineering duration not measured, no fixed3–5-day estimate.',postflight=post)
    (E/'stage70-result.json').write_text(json.dumps(out,indent=2)+'\n')
    report='''# Stage70：真实城建写状态切片已构建，行为尚未证明

固定728字节、六函数214条原指令保持不变。新建通用分析器修复REP STOS、16位宽度、MOVSX/TEST、高8位寄存器和flags处理；完整CFG从旧15条恢复到214条，所有指令长度与已有独立LLVM反汇编一致。真实切片通过自写通用解码→逐指令C→Apple clang生成arm64动态库，未加载、未执行。它不是“真实游戏已通过SR链”的结果。

SR本身支持REP；此前首个阻塞是自写解码/CFG。另用纯自造字节确认SRW→llasm→LLVM→Apple clang能构建arm64对象，包括REP STOSB/W/D、16位MOV/TEST和MOVSX。初次llasm因仅有label的空重定位段返回78，样例格式正规化后通过，未改vendor。SR现有REP helper只在循环结束回写ECX/EDI，中途fault/保存的精确CPU状态仍不成立。自写检查型helper逐元素提交，30项自造运行测试覆盖正反DF、宽度、故障、预算暂停、部分寄存器及16种分支；这些不证明原游戏等价。

真实cycle61a输入已只读提取：原始guest region/permissions、完整指针vector、42个索引涉及的完整allocated objects、整段主线程stack及5608字节opaque tid1 CPU。全量历史memory/state仍以固定哈希不可变引用保留。当前顶层是worker tid2，不能拿其寄存器代替tid1；没有猜测opaque CPU偏移。入口4af311/SP700ffd68，必须停在4ac333/SP700ffd8c且不执行caller指令、不推进phase7。晚期cycle61p07包含phase8/refresh，仅作次级一致性证据，不能充当精确oracle。

下一次差分前还需CPU上下文适配、全量guest映射loader、有序写trace和跨进程保存；当前runtime只计store次数，跨相邻region的访问和部分fault细节未验证。提案最多一原i386参考进程、三native进程（baseline、暂停保存、冷恢复），每个guest8秒/500万虚拟步、进程硬60秒，异常不自动重试。历史checkpoint超出已关闭batch的latest-continuation规则，需单独明确范围和记账；本轮未执行EXE/Unicorn/新native city，旧账本仍20已用/80剩余且关闭，未启动21。

本轮实际约20分钟完成通用修复、构建和输入准备；单次构建不到1秒、自造测试约1.6秒、真实输入提取5.376秒。尚未完成部分没有实测工期，不沿用预设3–5天。值得继续一个有界、精确差分切片；通过前不外推整城或可玩游戏。

原错误日志保留：stage70-build-only-city-and-SR-self[及-v2/-v3]、stage70-own-byte-and-runtime-tests。后续-v4/-v5构建通过，-v2/-v3语义测试通过。首版C条件函数把case0当普通label，后来修正并加入16条件测试；首版14项通过不能作为分支通过证据。

交付：evidence/stage70-result.json、stage70-build-result.json、stage70-real-input-contract.json、stage70-next-differential-plan.json、stage70-postflight.json；samples/stage70_analysis.py、stage70_runtime.[ch]、stage70_selftest.py、stage70_build.py、stage70_inputs.py；runs/stage70-city/libstage70_city_UNEXECUTED.dylib及runs/stage70-SR-self/self.o。受保护3482项哈希、550源码pins、最新checkpoint和原728字节均通过只读审计。无安装、系统设置改变、原DLL调用、上传、提交或发布。
'''
    (ROOT/'reports/stage70-build-only-real-city-slice.md').write_text(report)
    print(json.dumps(dict(status=out['status'],audit=post)))
if __name__=='__main__':main()
