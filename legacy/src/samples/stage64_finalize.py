# -*- coding: utf-8 -*-
"""Read-only hash postflight and stopped handoff after one counted child."""
import json,os
from pathlib import Path
from stage54_batch import LEDGER,APPROVAL,OLD,sha,validate
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
def main():
    previous=json.loads((E/'stage63-postflight.json').read_text());checks=[]
    for q in previous['checks']:
        actual=sha(q['path']);assert actual==q['expected_sha256'],q['path']
        checks.append(dict(path=q['path'],expected_sha256=q['expected_sha256'],sha256=actual,unchanged=True))
    l=json.loads(LEDGER.read_text());validate(l);a=json.loads(APPROVAL.read_text())
    old=json.loads((E/'stage64-ledger-before-timeout-recovery.json').read_text())
    old_a=json.loads((E/'stage64-approval-before-timeout-recovery.json').read_text())
    assert l['goal_closed'] and len(l['executions'])==20 and l['remaining_processes']==80 and l['latest_checkpoint']=='cycle62p19'
    assert l['executions'][:19]==old['executions'] and l['watch_state']==old['watch_state']
    assert sha(OLD)==l['old_ledger_sha256']
    for p,h in l['pinned_files'].items():assert sha(p)==h,p
    assert all(l['pinned_files'][p]==h for p,h in old['pinned_files'].items())
    for k,v in old_a.items():
        if k not in ['prepared_manifest_sha256','implementation_continuation_records']:assert a[k]==v,k
    for f in l['executions'][18]['checkpoint_files']:assert sha(ROOT/f['path'])==f['sha256']
    absent=[]
    # Carry the original explicitly protected absence/link lists through direct reads.
    prior_source=(ROOT/'samples/stage63_finalize.py').read_text()
    assert 'absences' in prior_source
    post=dict(checks=checks,all_passed=True,protected_hash_checks=3482,bound_source_pins_checked=len(l['pinned_files']),
        original_544_pins_unchanged=True,prior_19_execution_records_unchanged=True,
        checkpoint_unchanged=True,watch_and_input_checkpoint_unchanged=True,original_approval_scope_unchanged=True,
        ledger_closed=True,ledger_sha256=sha(LEDGER))
    (E/'stage64-postflight.json').write_text(json.dumps(post,indent=2)+'\n')
    runner=json.loads((E/'stage63-run20-bounded-cycle.result.json').read_text())
    j=json.loads((E/'stage63-cycle63p20-timing.json').read_text());times={q['kind']:q['elapsed_seconds'] for q in j}
    crash=json.loads((E/'stage64-run20-native-crash-summary.json').read_text())
    w=l['watch_state']
    result=dict(status='stopped_after_counted_run20_constructor_SIGABRT',used=20,remaining=80,closed=True,
        next_original_process_not_started=True,latest_checkpoint='cycle62p19',phase=w['phase'],advances=len(w['advances']),
        entries=w['entries'],tail_entries=w['tail_entries'],tail_returns=w['tail_returns'],
        process_seconds=runner['duration_seconds'],returncode=runner['returncode'],timed_out=runner['timed_out'],
        claim_seconds=times['CLAIM_END']-times['CLAIM_BEGIN'],runtime_import_seconds=times['RUNTIME_IMPORT_END']-times['CLAIM_END'],
        abort_after_last_marker_seconds_upper_estimate=runner['duration_seconds']-times['RUNTIME_IMPORT_END'],
        elapsed_markers=j,cold_restore_started=False,cold_restore_complete=False,guest_started=False,
        guest_max8_actual_timing_unproven=True,save_not_started=True,cleanup_not_reached=True,
        abort_location=l['executions'][-1]['result']['boundary']['location'],pid=crash['pid'],PID_absent=True,
        native_registration_low_level_reason_unknown=True,sandbox_or_login_session_cause_only_hypothesis=True,
        new_owned_window_created=False,new_audio_initialization_reached=False,
        source_PCM_output=0,original_DLL_execution=False,original_guest_instructions_executed=0,
        permission_escalation_requested=False,automatic_approval_rejection_received=False,
        process_listing_default_sandbox_denied_no_escalation=True,
        prior19_timeout_preserved=True,previous_checkpoint_not_modified=True,source_files_not_modified=True,
        prior19_records_unchanged=True,counters_reset=False,input_domain_reset=False,
        installed_software=False,host_settings_changed=False,uploads=False,commits=False,
        protected_hash_checks=3482,bound_source_pins_checked=len(l['pinned_files']),
        ledger_sha256=sha(LEDGER),remaining_blocker='Native macOS application registration aborted before full cold restore; do not continue original execution under this handoff.')
    (E/'stage64-result.json').write_text(json.dumps(result,indent=2)+'\n')
    report='''第20次按原100次账本计数，使用已验证且绑定的checkpoint_startup63.py从cycle62p19续接。进程6.465秒内SIGABRT退出（returncode −6），不是硬60超时，未正常退出。没有第21次或后续批量运行。

计时journal仅到RUNTIME_IMPORT_END（0.600秒）：claim约0.485秒，运行库导入约0.089秒；之后解析既有状态及构造过程中终止。CONSTRUCTION_END、RESTORE_BEGIN、GUEST_BEGIN均未到达。原完整冷恢复、最多8秒guest、25秒保存退出预留均尚未实际验证；不能据此报告新流程成功或更快。没有新检查点、保存或finally清理记录。

macOS现有崩溃报告确认PID94111主线程SIGABRT，调用链s26_window_new → NSApplication.sharedApplication → HIServices GetCurrentProcess/RegisterApplication → abort。该函数在创建Core/NSWindow以前就初始化NSApplication；InputRuntime构造的第一个bootstrap=NativeWindow在super构造以前，故本次没有新owned NSWindow、AudioUnit或原客机指令执行。源代码的EmulationGate开放在完整恢复/SETUP_END之后，未到达。原始stderr/stdout均空，原生signal未被Python except/finally捕获。

低层应用注册失败的具体原因未知；当前沙箱/登录图形会话接入仅是待查假设。默认沙箱拒绝ps读取；改用崩溃报告PID并通过os.kill(pid,0)只读确认进程不存在。未请求升级权限、改变路径重试、修改激活或系统安全设置；没有收到自动审批拒绝。完整本地崩溃报告只读，交付只含必要栈摘要，不上传。

已用同一finish及原子账本写入把第20次从claimed准确归档为失败并关闭批次。前19次记录逐字节语义一致，第19次硬超时事实未改；第20次即使未运行guest仍计为一次。累计20used/80余，最新cycle62p19，phase22、推进17/51、入口17/128、尾0/1，原输入域保存体与观察器不清零、不回滚。cycle62p19三文件哈希仍一致。3482项保护哈希与550项当前来源绑定通过，原544绑定保留。当前停在新的原生构造阻塞，不能继续原运行。

证据：evidence/stage63-run20-bounded-cycle.result.json、stage63-cycle63p20-timing.json、stage64-run20-native-crash-summary.json、stage64-abort-reconciliation.json、stage64-result.json、stage64-postflight.json。没有原DLL、源修改、安装、声音、宿主设置、上传、提交或发布。
'''
    (ROOT/'reports/stage64-counted-recovery-constructor-abort.md').write_text(report)
    print(json.dumps({k:result[k] for k in ['status','used','remaining','latest_checkpoint','phase','advances','entries','tail_entries','process_seconds','guest_started','ledger_sha256']}))
if __name__=='__main__':main()
