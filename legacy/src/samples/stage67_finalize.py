# -*- coding: utf-8 -*-
"""Bounded partial migration handoff: legacy screen contract blocks readiness."""
import ast,hashlib,json
from pathlib import Path
from stage54_batch import LEDGER,APPROVAL,sha,validate
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
EXPECTED='4e499d8560cc4c7a9bc4a2e725e27f502fa13979de6366e9f66bd06f237edcc9'
def main():
 assert sha(LEDGER)==EXPECTED
 l=json.loads(LEDGER.read_text());validate(l);assert l['goal_closed'] and len(l['executions'])==20 and l['remaining_processes']==80
 assert l['latest_checkpoint']=='cycle62p19' and l['watch_state']['phase']==22
 prior=json.loads((E/'stage64-postflight.json').read_text())
 for q in prior['checks']:assert sha(q['path'])==q['expected_sha256'],q['path']
 for p,h in l['pinned_files'].items():assert sha(p)==h,p
 for f in l['executions'][18]['checkpoint_files']:assert sha(ROOT/f['path'])==f['sha256']
 own=['stage67_pixels.c','stage67_launcher.m','stage67_build.py','stage67_server.py','stage67_proxy.py','stage67_dd.py','stage67_selfcheck.py','stage67_pixel_selfcheck.py','stage67_finalize.py']
 for n in own:
  if n.endswith('.py'):ast.parse((ROOT/'samples'/n).read_text())
 pixel=json.loads((E/'stage67-pixel-selfcheck-v2.json').read_text());assert pixel['status']=='detached_pixels_saved_fixture_equal'
 assert len(pixel['rows'])==8 and all(q['byte_mismatches']==0 for q in pixel['rows'])
 screen=(E/'stage67-window-isolation-helper.stderr.log').read_text()
 assert 'main_thread=1 screens=0 main_screen=0' in screen
 isolated=(E/'stage67-own-resource-window-isolation.stdout.log').read_text()
 assert 'window import 0' in isolated and 'close 0' in isolated
 full=json.loads((E/'stage67-own-UI-ABI-proxy-selfcheck-v2.result.json').read_text());assert full['returncode']==1 and not full['timed_out']
 dd=json.loads((E/'stage67-dd-build.json').read_text());assert sha(dd['path'])==dd['sha256'] and not dd['translated_game_code_linked']
 result=dict(status='partial_proxy_migration_legacy_NSScreen_contract_BLOCKED',
     native_entry_embeds_only_installed_Apple_Python=True,Python_parent_No_AppKit=True,
     scoped_context_validation_passed=True,opaque_remote_handles_implemented=True,
     buffer_copyback_and_native_failure_not_success_stub=True,
     own_window_colour_and_cursor_resource_calls_actual_nonzero=True,
     legacy_window_import_actual_return=0,native_main_thread=True,NSScreen_count=0,mainScreen_available=False,
     permissions_changed=False,global_display_access_workaround_attempted=False,
     clean_DD_source_built_no_original_translated_functions=True,
     independent_RGB555_RGB565_platform_pixels_eight_saved_synthetic_fixtures_exact=True,
     pixel_initial_alpha_mismatch_preserved_then_fixed=True,
     legacy_window_DD_scene_restore_selfcheck_failed=True,
     original_DD_GPU_present_and_callbacks_not_proven=True,
     atomic_scene_export_command_implemented=True,staging_new_helper_design_implemented_in_selfcheck=True,
     whole_scene_restore_commit_not_validated=True,
     pump_cursor_text_protocol_present_but_not_full_integration_validated=True,
     complete_cycle62p19_restore_not_ready=True,no_new_wrapper_activation=True,
     current_rollback='Keep approved old files and checkpoint untouched; retain Stage66 own display-only IPC prototype. Do not attach partial proxy to production runtime.',
     prospective_minimal_route='Separate only final framebuffer presentation into Stage66 helper while retaining logical old DD/surface/state in Python with clean platform backend; virtual desktop geometry must be explicitly modeled and validated against saved ABI instead of inventing native NSScreen success.',
     next_scope_choice_requires_parent_direction=True,
     no_EXE_no_Unicorn_no_game_resources_loaded=True,original_reference_instructions_executed=0,original_DLL_executed=False,
     used=20,remaining=80,closed=True,ledger_unchanged=True,ledger_sha256=EXPECTED,
     latest_checkpoint='cycle62p19',phase=22,advances=17,entries=17,tail=0,no_run21=True,
     protected_hash_checks=3482,pins_rechecked=len(l['pinned_files']),old_sources_and_checkpoint_unchanged=True)
 (E/'stage67-result.json').write_text(json.dumps(result,indent=2)+'\n')
 (E/'stage67-own-migration-artifacts.json').write_text(json.dumps(dict(status='PARTIAL_NOT_ORIGINAL_EXECUTION_APPROVAL',
    sources={str(ROOT/'samples'/n):sha(ROOT/'samples'/n) for n in own},
    binaries={str(ROOT/'runs/stage67-native'/n):sha(ROOT/'runs/stage67-native'/n)for n in ['stage67-ui-helper','libstage67_dd.dylib','libstage67_pixels.dylib']}),indent=2)+'\n')
 (E/'stage67-postflight.json').write_text(json.dumps(dict(all_protected_passed=True,ledger_unchanged=True,ledger_sha256=sha(LEDGER),
    approval_sha256=sha(APPROVAL),protected_hash_checks=3482,pins_rechecked=len(l['pinned_files']),checkpoint_unchanged=True,no_run21=True),indent=2)+'\n')
 report='''本轮约20分钟完成部分迁移并定位新的实际边界，不能把代理接入当前原运行。旧源、ledger、approval和cycle62p19完全不动，没有第21次、原EXE/Unicorn/原游戏资源加载或原指令执行。

最小闭合范围静态/快照审查：当前DD含6个逻辑对象，primary1440×900/32bit；1024×768/32bit offscreen与1024×768/RGB555 main都处于locked=1，还有RGB555辅助surface和refs=2 clipper。后启动记录仍有Lock/Unlock、IsLost、555→32 Blt及primary Metal present；光标SetCursor和消息泵PeekMessage持续使用。text_dc_state当前为空，但NativeText、cursor_selection_blob、pump/route、window Image/LifeImage都是构造或完整恢复链的一部分。没有假定只迁移present就自动解决所有依赖。

实现：新增原生launcher在主线程先构造NSApplication，再嵌入已安装Apple Python.framework运行自写RPC服务；不调用或启动另一个Python.app，不加载site/user包。主Python保持无AppKit。服务复用自写NativeWindow、cursor、pump、input和text包装；DD使用新的隔离编译版本。客户端Handle不可转为整数地址，NSWindow指针只在helper内传给DD，RPC函数返回保留真实status，对out buffer按实际字节回填；禁止物理键查询、窗口activate/focus和PNG输出。helper身份/安全会话/既定六项sandbox查询与父进程一致，失败与关闭均通过显式IPC。

不能直接复用旧DD binary：它链接了原游戏555/565转换的LLVM函数。本轮重新编译原自写stage34/stage33 DD/Metal代码，链接独立平台Surface实现，没有CPU/native_cpu、原转换函数或combined.ll。RGB555/565初次自己实现遗漏alpha，已有自造case参考记录出现每像素一字节差异；修正alpha=0xff后8个已有自造夹具全字节一致，含剪裁、keyed、pitch padding等输入。只是读取既有自造输入和保存输出，未重新执行原参考函数。此验证不等于完整DD/Metal端到端验证。

实际失败：旧Window import用自造66332字节Image调用，native返回0，原断言经RPC传回RemoteError。分离实验的真实brush/cursor资源创建分别返回1459617793/1459617794，说明不是空包装；关闭正常。原生prefix实测main_thread=1、NSScreen.screens.count=0、NSScreen.mainScreen=nil。旧make_window依赖mainScreen，因此拒绝恢复；Stage66自造窗口没使用NSScreen，不能据其成功推断旧ABI全链可用。新helper仍在相同查询矩阵下，未通过其他途径读取屏幕/WindowServer信息、升级权限或重试被拒日志。

截至交回：旧Image/LifeImage、DD scene、cursor/pump的完整跨helper恢复测试在窗口阶段真实失败，后续GPU present/消息FIFO/callback测试未到达。SCENE_EXPORT提供单命令主线程冻结导出，测试代码设计为新helper staging、失败保留原scene；由于前置失败，原子完整scene恢复尚未通过，不声称通过。代理函数形状已有，但构造别名/visibility/text/消息回调完整接入旧runtime尚未完成；不改变source pins或把半成品注册为生产入口。

最小可回退方案：保留所有批准旧文件、关闭账本、当前checkpoint，Stage66显示-only原型可继续单独自造测试；本轮新增文件全在独立stage67目录，可停用不影响已有工作。较小后续候选是仅把最终framebuffer呈现交给helper，旧逻辑DD/surface state留Python并用干净平台像素后端；但窗口逻辑几何需显式使用快照中的virtual desktop模型并与旧ABI校验，不能把缺失的NSScreen伪装成功。这比全UI迁移更窄，仍需parent决定和自造验证后才谈原第21次。当前旧窗口/屏幕依赖闭合范围已超出一个小wrapper修复，先交回权衡。

保护复核3482hash、550pins和原3文件hash通过。账本SHA4e499d8560cc4c7a9bc4a2e725e27f502fa13979de6366e9f66bd06f237edcc9保持关闭20used/80余，phase22、17/51推进、17/128入口、0/1尾，输入域未重置。无原DLL、安装、实播、权限/系统设置改变、上传、提交或发布。

交付：stage67-active-path-readonly-audit.json、stage67-DD-snapshot-metadata.json、stage67-pixel-selfcheck-v2.json、stage67-result.json、stage67-postflight.json；失败日志stage67-own-UI-ABI-proxy-selfcheck-v2.stderr.log及stage67-own-resource-window-isolation.stdout.log；代码stage67_proxy.py、stage67_server.py、stage67_launcher.m、stage67_pixels.c。
'''
 (ROOT/'reports/stage67-partial-UI-proxy-NSScreen-boundary.md').write_text(report)
 print(json.dumps({k:result[k]for k in ['status','NSScreen_count','legacy_window_import_actual_return','independent_RGB555_RGB565_platform_pixels_eight_saved_synthetic_fixtures_exact','complete_cycle62p19_restore_not_ready','used','remaining','no_run21']}))
if __name__=='__main__':main()
