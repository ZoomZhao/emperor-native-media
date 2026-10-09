# -*- coding: utf-8 -*-
"""Finalize scoped AppKit-only diagnosis; never imports original runtime."""
import json,os
from pathlib import Path
from stage54_batch import LEDGER,APPROVAL,sha,validate
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
EXPECTED='4e499d8560cc4c7a9bc4a2e725e27f502fa13979de6366e9f66bd06f237edcc9'
def read_records(label):return [json.loads(line) for line in (E/(label+'.stdout.log')).read_text().splitlines()]
def crash_summary(path,pid):
    s=Path(path).read_text();head,body=s.split('\n',1);h=json.loads(head);c=json.loads(body)
    assert c['pid']==pid and c['exception']['signal']=='SIGABRT'
    t=c['threads'][c['faultingThread']]
    return dict(path=str(path),sha256=sha(path),timestamp=h.get('timestamp'),
        context={k:c.get(k) for k in ['pid','userID','parentProc','parentPid','responsibleProc','responsiblePid','coalitionName','procPath','exception','termination','asi','faultingThread']},
        thread={k:t.get(k) for k in ['queue','triggered']},
        stack=[dict(symbol=q.get('symbol'),image=c['usedImages'][q['imageIndex']].get('name')) for q in t['frames'][:18]])
def main():
    assert sha(LEDGER)==EXPECTED
    l=json.loads(LEDGER.read_text());validate(l)
    assert l['goal_closed'] and len(l['executions'])==20 and l['remaining_processes']==80 and l['latest_checkpoint']=='cycle62p19'
    protected=json.loads((E/'stage64-postflight.json').read_text())
    for q in protected['checks']:assert sha(q['path'])==q['expected_sha256'],q['path']
    for p,h in l['pinned_files'].items():assert sha(p)==h,p
    for f in l['executions'][18]['checkpoint_files']:assert sha(ROOT/f['path'])==f['sha256']
    own=read_records('stage65-own-mainthread-appkit-repro')
    direct=read_records('stage65-own-raw-native-appkit');python=read_records('stage65-own-raw-python-appkit')
    session=read_records('stage65-python-own-session-readonly')[0]
    assert direct[0]['NSThread_isMainThread'] and direct[1]['created'] and direct[1]['windows']==0 and not direct[1]['active']
    assert python[-1]['kind']=='BEFORE_RAW_NSAPPLICATION' and python[-1]['NSThread_isMainThread'] and python[-1]['pthread_main_np']==1
    assert session['status']==0 and session['graphic_access'] and not session['remote'] and session['session']==100002
    labels=['stage65-own-mainthread-appkit-repro','stage65-own-raw-native-appkit','stage65-own-raw-python-appkit','stage65-python-own-session-readonly','stage65-task-pid-log-read']
    results={n:json.loads((E/(n+'.result.json')).read_text()) for n in labels}
    assert results['stage65-own-raw-native-appkit']['returncode']==0 and results['stage65-own-raw-python-appkit']['returncode']==-6
    assert (E/'stage65-task-pid-log-read.stderr.log').read_text().strip()=='log: Cannot run while sandboxed'
    original=crash_summary('<LOCAL_MACHINE_PATH>',94111)
    mini=crash_summary('<LOCAL_MACHINE_PATH>',95743)
    for c in [original,mini]:
        assert c['thread']['queue']=='com.apple.main-thread'
        assert any(q['symbol']=='___RegisterApplication_block_invoke' for q in c['stack'])
        try:os.kill(c['context']['pid'],0)
        except ProcessLookupError:pass
        else:raise AssertionError('task crashed process still exists')
    (E/'stage65-scoped-crash-context.json').write_text(json.dumps(dict(original20=original,minimal_python=mini),indent=2)+'\n')
    artifacts={str(p):sha(p) for p in [ROOT/'samples/stage65_appkit_probe.m',ROOT/'samples/stage65_appkit_raw.m',ROOT/'samples/stage65_ctypes_probe.py',ROOT/'samples/stage65_python_session.py',ROOT/'runs/stage65-appkit-probe',ROOT/'runs/stage65-appkit-raw',ROOT/'runs/stage65-appkit-raw.dylib']}
    (E/'stage65-own-diagnostic-artifacts.json').write_text(json.dumps(dict(status='OWN_DIAGNOSTIC_ONLY_NOT_ORIGINAL_EXECUTION_APPROVAL',artifacts=artifacts,no_original_source_loaded=True),indent=2)+'\n')
    prev=json.loads((E/'stage54-cycle62p18.json').read_text())['native_diagnostic']
    result=dict(status='isolated_python_AppKit_registration_abort_original_still_stopped',
        original20_and_minimal_same_mainthread_RegisterApplication_abort=True,
        original_guest_executed=0,original_runtime_constructed=False,original_DLL_loaded=False,
        current_UID=session['uid'],current_EUID=session['euid'],current_security_session=session,
        prior_success_saved_command='/usr/bin/python3 samples/checkpoint_startup62.py',
        original20_saved_command='/usr/bin/python3 samples/checkpoint_startup63.py',
        prior_success_main_thread=prev['main_thread'],prior_success_active=prev['NSApp_isActive'],prior_success_activation_requests=prev['activation_requests'],
        prior_success_UID_security_session_sandbox_profile_not_recorded=True,
        native_raw_construction_passed=True,native_raw_full_window_or_game_not_proven=True,
        Python_raw_SIGABRT_reproduced=True,Python_raw_main_thread=True,
        interpreter_or_launch_context_specific_difference_demonstrated=True,
        specific_Python_bundle_or_sandbox_registration_cause_not_proven=True,
        persistent_current_Python_path_failure_observed=True,transient_only_hypothesis_not_supported_by_current_repeat=True,
        low_level_PID_scoped_log_read_denied=True,log_denial_reason='Cannot run while sandboxed',
        permission_or_path_workaround_attempted=False,automatic_approval_rejection=False,
        verified_code_fix_available=False,
        minimum_user_action='In the already logged-in desktop Terminal, run only the own no-game ctypes probe with the same interpreter and hard60 wrapper; do not run original game or restart system services.',
        no_runtime_source_pins_or_authorization_changed=True,ledger_unchanged=True,ledger_sha256=EXPECTED,
        used=20,remaining=80,closed=True,latest_checkpoint='cycle62p19',phase=22,advances=17,entries=17,tail=0,
        no_run21=True,cold_restore63_not_proven=True,guest8_budget_not_proven=True,
        protected_hash_checks=3482,hash_rows_rechecked=len(protected['checks']),bound_pins_rechecked=len(l['pinned_files']),
        no_settings_changes=True,no_installs=True,no_uploads=True,no_system_service_killed=True,
        probes=results,diagnostic_native_records=own,direct_raw_records=direct,python_raw_records=python)
    (E/'stage65-result.json').write_text(json.dumps(result,indent=2)+'\n')
    post=dict(all_passed=True,ledger_sha256=sha(LEDGER),approval_sha256=sha(APPROVAL),ledger_unchanged=True,
        protected_hash_checks=3482,hash_rows_rechecked=len(protected['checks']),bound_pins_rechecked=len(l['pinned_files']),
        checkpoint_three_hashes_unchanged=True,original20used80remaining=True,no_run21=True)
    (E/'stage65-postflight.json').write_text(json.dumps(post,indent=2)+'\n')
    report='''已把Stage64故障缩小到当前Apple Python启动路径下的AppKit应用注册：同一自写构造函数直接作为原生可执行文件运行成功，经现有/usr/bin/python3的ctypes调用则重现相同SIGABRT。游戏、Unicorn、旧适配器和资源都没有参与最小复现。本轮未启动第21次。

验证结果：带先行SessionGetInfo的原生探针2.420秒正常退出；去掉先行会话查询、保留原适配器相同NSApplication.sharedApplication及setActivationPolicyProhibited前缀的原生探针1.417秒正常退出；同一前缀编译为自写dylib后通过Python ctypes调用0.677秒SIGABRT。直接原生探针没有创建窗口、激活应用或启动事件循环；它的成功只证明NSApplication构造，不证明完整原生窗口/游戏兼容。

主线程证据：第20次及最小Python复现崩溃报告都位于thread0、com.apple.main-thread；最小函数在调用前同时确认NSThread.isMainThread=true和pthread_main_np=1。因此此次崩溃没有退出主线程。两次栈均为sharedApplication → HIServices GetCurrentProcess/RegisterApplication → abort，分别由旧s26_window_new和自写s65_construct调用。第20次PID94111及最小复现PID95743均不存在。

身份/会话：当前Python UID/EUID501，Security SessionGetInfo对调用者返回status0、session100002、graphic_access=true、remote=false、root=false；原生探针同一会话100002。会话HasTTY属性为true，当前进程stdin/stdout不是TTY，二者不能混为一谈。原20次崩溃UID也是501，responsible进程为ChatGPT PID66820。不是仅凭“后台进程”就可断言无图形登录会话；但会话具备graphic标志并不证明所有WindowServer/LaunchServices注册请求已被允许。

与前次成功的已知比较：第18次使用同一个/usr/bin/python3、相同任务cwd和硬60运行器，原生诊断main_thread=1、NSApp_isActive=0、activation_requests=0，59.264秒正常退出；第19次也已经注册并呈现窗口后才硬超时。前次UID、安全会话、完整environment和工具沙箱profile未记录，不能宣称已证实哪一个环境参数变化。当前最小Python复现仍失败，故“只是一次瞬态失败”不符合当前重复证据。现在能证明的是Python.app/其启动上下文与直接原生入口之间的差异；Python bundle、特定服务连接或沙箱规则哪项导致底层abort，仍未确定。

只查询本任务94111/95743 PID及明确含其PID的日志，/usr/bin/log直接返回“Cannot run while sandboxed”；未升级权限、换工具/路径读取、扫描无关私人日志或杀系统服务。没有自动审批拒绝。此限制使底层注册原因仍无法定论。目前没有验证过的代码修复，也不建议把直接原生探针成功当作换入口即可修复原游戏。

最小用户动作：在当前已登录桌面的Terminal，仅运行下面的自写无游戏探针；不需要更改安全设置，不执行EXE，不消耗原100次预算。其正常/异常退出日志可用于比较当前工具启动上下文和桌面Terminal启动上下文；执行结果出来前不能称为修复。

    cd <LOCAL_LEGACY_TASK>
    /usr/bin/python3 samples/run_bounded.py stage65-user-desktop-ctypes-probe . /usr/bin/python3 samples/stage65_ctypes_probe.py

原账本SHA4e499d8560cc4c7a9bc4a2e725e27f502fa13979de6366e9f66bd06f237edcc9不变，关闭20used/80余；cycle62p19三文件、phase22、17/51推进、17/128入口、0/1尾、输入设备保存体保持。3482保护hash与550绑定核对通过。新wrapper的完整冷恢复、8秒guest及60秒正常退出仍未验证；没有原执行、原DLL、安装、权限/宿主设置改变、上传、提交或发布。

证据：evidence/stage65-result.json、stage65-scoped-crash-context.json、stage65-postflight.json，以及stage65-own-raw-native-appkit/stage65-own-raw-python-appkit的stdout/stderr/result日志。自写探针源码在samples/stage65_appkit_raw.m及stage65_ctypes_probe.py。
'''
    (ROOT/'reports/stage65-AppKit-registration-diagnosis.md').write_text(report)
    print(json.dumps({k:result[k] for k in ['status','used','remaining','closed','current_UID','native_raw_construction_passed','Python_raw_SIGABRT_reproduced','low_level_PID_scoped_log_read_denied','ledger_unchanged','no_run21']}))
if __name__=='__main__':main()
