# -*- coding: utf-8 -*-
"""Prepared-only handoff: no approval/ledger changes and no production entry call."""
import ast,json,os
from pathlib import Path
from stage54_batch import sha
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
EXPECTED='b807f91bf787dd0b8e06af321185affd194ac3aaa4ae0c56497d48468126b212'
def main():
    ledger_path=E/'stage55-batch-ledger.json';assert sha(ledger_path)==EXPECTED
    l=json.loads(ledger_path.read_text());assert len(l['executions'])==19 and l['remaining_processes']==81 and l['goal_closed']
    diagnosis=json.loads((E/'stage63-offline-diagnosis.json').read_text());test=json.loads((E/'stage63-selfcheck.json').read_text())
    assert test['passed'] and diagnosis['checkpoint_complete_on_current_filesystem'] and diagnosis['no_old_task_processes_remain']
    wrapper=ROOT/'samples/checkpoint_startup63.py';tree=ast.parse(wrapper.read_text())
    top_imports=[n.module if isinstance(n,ast.ImportFrom) else [a.name for a in n.names] for n in tree.body if isinstance(n,(ast.Import,ast.ImportFrom))]
    assert not any('unicorn' in str(n) or 'checkpoint_startup61' in str(n) or 'stage20_threads' in str(n) for n in top_imports)
    source=wrapper.read_text();assert source.index('index,ledger=claim(')<source.index('from checkpoint_startup61 import')
    assert 'gate.allowed=True' in source and 'if gate.attempts:' in source and 'kernel_run(self,seconds' in source
    assert 'raw,n,None)' in source and 'screencapture' not in source and 'os._exit' not in source
    assert 'expected=decode(m[\'host\'])[\'guest_threads20\']' in source and 'r.primary54()' in source
    checks=[];cache={};protected=json.loads((E/'stage62-postflight.json').read_text())
    for q in protected['checks']:
        p=q['path']
        if p not in cache:cache[p]=sha(p)
        checks.append(dict(path=p,expected_sha256=q['expected_sha256'],sha256=cache[p],unchanged=cache[p]==q['expected_sha256']))
    assert all(q['unchanged'] for q in checks)
    for p,h in l['pinned_files'].items():assert sha(p)==h,p
    baseline=json.loads((E/'stage59-postflight.json').read_text())
    assert all(not Path(q['path']).exists() for q in baseline['absences'])
    assert all(os.readlink(q['path'])==q['target'] for q in baseline['DOS_namespace_symlinks'])
    names=['checkpoint_startup63.py','stage63_checkpoint.py','stage63_selfcheck.py','stage63_diagnose.py','stage63_finalize.py']
    additions={str(ROOT/'samples'/name):sha(ROOT/'samples'/name) for name in names}
    pins=dict(l['pinned_files']);pins.update(additions)
    manifest=dict(status='PREPARED_ONLY_NOT_EXECUTION_APPROVAL',existing_ledger_sha256=EXPECTED,
        original_existing_pins_retained=True,pinned_files=pins,new_sources=additions,
        ledger_remains_closed=True,used=19,remaining=81,next_index=20,resume='cycle62p19',
        guest_max_seconds=8,save_cleanup_exit_reserve_seconds=25,whole_process_hard_seconds=60,
        new_DEVICE_ATTACH=False,history_trimmed=False,counters_reset=False,
        next_command=['/usr/bin/python3','samples/run_bounded.py','stage63-run20-bounded-cycle','.',
            '/usr/bin/python3','samples/checkpoint_startup63.py','--mode','cycle','--resume','cycle62p19','--tag','cycle63p20'],
        original_instructions_executed=0,normal_full_runtime_below60_proven=False)
    (E/'stage63-prepared-continuation-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    assert sha(ledger_path)==EXPECTED
    (E/'stage63-postflight.json').write_text(json.dumps(dict(checks=checks,all_passed=True,
        protected_hash_checks=len(checks),existing_source_pins_checked=len(l['pinned_files']),
        absences_unchanged=True,DOS_links_unchanged=True,ledger_unchanged=True),indent=2)+'\n')
    t=diagnosis['timings'][-1]
    result=dict(status='offline_recovery_conditions_passed_prepared_no_run20',
        checkpoint='cycle62p19',complete_files_and_memory_regions=True,complete_CPU_and_four_threads=True,
        exact_input_and_observer_state=True,atomic_original_checkpoint=False,
        current_filesystem_complete_and_verified=True,latest_actual_cold_restore_not_run=True,
        old_owner_PIDs_absent=True,window_close_not_visible_records=True,
        global_audio_state_not_probed=True,normal_exit_of19_not_proven=True,
        historical_timings=t,host_history_largest_fields=diagnosis['largest_encoded_host_fields'][:4],
        own_fast_encode_seconds=test['total_fast_encode_seconds'],own_legacy_encode_seconds=test['total_legacy_encode_seconds'],
        encoder_compatibility_fields=178,new_wrapper_max_guest_seconds=8,adaptive_shorter=True,
        whole_hard_limit_seconds=60,save_cleanup_exit_reserve_seconds=25,
        no_repeated_PNG_or_window_screencapture=True,real_native_present_and_GPU_integrity_checks_preserved=True,
        full_restore_hashes_and_CPU_checks_preserved=True,new_save_incremental_hashes_avoid_second_disk_scan=True,
        new_save_atomic_directory_fixture_passed=True,original_history_not_trimmed=True,
        same_scope_normal_timeout_recovery_candidate=True,
        retry_counts_as_new_original_process20=True,original_budget_reused_or_reset=False,
        ledger_closed_and_unchanged=True,ledger_sha256=EXPECTED,used=19,remaining=81,
        phase=22,advances=17,entries=17,tail=0,original_instructions_executed=0,
        original_runtime_constructed=False,original_DLL_executed=False,
        greater_permissions_requested=False,new_execution_source_introduced=False,
        source_files_modified=False,files_uploaded=False,host_settings_changed=False,
        protected_hash_checks=len(checks),existing_pins_checked=len(l['pinned_files']),
        new_prepared_pins=len(pins),prepared_manifest_sha256=sha(E/'stage63-prepared-continuation-manifest.json'),
        limitation='Offline file/schema/CPU/device checks cannot prove future native-adapter cold restoration or whole process runtime; full restore and gate checks precede all original execution in prepared entry.')
    (E/'stage63-result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
    report=f'''cycle62p19具备带完整校验的恢复条件：三文件、所有内存段、完整CPU及4线程上下文、同域输入及共享观察器与关闭账本相符。主线程CPU payload与context.bin一致；受控设备保存体与退出日志一致，纯状态恢复不产生新DEVICE_ATTACH。最新检查点尚未实际构造Unicorn/原生适配器冷恢复，下一次正式过程必须先通过已有全部restore assertions，才开放原指令执行。

保存原子性：旧stage12保存直接写memory.bin/context.bin/state.json，不fsync文件，也无三文件原子提交；账本finish本身是临时文件fsync+rename。此次所有文件内容与账本哈希一致，内存段连续/长度完整，JSON完整解析，状态写入早于finish，故当前文件系统没有截断证据；这不证明突发断电耐久性。新准备的相同format1保存使用独立pending目录，完整写入/fsync后一次目录rename发布，惰性中断试验未发布半份状态，原检查点文件和历史不动。

残留：原16–19 PID82505/82778/83037/83277全部不存在，均有native_window_closed_and_not_visible记录；第19次退出未正常证明，但owner process已由硬限终止，原音频AudioUnit stop/uninitialize/dispose源链及窗口close记录存在。未启动新窗口/音频、未探测或修改系统全局音频，也未扩大权限。执行控制器此前已报告结束。

第19次历史耗时估计（来自文件mtime/birthtime及claim，非历史函数探针）：恢复/准备26.316秒；guest及输入退出14.030秒；原内存写出/regionhash0.287秒；context写出0.001秒；host字段采集、typed encode与JSON生成17.681秒；state文件写入0.144秒；后续文件hash及输出0.481秒；native dispose记录间0.010秒。Python/原生最终退出未单独测量，不能把dispose这0.010秒等同完整退出耗时。该轮有一个native primary PNG，代价含在guest分段中无法单独量化；首帧owned capture已保存，因此第16–19过程没有外部screencapture。数据主要不是磁盘写入慢：182.7MB中progress历史83.4MB、api_trace27.4MB、dd_native_blob23.0MB、clock_calls14.1MB，历史不裁剪。

准备的checkpoint_startup63.py最大guest8秒，并按60−已用时间−25秒余量自动更短，不足1秒则执行前拒绝。仍通过同一claim/finish和硬60外层运行器，保留来源绑定、完整映射内存/CPU/4线程/native恢复assertions、同域设备、后台/静音和原Watch上下限，已有首帧review与capture继续保留。原生primary present实际调用和GPU零差异/源像素不变验证仍执行，只给现有native ABI传NULL PNG路径以取消每轮PNG；不会重拍已证明首帧。新保存把全文件hash在原内存写入或待写JSON字节上一次取得，避免postsave再次从磁盘扫描大文件，不减少下次恢复的完整校验。178个实际保存字段逐项与旧encoder完全一致，纯状态测试old encode {test['total_legacy_encode_seconds']:.3f}秒/新 {test['total_fast_encode_seconds']:.3f}秒，这不是原运行新整体耗时保证。原子保存、中断不发布、hash等于落盘字节、预算和惰性present路径测试通过；没有运行生产main或第20次。

范围判断：这是同一固定EXE/Unicorn/输入域/城市有限调度任务的正常超时恢复候选，没有新原执行来源或更大权限。续接应从cycle62p19作为新第20次计数，不能退回phase或免费重跑19。当前本轮只准备，关闭账本及approval未修改；准备manifest是PREPARED_ONLY，不是假审批。技术续接需把新增源码绑定并按同范围处理已关闭超时状态，随后新第20次若cold restore失败或触限，仍立即停止。累计19used/81余，推进17/51、入口17/128、尾0/1、phase22维持。完整原生AOT、真实键盘等价、首次尾部及完整周期尚未证明。

交付evidence/stage63-offline-diagnosis.json、stage63-timing-raw.json、stage63-selfcheck.json、stage63-prepared-continuation-manifest.json、stage63-result.json及stage63-postflight.json。3482项旧保护hash、544项既有绑定、缺失18及DOS链接2全部不变。新增原指令0、原DLL0，不上传/安装/宿主设置/提交/发布。
'''
    (ROOT/'reports/stage63-timeout-recovery-readiness.md').write_text(report)
    print(json.dumps(result,ensure_ascii=False))
if __name__=='__main__':main()
