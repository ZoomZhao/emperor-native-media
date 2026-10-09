"""Bounded checkpoint: static real-input contracts, own tests, unchanged audit."""
import hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
def sha(p):
    h=hashlib.sha256()
    with Path(p).open('rb')as f:
        for b in iter(lambda:f.read(1048576),b''):h.update(b)
    return h.hexdigest()
def main():
    ledger=E/'stage55-batch-ledger.json';expected='4e499d8560cc4c7a9bc4a2e725e27f502fa13979de6366e9f66bd06f237edcc9';assert sha(ledger)==expected
    l=json.loads(ledger.read_text());assert l['goal_closed']and len(l['executions'])==20 and l['remaining_processes']==80
    old=json.loads((E/'stage64-postflight.json').read_text())
    for r in old['checks']:assert sha(r['path'])==r['expected_sha256'],r['path']
    for p,h in l['pinned_files'].items():assert sha(p)==h,p
    for r in l['executions'][18]['checkpoint_files']:assert sha(ROOT/r['path'])==r['sha256']
    manifest=json.loads((E/'stage70-real-input-contract.json').read_text());regions=manifest['all_original_guest_regions']
    memory=Path(manifest['immutable_memory']['path']);assert sha(memory)==manifest['immutable_memory']['sha256']
    def read(a,n):
        r=next(r for r in regions if r['low']<=a and a+n<=r['high']+1)
        with memory.open('rb')as f:f.seek(r['offset']+a-r['low']);b=f.read(n)
        assert len(b)==n;return b
    gdt=read(0x60000000,56);descriptor=gdt[16:24];limit=(descriptor[0]|descriptor[1]<<8)|((descriptor[6]&15)<<16)
    base=descriptor[2]|descriptor[3]<<8|descriptor[4]<<16|descriptor[7]<<24
    assert base==0 and limit==0xfffff and descriptor[5]==0x93 and descriptor[6]&0xf0==0xc0
    reps=[]
    for pc,dest,count in [(0x4af380,0xec5a10,12996),(0x4af38c,0xeb8f00,12996),(0x4af4a7,0xc81b84,1000)]:
        for df in (0,1):
            low=dest-(count-1)*4 if df else dest;high=dest+4 if df else dest+count*4
            owner=next(r for r in regions if r['low']<=low and high<=r['high']+1 and r['permissions']&2)
            reps.append(dict(instruction_PC=hex(pc),source_sets_ECX=count,source_sets_EDI=hex(dest),DF=df,write_low=hex(low),write_exclusive_high=hex(high),bytes=count*4,
                all_elements_in_saved_writable_region=True,region=dict(low=hex(owner['low']),high=hex(owner['high']),permissions=owner['permissions'])))
    tests=json.loads((E/'stage72-own-mapping-tests.stdout.log').read_text());assert tests['passed']and tests['cases']==30 and not tests['game_object_linked']
    contracts=dict(status='NORMAL_REP_SPANS_AND_OWN_BOUNDARY_CHECKS_VALIDATED_RUNTIME_BINDING_PENDING',
        original_reference_or_native_game_execution=False,Unicorn_import_restore_or_execution=False,
        segment_static_evidence=dict(bootstrap_source='samples/stage7_runtime.py:53-59',bootstrap_sha256=sha(ROOT/'samples/stage7_runtime.py'),
            DS_ES_SS_selector=16,saved_GDT_address='0x60000000',descriptor16_hex=descriptor.hex(),descriptor_base=base,descriptor_effective_limit='0xffffffff',
            descriptor_normal_read_write_data=True,main_opaque_cached_base_not_read=True,
            static_saved_descriptor_not_complete_hidden_cache_proof=True,
            historical_linear_disassembly_contains_segment_mutators_or_data_artifacts=1356,
            required_future_check='Official restore/reg_read verifies saved selectors/CR0/GDTR; separate clone of unchanged saved context executes only self-authored segment-override load/store probes to unmapped logical endpoints and checks reported effective addresses. No segment reg_write to force flatness. If API/probe fails, stop before original instruction. This probe source is not yet implemented or run.'),
        normal_REP_access_spans=reps,
        REP_fault_scope='Only entire valid real-input spans closed statically. Invalid/overflow spans are declined before helper; original mid-fault ECX/EDI or cold resume inside REP is not equivalent/proven. First differential must not request mid-REP suspension.',
        real_heap_contract=dict(indices=manifest['pointer_indices'],objects=manifest['unique_allocated_objects'],
            source='Full vector and allocated objects from original snapshot; preserve original VA, not host pointer or native static-label rebase',
            dynamic_future_access_guard_required=True,all_future_generated_inputs_not_proven=True),
        own_boundary_tests=tests,
        own_runtime_source=[dict(path=p,sha256=sha(ROOT/p))for p in ['samples/stage72_runtime.c','samples/stage72_runtime.h','samples/stage72_selftest.c']],
        implemented_checks=['Sorted guest-region nonoverlap and32-bit interval overflow','1/2/4-byte access across contiguous regions with original permissions','Unmapped gap/RO-write refusal','Host-pointer underflow or32-bit conversion truncation refusal','Return-PC duplicate/native-target alias conflict refusal','Entire forward/back REP span validation and pre-call budget check','Ordered store logger on own buffers'],
        implemented_runtime_not_connected_to_game=True)
    (E/'stage72-input-and-preflight-contracts.json').write_text(json.dumps(contracts,indent=2)+'\n')
    a=json.loads((ROOT/'runs/stage70-city/analysis.json').read_text());returns=[dict(original_CALL_PC=hex(q['pc']),guest_return_PC=hex(q['end']),SR_native_continuation='loc_'+format(q['end'],'X'))for q in a['instructions']if q['name']=='call']
    assert len(returns)==5
    binding=dict(status='EXACT_BINDING_SPEC_NOT_BUILT',
        data='Prepare a new SR input with original numeric addresses retained and PE base-reloc directory zero, as original EXE. Stage71 reconstructed54 HIGHLOW entries must not rebase data labels for checkpoint execution. Verify SR emits rawVA accesses and no native data-label references in functions; retain Stage71 artifacts.',
        heap='All original32-bit pointer bytes remain unchanged; runtime address is shadow_offset+zero_extend(guestVA), with same full28-region guest map and logical permissions. No generic pointer scanning/rebasing or layout shrinking.',
        shadow_placement='Choose one16KB-aligned offset near compiled SR function addresses so all resolver targets fit unsigned32-bit difference; use non-overwriting Mach allocation for the rounded union of guest regions. Physical host pages may combine4KB guest permissions; enforce original guest permissions before every LLVM guest load/store, do not claim host page permissions alone suffice.',
        return_bindings=returns,outer_return=dict(guest_PC='0x4ac333',native_target='own stop thunk',expected_SP='0x700ffd8c',before_caller_instruction=True),
        original_stack_binding='Retain existing saved4af3cc and4ac333 words. Rewrite only generated5 PUSH loc_cont statements to their original numeric guest returnPC; original i386 bytes remain unchanged. Route generated7 RET target transfers through exact6-entry resolver; reject unknown PC/alias/range instead of jumping rawhost address.',
        entry='Use separately audited stack-neutral ccc->fastcc entry; SR automatic C wrapper pushes __return_procedure and must not be used for saved4af311.',
        instrumentation='Future generated-source binding pass adds instruction budget markers and guards/logging to guest memory operations. Retain fixed SR arithmetic/branches. For REP, instrument a separate licensed source copy to log actual stores, retain normal helper end-commit behavior; reject invalid full spans and never claim general fault/persistence equivalence.',
        implemented=False,reason_not_started='Parent requested handoff after current bounded build/self-tests at22-minute checkpoint; do not start another SR build/test tranche during report preparation.')
    (E/'stage72-precise-SR-address-binding-spec.json').write_text(json.dumps(binding,indent=2)+'\n')
    plan=dict(status='MINIMAL_RUN_SCOPE_SPEC_HARNESS_NOT_READY_EXECUTION_REFUSED',
        ready_now=['Own mapping checks can execute on authored buffers;30 cases passed','Real three REP full spans are writable for both DF directions','Public CPU restore/read adapter source exists but remains uncalled'],
        must_build_before_execution=['Raw-VA SR metadata variant and audited five CALL/seven RET binding pass','Connect guard/store trace runtime to generated SR memory instructions and REP source copy','Non-overwriting shadow allocation/input loader','Reference/native CLI harness and approval/attempt accounting'],
        must_run_to_judge=['Public saved tid1 registers and effective cached segment bases via isolated authored probes','Mach shadow placement availability and32-bit native-target fit','Actual original return state and write trace','Original-i386/SR-arm64 every-mapped-byte and CPU equality'],
        reference_source=dict(EXE='<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe',EXE_sha256=l['source_EXE_sha256'],engine='vendor-tools/unicorn-2.1.4/unicorn/lib/libunicorn.2.dylib',engine_sha256=sha(ROOT/'vendor-tools/unicorn-2.1.4/unicorn/lib/libunicorn.2.dylib'),input_manifest=str(E/'stage70-real-input-contract.json')),
        scope=dict(main_tid=1,checkpoint='cycle61a',entry_PC='0x4af311',entry_SP='0x700ffd68',stop_PC_before_instruction='0x4ac333',stop_SP='0x700ffd8c',instructions=214,original_bytes=728,no_scheduler_or_workers_or_imports_or_UI=True),
        processes=dict(reference_max=1,native_max=1,retries=0,cold_resume_processes=0),
        budget=dict(hard_seconds_per_process=60,guest_seconds_each=8,original_instruction_hooks_max=5000000,native_instruction_plus_REP_work_max=5000000,write_events_max=2000000,estimated_peak_memory_GiB='reference0.7–1.2; native0.35–0.7, unbenchmarked',full_guest_memory_bytes=manifest['immutable_memory']['bytes'],write_log_capacity_bytes=32000000),
        proposed_exact_commands_after_missing_harness_built=[
            '/usr/bin/python3 samples/run_bounded.py stage72-reference-once . /usr/bin/python3 samples/stage72_diff.py reference --approval evidence/stage72-run-approval.json --output runs/stage72-reference-once',
            '/usr/bin/python3 samples/run_bounded.py stage72-native-once . /usr/bin/python3 samples/stage72_diff.py native --approval evidence/stage72-run-approval.json --reference runs/stage72-reference-once/result.json --output runs/stage72-native-once'],
        commands_ready_to_run=False,command_file_and_approval_do_not_exist=True,
        approval='Separate explicit historical-snapshot scope and1+1 process accounting required. Old20/80closed ledger immutable; no run21, reopening or spontaneous retry.',
        failures='Any cached-base/API mismatch, shadow overlap/allocation failure, unexpected target, guard violation, budget/write cap or unequal mapped bytes/CPU fails closed. Do not repair state or mask mismatch.',
        comparison='Exact ordered successful store log and all28 mapped regions plus GPR/defined-live EFLAGS; unchanged segment/x87 state carried as official architectural values/opaque sidecar. Latercycle61p07 remains secondary only.',
        current_original_or_native_game_runs=0)
    (E/'stage72-minimal-differential-plan.json').write_text(json.dumps(plan,indent=2)+'\n')
    post=dict(all_passed=True,protected_hash_checks=len(old['checks']),source_pins=len(l['pinned_files']),ledger_sha256=expected,used=20,remaining=80,closed=True,latest_checkpoint='cycle62p19',no_run21=True,original_or_Unicorn_or_generated_game_library_execution=False)
    (E/'stage72-postflight.json').write_text(json.dumps(post,indent=2)+'\n')
    result=dict(status='BOUNDED_PREFLIGHT_PROGRESS_RUNTIME_BINDING_AND_ORACLE_STILL_BLOCKED',own_tests=tests,segment_hidden_cache_verified=False,
        normal_real_REP_spans_verified=6,guest_mapping_and_return_checker_implemented=True,SR_game_binding_implemented=False,
        differential_harness_implemented=False,precise_binding_spec=str(E/'stage72-precise-SR-address-binding-spec.json'),plan=str(E/'stage72-minimal-differential-plan.json'),postflight=post)
    (E/'stage72-result.json').write_text(json.dumps(result,indent=2)+'\n')
    (ROOT/'reports/stage72-bounded-preflight-checkpoint.md').write_text('''# Stage72有界检查点

30项自造映射/宽度/越界/别名冲突测试通过，链接仅自写runtime和自造stub，没有game object、Unicorn或SR游戏库执行。完成sorted不重叠guest region、uint32溢出、1/2/4字节跨相邻region访问、原权限/gap、host→guest截断/下溢、返回PC/native目标冲突检查及store日志。无效REP先拒绝，没有伪称fault中途状态等价。

真实三个REP的ECX12996/12996/1000及EDI来自固定原指令；六个完整跨度（正/反DF各三）都在已保存可写.data region内。只闭合正常访问边界，不能外推通用fault或中途保存/恢复。

segment静态来源已定位：stage7_runtime.py:53–59建立GDT base0、4GB普通读写data descriptor并把DS/ES/SS设为16。历史memory.bin中descriptor16仍与之相符。opaque main CPU缓存未读取；全text线性反汇编1356处segment-mutator/数据伪指令不能作为全历史无segment改变证明，故不宣称缓存已静态证明。未来参考进程先用官方完整context恢复/reg_read检查，再以独立clone的自造override load/store probe核对有效地址；失败须在执行原指令前停止。本轮未调用这些API，probe源尚未实现。

真实SR接入的精确方案已写：新build保留原数值VA/关闭重建fixup，统一shadow_offset加原guestVA；全部28原region和原heap指针原样保留，逻辑guard覆盖4KB guest权限与16KB host页差异。五个原call continuation加4ac333构成六项有限返回绑定；仅生成代码PUSH改为原PC、RET经resolver，输入栈不改，原728B不改。此binding pass和raw-VA SR新产物尚未实现/构建，Stage71库不能直接拿来跑。

最小差分范围明确为一original/Unicorn参考进程加一SRnative进程，不做冷恢复、不重试；cycle61a tid1从4af311/SP700ffd68到4ac333/SP700ffd8c之前，全部214PC闭包，无UI/import/scheduler/worker。guest8秒，hard60秒，500万source-step/REP-work及200万write events，内存原297521152字节另32MB日志；峰值内存仅估计0.7–1.2GiB参考/0.35–0.7GiB native，未实测。计划列出未来命令但stage72_diff.py及approval记录尚不存在，因此不是ready-to-run命令。严格拒绝条件和已具自造验证条件/必须build或run的项已分列。

按父线程22分钟有界交回要求，在当前构建和自造测试后停止新SR/harness扩展；没有为了完整报告再开测试或原执行。3482保护哈希、550源码pins、原EXE、历史memory及最新checkpoint审计通过，20/80closed账本未改，无第21。交付stage72-result.json、stage72-input-and-preflight-contracts.json、stage72-precise-SR-address-binding-spec.json、stage72-minimal-differential-plan.json及stage72-postflight.json。
''')
    print(json.dumps(dict(status=result['status'],own_cases=30,normal_REP_spans=6,protected_checks=len(old['checks']),pins=len(l['pinned_files']),runs=0)))
if __name__=='__main__':main()
