# -*- coding: utf-8 -*-
"""Scoped own-IPC handoff; no execution gate, game or old adapter imports."""
import json,os,re
from pathlib import Path
from stage54_batch import LEDGER,APPROVAL,sha,validate
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
EXPECTED='4e499d8560cc4c7a9bc4a2e725e27f502fa13979de6366e9f66bd06f237edcc9'
def main():
    assert sha(LEDGER)==EXPECTED
    l=json.loads(LEDGER.read_text());validate(l)
    assert l['goal_closed'] and len(l['executions'])==20 and l['remaining_processes']==80 and l['latest_checkpoint']=='cycle62p19'
    w=l['watch_state'];assert w['phase']==22 and len(w['advances'])==w['entries']==17 and w['tail_entries']==0
    prior=json.loads((E/'stage64-postflight.json').read_text())
    for q in prior['checks']:assert sha(q['path'])==q['expected_sha256'],q['path']
    for p,h in l['pinned_files'].items():assert sha(p)==h,p
    for f in l['executions'][18]['checkpoint_files']:assert sha(ROOT/f['path'])==f['sha256']
    context=json.loads((E/'stage66-context-comparison-v2.json').read_text())
    small=json.loads((E/'stage66-ipc-canvas-result.json').read_text());large=json.loads((E/'stage66-canvas12m-ipc-canvas-result.json').read_text())
    assert context['exact_matrix_match'] and all(q['normal_exit_code']==0 for v in [small,large] for q in v['rows'])
    assert all(q['permission_matrix_after_UI_unchanged'] for q in large['rows'])
    assert large['rows'][1]['complete_defined_state_restored']
    for v in [small,large]:
        for q in v['rows']:
            assert q['window']['visible'] and q['window']['main_thread'] and q['window']['draw_calls']>=1
            assert not q['window']['key'] and not q['window']['main'] and not q['window']['NSApp_active']
            assert q['closed']['closed'] and not q['closed']['window_visible']
            try:os.kill(q['pid'],0)
            except ProcessLookupError:pass
            else:raise AssertionError('own helper remains alive')
    helper=(ROOT/'samples/stage66_helper.m').read_text();client=(ROOT/'samples/stage66_ipc_canvas_test.py').read_text()
    assert all(token not in helper for token in ['sandbox_init(','sandbox_apply(','sandbox_extension_consume(','setuid(','setgid(','activateIgnoringOtherApps','NSEvent.modifierFlags','CGEventSourceKeyState'])
    assert 'start_new_session=False' in client
    files=['stage43_window.m','stage33_dd.m','stage34_dd.m','stage43_input.m','stage43_cursor.m']
    abi={name:sorted(set(re.findall(r'\b(s(?:26|27|28|29|30|31|32|33|34|43c|43)_\w+)\s*\(', (ROOT/'samples'/name).read_text()))) for name in files}
    (E/'stage66-static-ABI-migration-catalog.json').write_text(json.dumps(dict(files=abi,
        raw_NSWindow_pointer_boundary='stage43_window.s26_window_pointer -> stage29/30/31.ensure_dd + stage43_input + stage43_cursor',
        current_pointer_transfer_not_valid_cross_process=True,existing_binary_export_import_should_be_retained=True,
        port_not_implemented=True),indent=2)+'\n')
    source_metrics=json.loads((E/'stage63-result.json').read_text())['host_history_largest_fields']
    dd=next(q for q in source_metrics if q['field']=='dd_native_blob')['encoded_JSON_bytes']
    dd_raw=(dd-12)//2
    timings={str(size):dict(import_seconds=[q['seconds'] for q in v['wire_rows'] if q['command']=='SURFACE_IMPORT'],
        export_seconds=[q['seconds'] for q in v['wire_rows'] if q['command']=='SURFACE_EXPORT'],
        window_create_seconds=[q['seconds'] for q in v['wire_rows'] if q['command']=='WINDOW_CREATE'],
        complete_parent_test_seconds=v['seconds']) for size,v in [(3145728,small),(12582912,large)]}
    result=dict(status='own_UI_helper_IPC_prototype_passed_original_still_stopped',
        direct_Python_child=True,UID_EUID_same=True,security_session_same=True,both_sandboxed=True,
        scoped_permission_matrix_equal_before_and_after_UI=True,full_profile_equality_not_claimed=True,
        no_credentials_entitlements_sandbox_extensions_transferred=True,no_privilege_escalation=True,
        helper_main_thread=True,Python_AppKit_initialization=False,explicit_stdin_stdout_JSON_IPC=True,
        own_window_background_nonkey_nonmain_inactive=True,no_global_input_hook=True,no_audio=True,
        own_3MiB_and_12MiB_surfaces_exact=True,new_helper_defined_state_restore_exact=True,
        helpers_normal_exit_and_not_alive=True,own_outer_hard_seconds=60,own_child_alarm_seconds=45,
        scoped_queries_are_read_only=True,initial_query_binding_error_preserved_and_corrected=True,
        original_DD_blob_prior_measured_encoded_JSON_bytes=dd,original_DD_blob_raw_bytes_inferred_from_old_bytes_tag=dd_raw,
        original_DD_blob_loaded=False,cost_timings=timings,
        measured_IPC_cost_excludes_original_guest_and_original_182MB_history_encoding=True,
        Metal_GPU_readback_or_game_pixel_equivalence_not_proven=True,
        raw_NSWindow_pointer_cannot_cross_process=True,old_native_ABI_drop_in_compatibility_not_proven=True,
        future_guest_callbacks_event_pump_surface_lock_text_cursor_port_required=True,
        helper_full_migration_larger_than_a_single_registration_fix=True,verified_single_registration_fix_unknown=True,
        permission_workaround_evidence=False,permission_matrix_is_scoped_not_exhaustive=True,
        no_denied_log_read_retry=True,no_game_EXE_or_resource_loaded=True,no_Unicorn_loaded=True,
        original_execution_processes_added=0,no_original_DLL=True,no_run21=True,
        budget_used=20,budget_remaining=80,ledger_closed=True,ledger_unchanged=True,ledger_sha256=EXPECTED,
        latest_checkpoint='cycle62p19',phase=22,advances=17,entries=17,tail=0,
        protected_hash_checks=3482,bound_pins_rechecked=len(l['pinned_files']),checkpoint_three_hashes_unchanged=True,
        next_stage='Only port own adapter interfaces and validate self-authored state; no original execution until separately directed within existing bounds.')
    (E/'stage66-result.json').write_text(json.dumps(result,indent=2)+'\n')
    artifacts=[ROOT/'samples'/name for name in ['stage66_helper.m','stage66_context_test.py','stage66_ipc_canvas_test.py','stage66_finalize.py']]+[ROOT/'runs/stage66-ui-helper']
    (E/'stage66-own-prototype-artifacts.json').write_text(json.dumps(dict(status='OWN_PROTOTYPE_NOT_ORIGINAL_EXECUTION_APPROVAL',files={str(p):sha(p) for p in artifacts}),indent=2)+'\n')
    (E/'stage66-postflight.json').write_text(json.dumps(dict(all_passed=True,ledger_sha256=sha(LEDGER),approval_sha256=sha(APPROVAL),
        ledger_unchanged=True,protected_hash_checks=3482,bound_source_pins_checked=len(l['pinned_files']),checkpoint_unchanged=True,
        original_executions_added=0,no_run21=True),indent=2)+'\n')
    report='''原生UI helper加明确IPC的最小路线已经实测可行，不需要用户Terminal参与。本轮仅自造画布/窗口，没有加载原EXE、Unicorn、原资源或旧适配器，没有第21次原运行。

权限上下文：Python用subprocess.Popen直接派生helper，start_new_session=False，同UID/EUID501、同security session100002、同session attributes24624，两边sandbox_present=1。只查询、不实际访问的六项矩阵完全一致：任务目录写允许，原项目/系统文件写限制，LaunchServices与指定WindowServer新lookup限制，用户preferences lookup允许。12MiB测试还在窗口创建后重新查询，矩阵不变。没有提升权限、entitlement传递、sandbox extension、改变sandbox或setuid。自写helper代码没有这些入口。这里只证明继承关系及所查矩阵一致，不声称拿到了完整沙箱profile的逐字节对照；目前没有“通过更高权限native子进程绕过限制”的证据。签名只读检查未显示helper额外entitlement。

权限查询第一次Python绑定少了arm64 variadic固定参数签名，带过滤器调用全返回−1；原生返回有效0/1，初次测试停止而没有初始化AppKit。按Python官方规则补全argtypes后查询成功并与原生一致。这是自写诊断绑定错误，不是真实权限不同；原失败日志/context-comparison.json保留。规则来源：[Python ctypes variadic functions](https://docs.python.org/3/library/ctypes.html#calling-variadic-functions)。只读sandbox_check声明参照[WebKit SandboxSPI.h](https://github.com/WebKit/WebKit/blob/main/Source/WTF/wtf/spi/darwin/SandboxSPI.h)，实际调用的是系统既有libsystem_sandbox，未运行仓库代码。

原型协议是带严格顺序id的stdin/stdout JSON命令：HELLO、SURFACE_IMPORT/EXPORT、WINDOW_CREATE、CLOSE。Python只加载系统Security/sandbox查询库，AppKit在原生helper主线程。surface_id=1、refs=1、尺寸/pitch/RGBA8以及像素作为明示状态传输，不序列化NSWindow/ObjC/CGContext地址。两个helper先后重建384×256窗口，drawRect各执行2次，visible=true、key=false、main=false、NSApp_active=false、activation_requests=0；无全局输入hook/声音/系统截图。CLOSE使窗口不可见，两helper正常退出，所有测试PID已不存在。玩具协议在EOF也关闭窗口，helper有45秒独立alarm，自写整个父测试仍由60秒运行器限制；这些是原型生命周期界限，不是假称现有原执行生命周期已完成接入。

3MiB自造1024×768 surface的读回、跨helper像素及元数据一致，整个测试1.713秒外层正常退出。12MiB自造2048×1536 surface测试2.416秒外层正常退出，第二helper恢复并重建窗口后整个已定义状态字典与第一helper导出体完全相同；UI创建前后权限矩阵保持。12MiB IPC导入0.130–0.165秒，导出0.119–0.127秒，窗口创建0.041–0.048秒；首helper启动/HELLO1.158秒、第二0.004秒，不能据此推断持续冷启动耗时。导入JSON约16.8MB、导出约19.4MB（base64加JSON slash escaping），多份buffer复制存在。作为数量级比较，既有Stage63仅统计的dd_native_blob编码JSON22,957,164字节，对bytes-tag去hex得到原raw约11,478,576字节；本轮没有加载其原始内容。12MiB自造状态已覆盖接近的单块传输量，但不包含原182.7MB历史编码、全部线程、音频、资源或原实际冷恢复，因此不能宣称60秒原恢复已通过。后续适合把bulk pixel/state改为有界长度前缀二进制，控制消息保持明确IPC；不能把每个scalar调用都附带完整surface。

迁移ABI：现有NativeWindow.pointer()直接返回NSWindow*，NativeDD构造、NativeInput、NativeCursor都使用它，原有dylib不能拿helper进程地址在Python进程继续工作。需要把AppKit/Metal窗口、DD、光标、坐标转换和消息泵一并移入helper；保留原逻辑handle/32位guest地址及返回码，在Python端用proxy或不含AppKit的C shim保持.fn调用形状并回填buffer。现有s26/s27窗口Image/LifeImage和s29 DD的export/import主要是标量、resource字节及surface像素，可以保持旧二进制blob格式，恢复后重新建立真正的native对象。此可迁移判断来自自写源码静态审查，尚未做到旧blob兼容或drop-in替换。原WndProc/guest回调仍留Python侧，必须有可重入且有界的事件协议，防止同步IPC回调死锁。surface lock、Metal GPU readback、文本/DC、多对象refs、光标/鼠标、关闭与异常子进程回收也需验证；原型只证明NSView绘制与surface字节恢复，不是Metal输出逐像素等价。

权衡：这条路线技术上值得继续做自写ABI桥接，但完整迁移是跨多个适配模块的架构工作，比若能找到单点Python注册修复明显更广。当前并无已验证且无需权限变化的Python单点修复，helper原型提供了可推进的本地代码路线。下一步可先保留旧窗口/DD导出格式、用自造state验证bridge；当前不建议直接消耗第21次试整个原恢复。

账本/approval均未改：20used/80余关闭、cycle62p19、phase22、推进17/51、入口17/128、尾0/1及输入保存体不变，3482保护hash和550绑定通过。原checkpoint三文件哈希不变。无权限升级、被拒日志重试、宿主设置、安装、上传、提交或发布。

交付evidence/stage66-result.json、stage66-context-comparison-v2.json、stage66-ipc-canvas-result.json、stage66-canvas12m-ipc-canvas-result.json、stage66-static-ABI-migration-catalog.json、stage66-postflight.json；自写源samples/stage66_helper.m、stage66_context_test.py、stage66_ipc_canvas_test.py。
'''
    (ROOT/'reports/stage66-native-UI-helper-IPC-feasibility.md').write_text(report)
    print(json.dumps({k:result[k] for k in ['status','scoped_permission_matrix_equal_before_and_after_UI','own_3MiB_and_12MiB_surfaces_exact','new_helper_defined_state_restore_exact','budget_used','budget_remaining','no_run21','ledger_unchanged']}))
if __name__=='__main__':main()
