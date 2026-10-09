"""Readonly postflight and bounded-city evidence summary; no emulator imports."""
import hashlib,json,os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
def sha(p):
    h=hashlib.sha256()
    with Path(p).open('rb') as f:
        for b in iter(lambda:f.read(1048576),b''):h.update(b)
    return h.hexdigest()
def main():
    ledger=json.loads((E/'stage55-batch-ledger.json').read_text())
    assert all(q['status']=='finished' for q in ledger['executions'])
    prior=json.loads((E/'stage60-postflight.json').read_text());cache={};checks=[]
    for q in prior['checks']:
        p=q['path']
        if p not in cache:cache[p]=sha(p)
        checks.append(dict(path=p,expected_sha256=q['expected_sha256'],sha256=cache[p],unchanged=cache[p]==q['expected_sha256']))
    assert all(q['unchanged'] for q in checks)
    for p,h in ledger['pinned_files'].items():assert sha(p)==h,p
    baseline=json.loads((E/'stage59-postflight.json').read_text())
    assert all(not Path(q['path']).exists() for q in baseline['absences'])
    assert all(os.readlink(q['path'])==q['target'] for q in baseline['DOS_namespace_symlinks'])
    runs=[];previous_device=None;exact_restores=0
    for q in ledger['executions'][4:]:
        tag=q['tag'];outpath=E/f'stage54-{tag}.json'
        out=json.loads(outpath.read_text()) if outpath.exists() else json.loads((E/f'stage54-{tag}-failure.json').read_text())
        inp=E/f'stage61-{tag}-input-exit.json';input_out=json.loads(inp.read_text()) if inp.exists() else None
        entries=E/f'stage61-{tag}-input-entry.json';entry=json.loads(entries.read_text()) if entries.exists() else None
        log=input_out['device']['payload']['log'] if input_out else []
        calls=[r['payload'] for r in log if r['payload']['kind']=='GUEST_QUERY']
        if input_out:
            assert sum(r['payload']['kind']=='DEVICE_ATTACH' for r in log)==1
            assert input_out['meta']['host_keyboard_queries']==0
            assert input_out['meta']['restore_count']==q['index']-5
            assert all(r['result']==0 and r['down']==r['pending_before']==r['pending_after']==0 for r in calls)
            if previous_device is not None:
                assert entry['kind']=='EXACT_DEVICE_RESTORE' and not entry['new_DEVICE_ATTACH']
                assert entry['saved_device_sha256']==previous_device['sha256']
                assert entry['saved_log_head']==previous_device['payload']['head']
                old_log=previous_device['payload']['log'];assert log[:len(old_log)]==old_log
                exact_restores+=1
            previous_device=input_out['device']
        audio=out.get('native_audio');diag=out.get('native_diagnostic')
        if audio:assert audio['source_frames']==0
        if diag:assert not(diag['key_window'] or diag['NSApp_isActive'] or diag['activation_requests'])
        label='stage61-run05-city-frame' if q['index']==5 else f"stage61-run{q['index']:02d}-finite-cycle"
        duration=json.loads((E/f'{label}.result.json').read_text())
        assert duration['duration_seconds']<=60 and not duration['timed_out']
        for f in q['checkpoint_files']:assert sha(ROOT/f['path'])==f['sha256']
        runs.append(dict(index=q['index'],source=q['resume'],checkpoint=q['checkpoint'],
            boundary=q['result'].get('boundary'),status=q['result']['status'],
            whole_seconds=duration['duration_seconds'],original_execution_calls=out.get('emu_start_calls'),
            natural=out.get('natural_state'),source_PCM_frames=audio['source_frames'] if audio else None,
            no_focus_change=not(diag['key_window'] or diag['NSApp_isActive'] or diag['activation_requests']) if diag else None,
            input_entry_kind=entry['kind'] if entry else None,
            device_query_count_total=len(calls),device_queries_by_vk={str(k):sum(c['vk']==k for c in calls) for k in sorted(set(c['vk'] for c in calls))},
            input_meta=input_out['meta'] if input_out else None,device_log_head=input_out['device']['payload']['head'] if input_out else None,
            actual_original_segment_ceiling_seconds=10,
            actual_runtime_evidence=str(outpath.relative_to(ROOT))))
    w=ledger['watch_state']
    driver_path=E/'stage61-cycle-driver-result.json'
    driver=json.loads(driver_path.read_text()) if driver_path.exists() else {}
    result=dict(status='city_first_frame_and_bounded_reference_continuation',
        city_frame_passed=bool(ledger['city_frame_review']),original_native_AOT_proven=False,
        physical_keyboard_compatibility_tested=False,controlled_no_key_input=True,
        input_domain='935b7dbc8d4775c10fddda87940098035658575c667d80d81b925b9d7c501629',
        used=len(ledger['executions']),remaining=ledger['remaining_processes'],maximum=100,
        closed=ledger['goal_closed'],stop_reason=ledger.get('stop_reason'),latest_checkpoint=ledger['latest_checkpoint'],
        handback_reason=driver.get('reason','substantive city/reference result'),
        actual_exact_device_restores_verified=exact_restores,single_DEVICE_ATTACH_preserved=True,
        device_log_prefixes_preserved=True,
        phase=w['phase'],productive_advances=len(w['advances']),productive_advance_cap=51,
        scheduler_entries=w['entries'],scheduler_entry_cap=128,tail_entries=w['tail_entries'],tail_returns=w['tail_returns'],
        tail_entry_cap=1,full_phase_cycle=w['full_phase_cycle'],refresh_returns=w['refresh_returns'],
        original_batch_history_preserved=True,counters_reset=False,original_ledger_sha256=sha(E/'stage55-batch-ledger.json'),
        source_read_only=True,original_DLL_executed=False,source_PCM_to_device=False,
        global_keyboard_listener=False,focus_requested=False,files_uploaded=False,host_settings_changed=False,
        dependencies_installed=False,committed_or_published=False,
        protected_hash_checks=len(checks),all_protected_inputs_unchanged=True,pinned_files_checked=len(ledger['pinned_files']),
        absences_checked=len(baseline['absences']),links_checked=len(baseline['DOS_namespace_symlinks']),runs=runs)
    (E/'stage61-postflight.json').write_text(json.dumps(dict(checks=checks,all_passed=True,
        protected_hash_checks=len(checks),pinned_files_checked=len(ledger['pinned_files']),
        absences_unchanged=True,links_unchanged=True),indent=2)+'\n')
    (E/'stage61-result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
    table='| 批次序号 | 整进程秒数 | 阶段 | 累计键查询 | 结果 |\n|---:|---:|---:|---:|---|\n'
    for row in runs:
        phase=row['natural']['phase'] if row['natural'] else '未知'
        boundary=row['boundary']['kind'] if row['boundary'] else row['status']
        table+=f"| {row['index']} | {row['whole_seconds']:.3f} | {phase} | {row['device_query_count_total']} | {boundary} |\n"
    report=f'''受控无按键条件下的城市首帧：{'通过' if result['city_frame_passed'] else '未通过'}。这次验证执行固定 EmperorEN.exe 的原指令，由 Unicorn 2.1.4 提供参考执行，使用已有 macOS 原生适配；完整原生 AOT 游戏与真实物理键盘兼容仍未证明。

在原 frame56a 的 F2 查询返回前，完整旧内存、CPU、四线程及原生状态恢复后显式接入新设备。原物理键状态保留为 unknown；旧字节、历史、队列没有清除，没有注入释放。设备空时间线使所有键松开；原 F2/F5/F6 实际查询分别由同一设备计算返回0。后续进程恢复设备完整状态和哈希链日志，未再次 DEVICE_ATTACH。键盘消息不轮询宿主键盘事件，任何鼠标消息修饰键由同域投影，物理 Alt 查询入口已禁用。

首帧是原刷新536b20返回536fe7前产生的新 primary Blt（返回52c04a），与保存基线不同，GPU读回零字节差异；本地同时检查原primary图和自有后台窗口截图，看到城市地形、道路、建筑、车队、人口侧栏和小地图，顶部人口28/金钱1633与原自然状态相符。证据：stage54-frame61a.json、stage61-city-frame-review.json、stage54-frame61a-present-1.png、stage54-frame61a-owned-city-candidate.png。

累计已用{result['used']}/100，剩余{result['remaining']}；账本关闭={result['closed']}，停止原因={result['stop_reason']}。原调度推进{result['productive_advances']}/51、入口{result['scheduler_entries']}/128、尾入口{result['tail_entries']}/1、尾返回{result['tail_returns']}，当前phase{result['phase']}，刷新返回{result['refresh_returns']}。full_phase_cycle={result['full_phase_cycle']}；尾上限仅允许一次，从初始phase5到首次wrap并不等于完整51阶段周期，不做完整周期通过声明。所有原过程都计入旧账本，未重置。

本轮因“{result['handback_reason']}”交回。实际独立进程精确恢复设备{exact_restores}次，保存设备哈希与前一进程退出状态一致，旧日志前缀逐条相等，始终只有一次DEVICE_ATTACH。设备持久化字段在新检查点host.keyboard_snapshot61中，检查点三文件哈希仍由原账本绑定。完整逐项记录见evidence/stage61-result.json。

{table}

剩余不确定性：输入可重复只覆盖明确的新测试域之后；既有历史输入未被重写，宿主时间/音频时钟仍live。真实键盘、完整Win32输入域、原Miles解码混音等价和完整原生AOT尚未证明。遇到精确新未实现契约仍按原限制停止；普通自写接入检查不消耗原批次预算。3482个旧保护哈希及{len(ledger['pinned_files'])}个运行绑定、{len(baseline['absences'])}个缺失路径和2个DOS链接全部未变；所有已完成原进程≤60秒，sourcePCM设备帧0，没有抢焦点、全局键盘监听、原DLL执行、上传、依赖安装、宿主设置变更、提交或发布。
'''
    (ROOT/'reports/stage61-controlled-input-city-reference.md').write_text(report)
    print(json.dumps({k:v for k,v in result.items() if k!='runs'}))
if __name__=='__main__':main()
