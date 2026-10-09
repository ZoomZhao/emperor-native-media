"""Only summarize own keyboard tests and verify all existing frozen evidence."""
import hashlib,json,os
from datetime import datetime,timezone
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
EV=ROOT/'evidence'
EXPECTED_LEDGER='61d81a93ce89a0c8b79bec344f31960d34362dc52e2b23f4c8e7af3bd85c7353'


def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as f:
        for b in iter(lambda:f.read(1048576),b''):h.update(b)
    return h.hexdigest()


def main():
    assert sha(EV/'stage55-batch-ledger.json')==EXPECTED_LEDGER
    ledger=json.loads((EV/'stage55-batch-ledger.json').read_text())
    assert len(ledger['executions'])==4 and ledger['remaining_processes']==96 and ledger['goal_closed']
    watch=ledger['watch_state']
    assert watch['advances']==[dict(before=5,after=6)] and watch['entries']==1 and watch['tail_entries']==0
    create=json.loads((EV/'stage59-probe-create.json').read_text())
    restore=json.loads((EV/'stage59-probe-restore.json').read_text())
    assert create['passed'] and restore['passed']
    assert create['case_count']==26 and restore['case_count']==3
    assert all(q['passed'] for q in create['cases']+restore['cases'])
    for q in (create,restore):
        assert q['original_instructions_executed']==0 and not q['original_DLL_executed']
        assert not q['result']['original_API_unblocked']
        diag=q['result']['native_diagnostic']
        assert diag['app_active']==diag['key_window']==diag['can_become_key']==0
        assert diag['pid']!=diag['frontmost_pid']
    assert restore['result']['native_diagnostic']['delivered']==0
    base=json.loads((EV/'stage58-postflight.json').read_text())
    checks=[];cache={}
    for q in base['checks']:
        p=q['path']
        if p not in cache:cache[p]=sha(p)
        checks.append(dict(path=p,sha256=cache[p],expected_sha256=q['expected_sha256'],unchanged=cache[p]==q['expected_sha256']))
    absences=[dict(path=q['path'],still_missing=not Path(q['path']).exists()) for q in base['absences']]
    links=[dict(path=q['path'],target=os.readlink(q['path']),unchanged=os.readlink(q['path'])==q['target']) for q in base['DOS_namespace_symlinks']]
    assert all(q['unchanged'] for q in checks+links) and all(q['still_missing'] for q in absences)
    assert sha(EV/'stage51-batch-ledger.json')==ledger['old_ledger_sha256']
    (EV/'stage59-postflight.json').write_text(json.dumps(dict(hash_checks=len(checks),checks=checks,
         absences=absences,DOS_namespace_symlinks=links,all_protected_inputs_unchanged=True,
         original_ledger_unchanged=True,original_ledger_sha256=EXPECTED_LEDGER),indent=2)+'\n')
    build=json.loads((EV/'stage59-build.json').read_text())
    assert sha(build['library'])==build['sha256']
    assert sha(ROOT/'samples/stage59_keys.m')==build['source_sha256']
    native=(ROOT/'samples/stage59_keys.m').read_text()
    for forbidden in ['CGEventSource','CGEventTap','addGlobalMonitor','addLocalMonitor','activateIgnoringOtherApps','makeKeyWindow','interpretKeyEvents','[event characters']:
        assert forbidden not in native,forbidden
    bounded=[]
    for name in ['stage59-build-own-native-keyboard','stage59-own-AppKit-keyboard-fixtures',
                 'stage59-own-AppKit-keyboard-fixtures-v2','stage59-own-independent-keyboard-restore']:
        q=json.loads((EV/(name+'.result.json')).read_text())
        assert not q['timed_out'] and q['duration_seconds']<60
        bounded.append(dict(label=name,**q))
    concurrency=next(q for q in create['cases'] if q['name'].startswith('eight concurrent'))
    inputs={str(p.relative_to(ROOT)):sha(p) for p in [ROOT/'samples/stage59_keys.m',
              ROOT/'samples/stage59_keys.py',ROOT/'samples/stage59_keys_probe.py',
              ROOT/'samples/build_stage59_keys.py',ROOT/'samples/stage59_finalize.py']}
    result=dict(created_utc=datetime.now(timezone.utc).isoformat(),
        status='own_event_state_tests_passed_original_async_scope_still_blocked',
        documentation_sources=dict(Microsoft_GetAsyncKeyState='https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getasynckeystate',
             Microsoft_virtual_keys='https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes',
             Apple_key_event_delivery='https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/EventOverview/HandlingKeyEvents/HandlingKeyEvents.html',
             independent_hardware_key_mapping='<LOCAL_MACHINE_PATH>'),
        original_consumer=dict(PC='0x477fb9',predicate='(EAX &0x8001)==0x8001',VKs=[113,116,117],keys=['F2','F5','F6'],
             evidence='evidence/stage58-keyboard-boundary-review.json',disassembly_log_sha256=sha(EV/'stage58-keyboard-consumer-static.stdout.log')),
        implementation=dict(native_library=build['library'],native_library_sha256=build['sha256'],new_source_sha256=inputs,
             no_original_caller_address_filters=True,function_key_support='F1..F20; F21..F24 explicitly unsupported',
             event_source='own task NSWindow.sendEvent to actual NSView first responder',
             state_scope='levels as of last delivered owned event; not current global hardware state',
             query_latch='shared per-key pending edge, NSLock atomic consumption across host threads',
             repeat_policy='no pending edge for repeat; implementation policy, not full Windows repeat behavior proven',
             snapshot_bytes=48,original_runtime_integration_performed=False,
             original_query_scope='background/inactive nonkey cannot observe session hardware; -20 refuses, leaves guest regs/latch unchanged'),
        semantic_cases=29,passed_cases=29,failed_semantic_cases=0,
        create_cases=26,independent_restore_cases=3,
        native_fixture_events_created=create['result']['native_diagnostic']['fixture_events'],
        native_events_delivered=create['result']['native_diagnostic']['delivered'],
        native_function_events_accepted=create['result']['native_diagnostic']['accepted_events'],
        foreign_window_event_rejected=True,non_function_key_ignored_without_text_read=True,
        concurrent_query_values=concurrency['values'],
        snapshot_SHA256=create['result']['snapshot_sha256'],
        independent_restore_posted_events=0,
        restore_contract=dict(replay='exact event history: consumed latch remains consumed; same pending edge restored once',
             live='invalidate known/down/pending; unknown refuses, repeat only learns down level; new key-down requires new genuine delivered transition'),
        guarantees=dict(original_system_async_input_equivalence=False,original_API_unblocked=False,
             physical_function_keys_or_other_app_capture_tested=False,Windows_cross_process_latch_competition_reproduced=False,
             event_reception_while_background='synthetic events sent to own window are delivered; no claim to users physical keys in another app',
             current_hardware_state_observable=False,no_constant_zero_fallback=True),
        initial_failure=dict(label='stage59-own-AppKit-keyboard-fixtures',reason='Apple SDK header legacy copyright byte was not UTF-8; failed before window construction',
             corrected='read SDK header as latin1 preserving independent ASCII key names/constants',logs_retained=True),
        whole_process_results=bounded,original_instructions_executed=0,original_DLL_executed=False,
        new_TCC_permissions_requested=False,global_or_local_keyboard_monitor=False,event_tap=False,
        global_hardware_key_query=False,user_text_read_or_recorded=False,window_activated_or_made_key=False,
        source_PCM_sent_to_device=False,files_uploaded=False,system_settings_changed=False,
        dependencies_installed=False,committed_or_published=False,
        protected_hash_checks=len(checks),all_protected_inputs_unchanged=True,
        original_ledger_sha256=EXPECTED_LEDGER,original_ledger_unchanged=True,
        original_batch=dict(used=4,remaining=96,closed=True,phase=6,productive_advances=1,scheduler_entries=1,tail_entries=0,caps=ledger['caps']),
        continuation_recommendation='Do not reopen96 on these local event tests alone. Need a reviewed input-source contract that can supply current asynchronous key state and consumed low bit under allowed focus/privacy rules. Keep frame56a and all shared counts. No new original instructions in this stage.')
    (EV/'stage59-result.json').write_text(json.dumps(result,indent=2)+'\n')
    report=f'''键盘语义和自写事件模型已完成，29个合同/恢复测试通过（create26、独立restore3），原GetAsyncKeyState阻塞仍在。保留同批used4/remaining96/closed、phase6、推进1/51、入口1/128、尾0/1；本阶段原指令/原DLL执行均0，没有借用关闭余额。静态固定EN消费者477fb9要求(EAX &0x8001)==0x8001，其后同类F5/F6也用该谓词，证据沿用stage58。

微软合同及准确差距见[contract](stage59-keyboard-contract.md)：高位是查询时当前按下；低位表示此前按过，因跨线程/其他应用竞争可能漏报，不能保证每个消费者都得到。实现的是任务进程共享的原子latch，八个并发查询实际值{concurrency['values']}：只有一个取得8001，其余8000。没有模拟其他Windows进程竞争，也未把这一确定性实现宣称为全系统GetAsyncKeyState精确等价。[Microsoft GetAsyncKeyState](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getasynckeystate)。

实际AppKit路径：独立NSPanel不可成为key或main、inactive且非前台，仅向它sendEvent自造无文本NSEvent；真实NSView first responder处理77个已派发事件，其中75个函数键事件、1个不同窗口事件拒绝、1个非函数keyCode忽略。20个F1..F20映射测试的期望hardware keyCode独立读取Apple SDK Events.h，非对着实现表自证；F2/F5/F6没有分地址补丁。首次按下8001→再次查询8000，release后0、快速press/release后1→0、重复不重生edge、多键隔离、错误snapshot不改活状态、未知键/状态拒绝全部通过。

48字节冻结replay恢复确实逐字节相同；已消费F2仍8000，未消费F5仅第一次8001、第二次8000，不新增事件。独立进程fixture_events和实际delivered均0；其中accepted_events75是随快照恢复的历史累计值，不是新按键。live恢复不能回放宿主物理输入，清known/down/pending为unknown，查询拒绝而非0；随后repeat仅建立held、不设按过位，新真实release/down才产生新edge。repeat策略是本地模型选择，微软原文未给完整repeat细节，不宣称原Windowsrepeat对照通过。

关键保证边界：本地模型维护“最后一次已交付任务事件的level”，不能由非key/非激活窗口推知别的应用中的当前物理按键。普通AppKit事件经key window投递；自造sendEvent通过只证明处理链，不能证明后台可监听用户硬件。[Apple Handling Key Events](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/EventOverview/HandlingKeyEvents/HandlingKeyEvents.html)。原生产query明确status-20，不填写0，未消费本地pending；通用stdcall adapter在四个任意caller fixture上保留EAX/SP/PC原值。当前约束下仍不能解除原477fb9边界；无新的原runtime接入、原首帧或周期进展。

故恢复建议是继续保持96关闭。先核实一个在允许输入/焦点范围内能回答当前异步状态和低位消耗的来源合同；当前仅窗口事件域不足，不能默认无键按下，更不能用0伪造通过。后续如满足合同并获继续，必须保留frame56a、phase6及同一已用4次历史/共享上限，不能重载或重置。

首轮自测在Apple SDK头copyright非UTF8字节读取失败，窗口尚未创建；改为latin1读取ASCII常量后通过，失败日志保留。可信Apple clang构建1.009秒；create复测1.452秒、独立restore0.139秒，均无超时。仅新任务内源码/库/日志/48字节自造状态，无原EXE/DLL/安装器运行，无TCC申请、global/local monitor/event tap/CG键查询、用户文本读取、激活/makeKey、音频播放、上传、安装、宿主设置、提交/发布。原受保护3482项哈希以及缺失18/链接2全不变；原账本SHA仍`{EXPECTED_LEDGER}`。

交付：evidence/stage59-result.json、stage59-probe-create.json、stage59-probe-restore.json、stage59-postflight.json、stage59-build.json和各bounded stdout/stderr/result。源码samples/stage59_keys.m、stage59_keys.py、stage59_keys_probe.py；没有改旧pinned实现和原checkpoint格式。
'''
    (ROOT/'reports/stage59-keyboard-tests-and-observability-boundary.md').write_text(report)
    print(json.dumps(dict(status=result['status'],passed=29,original_instructions=0,used=4,remaining=96,
                          protected_hashes=len(checks),original_API_unblocked=False)))


if __name__=='__main__':main()
