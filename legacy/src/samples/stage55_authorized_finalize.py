"""Finalize actual authorized evidence with read-only hashes; no emulator imports."""
import datetime
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EV = ROOT / 'evidence'
CACHE = {}
CHECKS = []


def load(name):
    return json.loads((EV / name).read_text())


def sha(path):
    path = Path(path)
    if str(path) not in CACHE:
        h = hashlib.sha256()
        with path.open('rb') as f:
            for block in iter(lambda: f.read(1024 * 1024), b''):
                h.update(block)
        CACHE[str(path)] = h.hexdigest()
    return CACHE[str(path)]


def check(path, expected, scope):
    actual = sha(path)
    CHECKS.append(dict(path=str(path), expected_sha256=expected,
                       sha256=actual, unchanged=actual == expected, scope=scope))


def main():
    ledger_path = EV / 'stage55-batch-ledger.json'
    ledger = load(ledger_path.name)
    ledger_before = sha(ledger_path)
    assert len(ledger['executions']) == 3 and ledger['remaining_processes'] == 97
    assert ledger['goal_closed'] and ledger['cold_restore_passed'] and ledger['caller_passed']
    assert ledger['latest_checkpoint'] == 'frame55a' and ledger['city_frame_review'] is None
    watch = ledger['watch_state']
    assert watch['entries'] == 0 and not watch['advances'] and watch['tail_returns'] == 0
    assert watch['refresh_returns'] == 0 and watch['frame_candidate'] is None
    assert watch['phase'] == 5 and not watch['full_phase_cycle']
    restore = load('stage54-restore55.json')
    caller = load('stage54-caller55.json')
    frame = load('stage54-frame55a.json')
    assert restore['emu_start_attempts'] == restore['emu_start_calls'] == 0
    assert caller['CPU_PC'] == '0x44a7e2'
    assert caller['result']['boundary']['kind'] == 'Caller_return_goal_stop54'
    assert caller['watch_state']['observations'][-1]['sp'] == '0x700ffaa4'
    assert frame['result']['boundary']['kind'] == 'Stream_open_parameters_unreviewed44'
    assert frame['CPU_PC'] == '0x610017c0'
    rows = []
    labels = ['stage55-run01-cold-restore', 'stage55-run02-original-caller',
              'stage55-run03-first-city-frame']
    for q, label in zip(ledger['executions'], labels):
        bounded = load(label + '.result.json')
        assert q['status'] == 'finished' and bounded['returncode'] == 0
        assert not bounded['timed_out'] and bounded['duration_seconds'] < 60
        row = dict(index=q['index'], mode=q['mode'], resume=q['resume'], tag=q['tag'],
                   whole_process_seconds=bounded['duration_seconds'],
                   raw_runtime_status=q['result']['status'],
                   boundary=q['result'].get('boundary'), hard_outer_budget_passed=True)
        rows.append(row)
        for p in q['checkpoint_files']:
            check(ROOT / p['path'], p['sha256'], 'new checkpoint preserved')
    for run in [restore, caller, frame]:
        assert run['native_audio']['source_frames'] == 0
        diag = run['native_diagnostic']
        assert diag['key_window'] == diag['NSApp_isActive'] == diag['activation_requests'] == 0
        assert diag['current_pid'] != diag['frontmost_pid']
        assert not run['original_code_patched'] and not run['game_fields_injected']
        assert not run['original_DLL_exports_executed'] and not run['files_uploaded']
    prior = load('stage52-postflight.json')
    for p in prior['checks']:
        check(p['path'], p['sha256'], 'old source/frozen inputs/checkpoints')
    for path, expected in ledger['pinned_files'].items():
        check(path, expected, 'approved source/runtime/returnwatch51 pins')
    check(EV / 'stage51-batch-ledger.json', ledger['old_ledger_sha256'], 'old closed ledger')
    check(EV / 'stage55-prepared-manifest.json', ledger['prepared_manifest_sha256'], 'approved manifest')
    absences = []
    for q in prior['absences']:
        p = Path(q['path'])
        missing = not p.exists() and (not p.parent.is_dir() or not any(
            c.name.casefold() == p.name.casefold() for c in p.parent.iterdir()))
        absences.append(dict(path=str(p), still_missing=missing))
    links = []
    for q in prior['DOS_namespace_symlinks']:
        p = Path(q['path'])
        target = str(p.readlink()) if p.is_symlink() else None
        links.append(dict(path=str(p), target=target, unchanged=target == q['target']))
    audit = dict(created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                 hash_checks=len(CHECKS), unique_paths=len(CACHE), checks=CHECKS,
                 all_protected_hashes_unchanged=all(p['unchanged'] for p in CHECKS),
                 absences=absences,
                 all_observed_missing_inputs_still_missing=all(p['still_missing'] for p in absences),
                 DOS_namespace_symlinks=links,
                 all_namespace_symlinks_unchanged=all(p['unchanged'] for p in links),
                 original_instructions_executed_during_audit=0,
                 emulator_or_native_runtime_imported=False)
    (EV / 'stage55-authorized-postflight.json').write_text(
        json.dumps(audit, ensure_ascii=False, indent=2) + '\n')
    assert all(audit[k] for k in ['all_protected_hashes_unchanged',
                                 'all_observed_missing_inputs_still_missing',
                                 'all_namespace_symlinks_unchanged'])
    # sha() caches hashes for expensive protected files; explicitly read the ledger again.
    assert hashlib.sha256(ledger_path.read_bytes()).hexdigest() == ledger_before
    music = load('stage55-city-music-contract-review.json')
    assert music['original_wrapper_frame']['loop_count_argument'] == 1
    assert music['original_wrapper_frame']['effective_volume_before_open'] == 20
    assert music['only_open_guard_difference_is_new_city_path']
    next_plan = dict(
        status='reviewable followup only; no adapter change or original continuation performed',
        existing_closed_ledger=str(ledger_path.relative_to(ROOT)),
        used=3, remaining_unused=97, budget_must_not_reset=True,
        shared_caps=ledger['caps'], hard_process_seconds=60,
        actual_restore_point='frame55a', original_PC_at_platform_import='0x610017c0',
        original_wrapper_API_return='0x762d46',
        exact_resource=music['music_files'][0], actual_wrapper=music['original_wrapper_frame'],
        necessary_work=[
            'Bind the exact city MP3 path and verified frozen SHA, keeping other unknown paths rejected.',
            'Exercise actual Apple offline PCM decoding, owned stream creation, loop1 completion/close and save/restore with the existing self-authored backend; source PCM device output must remain0.',
            'Review original loop-count call at return762daf with value1 and retain volume20/callsite/driver/buffer checks; do not substitute return0 or force game fields.',
            'Update source pin manifest and obtain a narrowly reviewed continuation scope because this batch was sealed at unknown behavior; preserve used3/remaining97 and zero shared scheduler counts.',
            'Resume only from the saved blocked import after review, stop on next unknown behavior or any existing shared cap; city candidate still requires actual refresh return and local content review.'
        ],
        original_MSS_exact_decoder_mixer_equivalence_unproven=True,
        original_scheduler_and_new_AOT_still_unproven=True)
    (EV / 'stage55-next-music-review-plan.json').write_text(
        json.dumps(next_plan, ensure_ascii=False, indent=2) + '\n')
    outcome = dict(
        created_utc=audit['created_utc'],
        scope='approved original caller and first-city-frame bounded trial',
        direct_user_approval='evidence/stage55-direct-user-exe-approval.json',
        prior_approval_provenance_block_resolved=True,
        new_batch_maximum_processes=100, used=3, remaining_unused=97,
        batch_goal_closed=True, stop_reason=ledger['stop_reason'],
        hard_process_seconds=60, all_whole_process_budgets_passed=True, runs=rows,
        cold_restore_passed=True, original_caller_natural_return_proven=True,
        caller_return_PC='0x44a7e2', caller_return_SP='0x700ffaa4',
        screen_transition=dict(before=13, after=1), original_zero_AL_branch_observed=True,
        original_caller_goal_stop_before_outer_caller_instruction=True,
        first_city_frame_candidate_proven=False, city_frame_semantic_content_reviewed=False,
        original_city_refresh_returns=0, scheduler_entries=0, productive_phase_advances=0,
        tail_entries=0, tail_returns=0, full_phase_cycle_proven=False,
        shared_caps=ledger['caps'], actual_boundary=frame['result']['boundary'],
        latest_checkpoint='frame55a', latest_checkpoint_files=ledger['executions'][-1]['checkpoint_files'],
        latest_PC='0x610017c0', latest_checkpoint_separate_cold_restore_tested=False,
        natural_state=frame['natural_state'],
        source_EXE_sha256=ledger['source_EXE_sha256'],
        old_closed_ledger_remaining1_not_reused=True,
        old_closed_ledger_sha256=ledger['old_ledger_sha256'],
        source_pin_count=load('stage55-prepared-manifest.json')['pinned_file_count'],
        hash_checks=audit['hash_checks'], unique_protected_paths=audit['unique_paths'],
        all_protected_inputs_and_new_checkpoints_unchanged=True,
        all_absences_and_symlinks_unchanged=True,
        source_PCM_sent_to_device=False, original_DLL_exports_executed=False,
        original_code_patched=False, game_fields_injected=False,
        foreground_activation_requested=False, source_save_or_profile_written=False,
        files_uploaded=False, system_settings_changed=False,
        dependencies_installed=False, committed_or_published=False,
        new_original_root_AOT_proven=False, full_native_game_proven=False,
        music_static_review=music, followup_plan='evidence/stage55-next-music-review-plan.json',
        recommendation='Worth a narrowly bounded continuation after exact music-contract review: actual caller, pointers and ABI returned naturally; present block is known path/loop support. This evidence does not establish city logic or full native recompilation feasibility.')
    (EV / 'stage55-authorized-result.json').write_text(
        json.dumps(outcome, ensure_ascii=False, indent=2) + '\n')
    table = '\n'.join(f"| {p['index']} | {p['mode']} | {p['whole_process_seconds']} | "
                      f"{p['boundary']['kind'] if p['boundary'] else 'cold_restore_passed'} |" for p in rows)
    report = f'''本批通过冷恢复与原版调用者自然返回；城市首帧试验在未审查的城市音乐入口按约定停止。

用户在当前本机执行会话直接授权固定EmperorEN.exe原指令在Unicorn2.1.4中执行、覆盖最初限制并沿用剩99次后，原审批来源阻塞已解决。原EXE固定SHA {ledger['source_EXE_sha256']}，平台接口仍为本任务自写arm64适配，原指令仍在模拟器中运行；本批不证明新root AOT、全原生WinMain或完整游戏原生化。

| 本批进程 | 模式 | 整进程秒 | 结束边界 |
|---|---|---:|---|
{table}

三个整进程均正常结束且小于60秒。第2个运行的raw status是refused，但具名Caller_return_goal_stop54是预设成功停止点：原AL0分支0x401adc实际经过，原调用者在tid1自然返回0x44a7e2、ESP0x700ffaa4，外层调用者第一条指令未执行；screen13自然切到1，phase5/gate0/人口28/金库1633/周期计数119和7保持。这不是注入字段或以图片宣布逻辑通过。第1个冷恢复完整内存/CPU/四线程/native断言通过、emu_start0；第2个恢复returnwatch51、第3个恢复caller55也通过包装器的恢复断言。

第3个从caller55试城市首帧，实际在0x610017c0自写平台导入边界拒绝mss32.dll!_AIL_open_stream@12。原返回PC0x762d46，参数driver0x5c000101、path pointer0x115510c、buffer256000，实际路径audio\\music\\BG01-Lonely_Er_Hu.MP3。已有适配器只审查菜单音乐menu1-san_xuan.mp3，因此新路径触发Stream_open_parameters_unreviewed44。没有执行原MSS DLL，没有给城市音频伪造成功句柄，也没有跳过原音乐调用。拒绝前尚无city refresh返回、首帧候选或scheduler入口；不宣称城市画面或模拟通过。

实际未知边界已自动封闭同一stage55账本：已用3/100，剩97只是未用次数，不能直接继续或重置。旧stage51仍已用9/10、剩1且目标关闭，未混入。共享上限仍51有效推进/1周期尾/128调度入口，本批实际全0；今后首次phase5→0尾只可能表示部分区间，仍须在该上限停，不能称完整5→5周期。

停止后的只读音乐审查：原文件与冻结副本均4666101字节、SHA {music['music_files'][0]['sha256']}，文件存在且内容相同；系统已安装afinfo读取出双声道44100Hz MP3、估计145.811656秒，低于自写解码器静态240秒上限，但没有做城市PCM解码试验。保存的原wrapper0x762ccb–0x762de6代码与固定EXE字节完全相等。driver、缓冲区、open调用点与菜单相同，open guard当前唯一差异是新路径。后续还存在独立语义变化：wrapper原caller0x5c3040、slot0、循环参数1，已有菜单loop guard只允许0；volume20匹配。只修改路径将可能在下一loop设置边界停。循环1尚未执行，不能把静态参数当作已验证原MSS循环语义。具体原栈、资源、静态哈希及afinfo原日志见city-music-contract-review.json。

值得继续一个窄范围试验：先审查这首固定哈希城市音乐和原loop1参数，用已存在的自写离线backend检验真实解码、句柄/lifecycle/close及状态持久化，保持source PCM0，不假成功；然后复核新源码pin和精确续行范围，从frame55a未完成导入继续。此前用户原授权要求遇未知行为停，本批已遵守；不能仅靠剩97次数开放任意新路径或API。可复核的后续工作清单见stage55-next-music-review-plan.json，本轮未修改已锁定适配器、未重开目标、未运行额外原指令。

最终checkpoint frame55a（三文件SHA/大小见结果JSON）保留真实blocked-import状态，未另做冷恢复；已确认能够冷恢复的是returnwatch51，caller55实际作为第3次来源已恢复。{audit['hash_checks']}项哈希/{audit['unique_paths']}独立路径复核全不变，含原EXE、冻结源/存档/档案、旧代码/检查点、529项当前源码/库pin及两次新检查点；缺失项和DOS链接全不变。三个结果source_frames0，后台窗口非key/非active/激活请求0、frontmost是其他进程。没有源写入、原DLL执行、原代码补丁、字段注入、全局输入/光标、上传、依赖安装、宿主设置改变、提交或发布。

交付：evidence/stage55-authorized-result.json、stage55-authorized-postflight.json、stage54-caller55.json、stage54-frame55a.json、stage55-city-music-contract-review.json、stage55-next-music-review-plan.json、各run*.result.json/stdout/stderr。此前stage55-result和approval-blocked报告为历史状态，当前以本authorized-result为准。剩余不确定性包括城市绘制与语义内容、城市调度和计时、音频原MSS等价性、对象图与其他平台API，以及新原函数AOT到arm64的行为对照；本批没有把菜单/屏幕编号变化算作逻辑通过。
'''
    (ROOT / 'reports/stage55-authorized-caller-success-city-music-boundary.md').write_text(report)
    print(json.dumps(dict(cold_restore_passed=True, caller_return_proven=True,
                          used=3, remaining_unused=97, goal_closed=True,
                          city_frame=False, scheduler_entries=0,
                          blocker=frame['result']['boundary']['kind'],
                          music_loop_argument=1, hash_checks=audit['hash_checks'],
                          unique_paths=audit['unique_paths'], all_inputs_unchanged=True),
                     ensure_ascii=False))


if __name__ == '__main__':
    main()
