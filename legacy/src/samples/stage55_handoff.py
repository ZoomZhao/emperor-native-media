"""Read-only postflight and local handoff; never imports an emulator/runtime."""
import datetime
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / 'evidence'
CACHE = {}
CHECKS = []


def load(name):
    return json.loads((EVIDENCE / name).read_text())


def sha(path):
    path = Path(path)
    key = str(path)
    if key not in CACHE:
        h = hashlib.sha256()
        with path.open('rb') as source:
            for block in iter(lambda: source.read(1024 * 1024), b''):
                h.update(block)
        CACHE[key] = h.hexdigest()
    return CACHE[key]


def check(path, expected, scope):
    path = Path(path)
    actual = sha(path)
    CHECKS.append(dict(path=str(path), expected_sha256=expected,
                       sha256=actual, unchanged=actual == expected, scope=scope))


def main():
    approval = load('stage55-batch-approval.json')
    ledger = load('stage55-batch-ledger.json')
    manifest = load('stage55-prepared-manifest.json')
    restore = load('stage54-restore55.json')
    bounded = load('stage55-run01-cold-restore.result.json')
    probe = load('stage55-own100-cap-probe.json')
    prior = load('stage52-postflight.json')
    for row in prior['checks']:
        check(row['path'], row['sha256'], 'prior protected source/frozen inputs')
    for path, expected in ledger['pinned_files'].items():
        check(path, expected, 'current approved source/runtime/checkpoint pin')
    check(EVIDENCE / 'stage51-batch-ledger.json', ledger['old_ledger_sha256'],
          'old closed ledger must remain unchanged')
    check(EVIDENCE / 'stage55-prepared-manifest.json',
          approval['prepared_manifest_sha256'], 'approved manifest')

    absences = []
    for row in prior['absences']:
        p = Path(row['path'])
        still_missing = not p.exists() and (
            not p.parent.is_dir() or not any(
                child.name.casefold() == p.name.casefold()
                for child in p.parent.iterdir()))
        absences.append(dict(path=str(p), still_missing=still_missing))
    links = []
    for row in prior['DOS_namespace_symlinks']:
        p = Path(row['path'])
        target = str(p.readlink()) if p.is_symlink() else None
        links.append(dict(path=str(p), target=target,
                          unchanged=target == row['target']))

    files = []
    for name in ('context.bin', 'memory.bin', 'state.json'):
        path = ROOT / 'runs' / 'stage12-state-returnwatch51' / name
        files.append(dict(path=str(path.relative_to(ROOT)), bytes=path.stat().st_size,
                          sha256=sha(path)))
    command = ('/usr/bin/python3 samples/run_bounded.py stage55-run02-original-caller . '
               '/usr/bin/python3 samples/checkpoint_startup54.py --mode caller '
               '--resume returnwatch51 --tag caller55')
    rejections = [
        dict(attempt=1, command=command, process_created=False,
             consumes_batch_process=False, original_instructions_executed=0,
             reason='该命令会在模拟器中执行原版 EXE，而可信用户指令明确要求“不执行”原版；虽有60秒和只读等限制，仍超出授权范围。'),
        dict(attempt=2, command=command, process_created=False,
             consumes_batch_process=False, original_instructions_executed=0,
             readonly_authorization_check_before_resubmission=True,
             reason='新增材料仍是助手转述和本地记录，不能证明用户本人明确覆盖“不得执行原版 EXE”的限制；该命令仍会执行原版指令，因此授权不足。'),
    ]
    (EVIDENCE / 'stage55-auto-review-rejections.json').write_text(
        json.dumps(dict(command=command, attempts=rejections,
                        source='exec_command CreateProcess approval errors; manually transcribed',
                        raw_child_failure_log_exists=False,
                        explanation='Both were rejected before process creation; no stdout/stderr/result for run02.'),
                   ensure_ascii=False, indent=2) + '\n')
    audit = dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                 checks=CHECKS, hash_checks=len(CHECKS), unique_paths=len(CACHE),
                 all_protected_hashes_unchanged=all(x['unchanged'] for x in CHECKS),
                 absences=absences,
                 all_observed_missing_inputs_still_missing=all(x['still_missing'] for x in absences),
                 DOS_namespace_symlinks=links,
                 all_namespace_symlinks_unchanged=all(x['unchanged'] for x in links),
                 original_instructions_executed=0,
                 emulator_or_native_runtime_imported=False,
                 source_files_written=False)
    (EVIDENCE / 'stage55-postflight.json').write_text(
        json.dumps(audit, ensure_ascii=False, indent=2) + '\n')
    assert len(ledger['executions']) == 1 and ledger['remaining_processes'] == 99
    assert ledger['executions'][0]['status'] == 'finished'
    assert ledger['cold_restore_passed'] and not ledger['caller_passed']
    assert ledger['city_frame_review'] is None and not ledger['goal_closed']
    assert restore['result']['status'] == 'cold_restore_passed'
    assert restore['emu_start_attempts'] == restore['emu_start_calls'] == 0
    assert restore['CPU_PC'] == '0x401aa8' and restore['current_tid'] == 1
    assert restore['native_audio']['source_frames'] == 0
    assert bounded['returncode'] == 0 and not bounded['timed_out']
    assert bounded['duration_seconds'] < 60 and probe['passed']
    assert all(audit[k] for k in ('all_protected_hashes_unchanged',
                                 'all_observed_missing_inputs_still_missing',
                                 'all_namespace_symlinks_unchanged'))
    outcome = dict(
        created_utc=audit['created_utc'], scope='stage55 cold restore and approval-blocked caller',
        authorization_record='evidence/stage55-batch-approval.json',
        actual_user_reply_received_from_parent=approval['user_reply'],
        source_approval_utc=approval['user_approval_utc'],
        approval_review_still_blocks_original_EXE_execution=True,
        new_batch_maximum_processes=100, new_batch_processes_used=1,
        new_batch_remaining_processes=99, original_execution_processes_in_new_batch=0,
        original_instructions_in_new_batch=0, all_new_process_durations_under60=True,
        cold_restore_passed=True, cold_restore_whole_process_seconds=bounded['duration_seconds'],
        complete_memory_CPU_four_threads_native_restored=True,
        caller_after_loader_return_executed=False,
        original_caller_return_proven=False, first_city_view_proven=False,
        original_scheduler_entries_observed=0, original_phase_advances_observed=0,
        original_tail_returns_observed=0, full_cycle_proven=False,
        new_original_AOT_conversion_proven=False,
        frozen_checkpoint=ledger['latest_checkpoint'], checkpoint_files=files,
        restored_natural_state=restore['natural_state'], restored_PC=restore['CPU_PC'],
        original_source_EXE_sha256=ledger['source_EXE_sha256'],
        source_and_runtime_pin_count=manifest['pinned_file_count'],
        shared_caps=ledger['caps'],
        stage55_own_synthetic_probe_cases=probe['case_count'],
        own_probe_original_instructions_executed=0,
        synthetic100claims_are_not100actual_processes=True,
        first_tail_cap_requires_stop_at_phase0_partial_not_full5to5=True,
        old_closed_budget_remaining1_not_reused=True,
        postflight_hash_checks=audit['hash_checks'],
        postflight_unique_paths=audit['unique_paths'],
        all_protected_sources_frozen_inputs_checkpoints_unchanged=True,
        all_absences_and_symlinks_unchanged=True,
        no_source_save_or_profile_writes=True, source_PCM_sent_to_device=False,
        original_DLL_exports_executed=False, files_uploaded=False,
        system_settings_changed=False, global_dependencies_installed=False,
        committed_or_published=False, blocked_command=command,
        approval_rejections=rejections,
        next_required_action='Parent obtain direct user confirmation that fixed original EXE instructions may execute within already approved Unicorn2.1.4, overriding initial no-EXE-execution limitation; same remaining99 and other caps, no budget reset.',
        stop_reason='Automatic approval review rejected original caller execution twice; no bypass or further original execution attempted.')
    (EVIDENCE / 'stage55-result.json').write_text(
        json.dumps(outcome, ensure_ascii=False, indent=2) + '\n')
    report = f'''Stage55：冷恢复通过；原版调用者执行被自动审批阻塞。

本批实际只启动1个进程：从既有returnwatch51检查点冷恢复，整个进程{bounded['duration_seconds']}秒、returncode0、没有超时，完整映射内存/CPU/四线程/本任务原生资源状态恢复通过。实际PC0x401aa8、tid1，emu_start尝试和调用均0，原版指令0。自然状态screen13/route1/phase5/gate0，人口28、金库1633、周期计数119/7；source_frames=0。新批100次中已用1、剩99；旧stage51剩1未动，旧目标仍关闭。没有重复存档加载。

包装器和独立账本已为父线程转交的2026-10-07 06:43:08 UTC用户原文“允许，一次性批准 100 次”准备完毕。准备清单锁定{manifest['pinned_file_count']}项当前源码/运行库/检查点输入；审批记录、当前清单和原EXE固定SHA均已保存。原指令仅在实际claim后开启。生产新版的14项纯自造检查通过，包含100次总计claim、已完成caller不可重跑、101次拒绝、原调度入口128上限、51推进上限、首次真实尾返回即停。自造100个claim不等于执行100个进程，测试未读取原EXE、未导入模拟器或原生运行库。历史stage54的50项是旧8次提案的检查；此次改100及尾即停后的直接证据是stage55-own100-cap-probe.json与成功冷恢复，不混淆历史版本。

下一条已准备的命令：

```sh
{command}
```

这条命令两次均在CreateProcess前被自动审批拒绝，未形成子进程/子日志/检查点，不消耗第2次。第一次理由：{rejections[0]['reason']} 第二次是在只读核对后续批准记录并附准确原文重提同一命令后，理由：{rejections[1]['reason']} 拒绝内容转录保存在stage55-auto-review-rejections.json，没有伪造原游戏失败日志。

需要父线程处理的精确授权缺口：审批器要求用户本人明确覆盖最初“不执行原版EXE”的限制，允许固定EXE原指令在现有已获批准Unicorn2.1.4内执行。本地助手记录和父线程转述未被审批器接受。可确认的范围仍是本批剩99次、每个整个进程不超过60秒、固定源只读、仅任务目录写入、后台静音、不执行原DLL、不上传、不改系统设置；共享51推进/1尾/128调度入口上限不增加、不重置。

caller自然返回、第一张真正城市画面、有限原调度均未执行，因此全部未通过。首次从phase5到phase0的尾最多是46次自然推进后的部分区间，不能声称完整5到5周期；即使还剩进程预算，也必须在首次尾或其他任一共享上限处停。没有菜单图或恢复呈现被当作城市逻辑证据。此前stage52证明的是原loader在模拟器中自然成功返回；此轮也没有证明新root AOT、全原生WinMain或完整游戏原生化。

停止后只做只读保护复核和本地交接文档：{audit['hash_checks']}项哈希检查、{audit['unique_paths']}独立路径全部一致；缺失文件和DOS链接均不变。原EXE、冻结资源/存档/档案、旧代码/检查点、当前已批准源码/库与returnwatch51三文件均未变化。源PCM未送设备，后台窗口非key/非active、激活请求0；没有源写入、原DLL调用、上传、依赖安装、安全设置改变、提交或发布。当前returnwatch51冷恢复已实证，但后续行为的未知API、城市音乐、绘制语义、调度/状态等价性仍待获准原指令验证；不得用静态推测填成通过。

交付：evidence/stage55-result.json、stage55-postflight.json、stage55-auto-review-rejections.json、stage54-restore55.json、stage55-run01-cold-restore.result.json、stage55-own100-cap-probe.json。可恢复位置为runs/stage12-state-returnwatch51，三文件SHA/大小见结果JSON；没有新增原执行检查点。源码samples/checkpoint_startup54.py、stage54_watch.py、stage54_batch.py已经锁定，不在停留期间改动或重置预算。
'''
    (ROOT / 'reports' / 'stage55-cold-restore-success-execution-approval-blocked.md').write_text(report)
    print(json.dumps(dict(cold_restore_passed=True, whole_process_seconds=bounded['duration_seconds'],
                          used=1, remaining=99, original_instructions=0,
                          approval_blocked=True, hash_checks=audit['hash_checks'],
                          unique_paths=audit['unique_paths'], protected_inputs_unchanged=True),
                     ensure_ascii=False))


if __name__ == '__main__':
    main()
