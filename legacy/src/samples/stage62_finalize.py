"""Final immutable-input, device-chain and hard-timeout evidence audit."""
import copy,datetime,hashlib,json,os
from pathlib import Path
from stage54_batch import sha,validate
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
def main():
    ledger=json.loads((E/'stage55-batch-ledger.json').read_text());validate(ledger)
    assert len(ledger['executions'])==19 and ledger['remaining_processes']==81 and ledger['goal_closed']
    assert all(q['status']=='finished' for q in ledger['executions']) and ledger['executions'][-1]['result']['boundary']['kind']=='Whole_process_hard_timeout62'
    old=json.loads((E/'stage62-ledger-before-continuation.json').read_text())
    assert ledger['executions'][:15]==old['executions'] and ledger['watch_state']['advances'][:12]==old['watch_state']['advances']
    protected=json.loads((E/'stage61-postflight.json').read_text());checks=[];cache={}
    for q in protected['checks']:
        p=q['path']
        if p not in cache:cache[p]=sha(p)
        checks.append(dict(path=p,expected_sha256=q['expected_sha256'],sha256=cache[p],unchanged=cache[p]==q['expected_sha256']))
    assert all(q['unchanged'] for q in checks)
    for p,h in ledger['pinned_files'].items():assert sha(p)==h,p
    baseline=json.loads((E/'stage59-postflight.json').read_text())
    assert all(not Path(q['path']).exists() for q in baseline['absences'])
    assert all(os.readlink(q['path'])==q['target'] for q in baseline['DOS_namespace_symlinks'])
    previous=json.loads((E/'stage61-cycle61p15-input-exit.json').read_text())['device'];rows=[]
    for q in ledger['executions'][15:]:
        tag=q['tag'];out=json.loads((E/f'stage54-{tag}.json').read_text())
        entry=json.loads((E/f'stage61-{tag}-input-entry.json').read_text());exit=json.loads((E/f'stage62-{tag}-input-exit.json').read_text())
        device=exit['device'];log=device['payload']['log'];old_log=previous['payload']['log']
        assert entry['kind']=='EXACT_DEVICE_RESTORE' and not entry['new_DEVICE_ATTACH']
        assert entry['saved_device_sha256']==previous['sha256'] and entry['saved_log_head']==previous['payload']['head']
        assert log[:len(old_log)]==old_log and sum(r['payload']['kind']=='DEVICE_ATTACH' for r in log)==1
        assert device['payload']['domain']==previous['payload']['domain']
        assert exit['meta']['restore_count']==q['index']-5 and exit['meta']['host_keyboard_queries']==0
        assert out['native_audio']['source_frames']==0 and not out['game_fields_injected'] and not out['original_code_patched']
        d=out['native_diagnostic'];assert not(d['key_window'] or d['NSApp_isActive'] or d['activation_requests'])
        for f in q['checkpoint_files']:assert sha(ROOT/f['path'])==f['sha256']
        timing=json.loads((E/f"stage62-run{q['index']:02d}-finite-cycle.result.json").read_text())
        if q['index']<19:assert not timing['timed_out'] and timing['duration_seconds']<60
        else:assert timing['timed_out']
        claim=datetime.datetime.fromisoformat(q['claimed_utc']).timestamp()
        names=[f'stage61-{tag}-input-entry.json',f'stage62-{tag}-input-exit.json',f'stage54-{tag}.json',f'stage54-{tag}-after-dispose.json']
        rows.append(dict(index=q['index'],source=q['resume'],checkpoint=q['checkpoint'],natural=out['natural_state'],
            runner_whole_seconds=timing['duration_seconds'],hard_timeout=timing['timed_out'],normal_process_exit_proven=not timing['timed_out'],
            actual_inner_run_status=out['result']['status'],terminal_ledger_result=q['result'],
            original_execution_calls=out['emu_start_calls'],source_PCM_frames=out['native_audio']['source_frames'],
            input_restore_exact=True,input_log_prefix_exact=True,device_sha256=device['sha256'],
            input_query_count=sum(r['payload']['kind']=='GUEST_QUERY' for r in log),
            file_completion_times_relative_to_claim={name:round((E/name).stat().st_mtime-claim,3) for name in names}))
        previous=device
    w=ledger['watch_state'];memory=json.loads((E/'stage62-city-memory-audit.json').read_text())
    result=dict(status='stopped_at_whole_process_hard_timeout_with_verified_checkpoint',used=19,remaining=81,maximum=100,
        original_processes_this_turn=4,whole_process_successes=3,whole_process_hard_timeouts=1,
        all_original_processes_under60=False,hard_timeout_runner_seconds=60.076,
        latest_checkpoint='cycle62p19',latest_checkpoint_files=ledger['executions'][-1]['checkpoint_files'],
        ledger_closed=True,ledger_sha256=sha(E/'stage55-batch-ledger.json'),
        stop_reason=ledger['stop_reason'],phase=w['phase'],starting_phase=5,
        cumulative_productive_advances=len(w['advances']),advance_cap=51,new_advances_this_turn=5,
        cumulative_scheduler_entries=w['entries'],entry_cap=128,tail_entries=w['tail_entries'],tail_entry_cap=1,
        tail_returns=w['tail_returns'],tail_reached=False,full_phase_cycle=False,refresh_returns=w['refresh_returns'],
        pending_observer_frames=w['frames'],city_first_frame_prior_pass_preserved=True,
        population_before=28,population_after=34,treasury_before=1633,treasury_after=1633,
        original_data_changed_bytes=memory['original_data_changed_bytes'],changed_small_guest_blocks=memory['changed_small_guest_blocks'],
        checked_small_guest_blocks=memory['common_small_guest_blocks_checked'],
        guest_live_heap_blocks=memory['guest_live_heap_blocks_after'],owned_DirectDraw_objects=memory['owned_DirectDraw_COM_objects_after'],
        allocation_metadata_unchanged=memory['guest_allocation_metadata_unchanged'],semantic_city_entity_graph_decoded=False,
        original_API_boundary_observed=False,precise_population_writer_not_localized=True,
        controlled_input_exact_cross_process_restores_this_turn=4,new_DEVICE_ATTACH=0,device_domain_changed=False,
        last_device_log_head=previous['payload']['head'],counters_reset=False,
        source_PCM_to_device=False,foreground_activation_requested=False,global_keyboard_listener=False,
        source_original_DLL_executed=False,source_EXE_code_unchanged=memory['original_text_equals_fixed_EXE'],
        source_read_only=True,files_uploaded=False,host_settings_changed=False,dependencies_installed=False,committed_or_published=False,
        original_native_AOT_proven=False,physical_keyboard_compatibility_proven=False,
        protected_hash_checks=len(checks),pinned_files_checked=len(ledger['pinned_files']),all_protected_inputs_unchanged=True,
        absence_checks=len(baseline['absences']),DOS_link_checks=len(baseline['DOS_namespace_symlinks']),runs=rows,
        timing_next_step='Return to8-10-second inner slices within existing14 ceiling; reserve full restore/save/cleanup/exit time, review closed timeout state before another original process.')
    (E/'stage62-postflight.json').write_text(json.dumps(dict(all_passed=True,protected_hash_checks=len(checks),
        checks=checks,pinned_files_checked=len(ledger['pinned_files']),latest_checkpoint_hashes_verified=True,
        input_restore_and_logs_exact=True,absence_and_DOS_links_unchanged=True),indent=2)+'\n')
    (E/'stage62-result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
    table='| 原序号 | 整进程秒数 | 当前阶段 | 人口 | 外层结果 |\n|---:|---:|---:|---:|---|\n'
    for r in rows:table+=f"| {r['index']} | {r['runner_whole_seconds']:.3f} | {r['natural']['phase']} | {r['natural']['population']} | {'硬超时' if r['hard_timeout'] else '正常退出'} |\n"
    report=f'''本轮在第19次原进程的全生命周期60秒硬超时处停止，运行器计时60.076秒（包含终止/收尾开销）。没有启动第20次。第19次内层执行结果为wall_budget，完整检查点、窗口关闭观察及账本finish已在外层kill前写出；它不能作为整进程正常退出通过。旧运行证据不改写，账本保留runtime_result_before_process_timeout并补充Whole_process_hard_timeout62外层失败与关闭。三份最新检查点哈希均在kill后核实通过。

累计已用19/100、余81，账本已关闭。阶段5起累计推进17/51、入口17/128、尾入口0/1、尾返回0，当前阶段22，城市刷新返回16次；本轮新增4个原进程、5个阶段推进。首次尾部尚未达到，full_phase_cycle=false。旧15个过程和原12阶段前缀逐项保留，未重新DEVICE_ATTACH或清零计数。

{table}
城市实际状态：人口28→34，金钱1633保持；人口变化在cycle62p17暂停态已出现，现有原scheduler入口/返回观察仍记录28，故没有把变化错误归因于已定位的scheduler分支。只读比较原.data实际10992字节/40页改变，18051个共同小guest heap块中37个内容改变；live heap仍18131块/188931336字节，owned DirectDraw COM对象仍6个且分配元数据不变。完整实体图/每个原对象角色未解码，原EXE.text逐字节等于固定原文件。证据stage62-city-memory-audit.json按地址保存实际字节/哈希，不将heap块计数称为城市实体数。

同一受控键盘域4次跨进程恢复全部通过，日志前缀与前一进程退出快照一致，未产生新DEVICE_ATTACH。累计51次F2/F5/F6查询来自原已声明设备；没有读取真实键盘。原音频sourceFrames0，窗口始终非key/非active，截图只在本地，原DLL未执行，无上传/安装/宿主设置/提交/发布。3482项旧保护哈希、544项运行绑定、18个缺失路径和2个DOS链接不变。

14秒是原已批准分段上限，本轮在不改旧实现的新增wrapper使用该上限。前3个完整进程已接近60秒，最后一次触发硬限；诊断记录显示恢复约26秒、原执行约14秒、保存与结束约18至19秒，余量不足。当前只做只读诊断/账本归档，未在触限后重试。下一次实现应回到8–10秒分段并为完整恢复、保存、cleanup及Python/原生对象退出保留余量；确认关闭超时状态的处理后才能开启下一原进程，不增预算或回滚阶段。固定EXE仍通过Unicorn2.1.4参考执行，完整原生AOT、真实键盘等价、完整周期都未证明。

主要交付：evidence/stage62-result.json、stage62-timeout-reconciliation.json、stage62-city-memory-audit.json、stage62-postflight.json、stage62-run19-finite-cycle.result.json及原stdout/stderr日志。最新checkpoint cycle62p19，三文件哈希见结果JSON，尚未在后续独立进程冷恢复（本轮每个先前来源均在原执行前完整恢复校验）。
'''
    (ROOT/'reports/stage62-city-progression-hard-timeout.md').write_text(report)
    print(json.dumps({k:v for k,v in result.items() if k not in ['runs','latest_checkpoint_files','pending_observer_frames']}))
if __name__=='__main__':main()
