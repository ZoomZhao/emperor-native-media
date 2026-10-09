"""One controlled-device handover/replay preview; no emulator or host input calls."""
import copy
import hashlib
import json
import os
import struct
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
EV=ROOT/'evidence'
LEDGER_SHA='61d81a93ce89a0c8b79bec344f31960d34362dc52e2b23f4c8e7af3bd85c7353'


def canonical(q):return json.dumps(q,sort_keys=True,separators=(',',':'),ensure_ascii=False).encode()
def digest(q):return hashlib.sha256(canonical(q)).hexdigest()
def file_hash(p):
    h=hashlib.sha256()
    with Path(p).open('rb') as f:
        for b in iter(lambda:f.read(1048576),b''):h.update(b)
    return h.hexdigest()
def decode(v):
    if isinstance(v,list):return [decode(q) for q in v]
    if not isinstance(v,dict):return v
    if 'dict' in v:return {decode(k):decode(q) for k,q in v['dict']}
    if 'bytes' in v:return bytes.fromhex(v['bytes'])
    if 'tuple' in v:return tuple(decode(q) for q in v['tuple'])
    return v


class PreviewDevice:
    """Test key slots, event clock and shared latch; never a physical-key provider."""
    def __init__(self,manifest,boundary,existing_device_snapshot=None):
        if existing_device_snapshot is not None:
            raise ValueError('same-domain restore must restore saved device, not initialize all-up again')
        if not manifest['explicit_new_input_domain'] or not manifest['acknowledge_legacy_physical_state_unknown']:
            raise ValueError('unapproved/implicit input-domain switch')
        if boundary['pending_saved_keyboard_input']:
            raise ValueError('old keyboard input must not be silently discarded')
        self.manifest=copy.deepcopy(manifest);self.domain=digest(manifest)
        self.down=bytearray(256);self.pending=bytearray(256)
        self.tick=0;self.cursor=0;self.log=[];self.head=self.domain
        self._record('DEVICE_ATTACH',boundary=boundary,initial_key_slots='all256up; explicit new device',
                     old_guest_state_mutated=False,old_provider_state_cleared=False)

    def _record(self,kind,**fields):
        payload=dict(sequence=len(self.log),kind=kind,tick=self.tick,domain=self.domain,**fields)
        row=dict(payload=payload,previous=self.head,sha256=digest(dict(previous=self.head,payload=payload)))
        self.log.append(row);self.head=row['sha256']

    def advance(self,tick):
        if type(tick) is not int or tick<self.tick:raise ValueError('device clock cannot go backward')
        timeline=self.manifest['timeline']
        while self.cursor<len(timeline) and timeline[self.cursor]['tick']<=tick:
            e=timeline[self.cursor];self.tick=e['tick'];vk=e['vk'];down=e['down']
            if not 8<=vk<255:raise ValueError('keyboard slot outside declared device; mouse buttons refused')
            if down and not self.down[vk]:self.pending[vk]=1
            self.down[vk]=int(down);self.cursor+=1
            self._record('DEVICE_EVENT',event=e,cursor=self.cursor)
        self.tick=tick
        self._record('ADVANCE_DEVICE_CLOCK',timeline_cursor=self.cursor)

    def query(self,vk,tid,return_pc):
        if not 8<=vk<255:raise ValueError('keyboard-only device; unsupported/mouse key refuses')
        if len(self.log)>=1024:raise ValueError('finite input audit cap')
        value=(0x8000 if self.down[vk] else 0)|(1 if self.pending[vk] else 0)
        old_pending=self.pending[vk];self.pending[vk]=0
        self._record('SIMULATED_QUERY',vk=vk,guest_tid=tid,caller=return_pc,result=value,
                     down=int(self.down[vk]),pending_before=old_pending,pending_after=0,
                     source='declared test device, not host GetAsyncKeyState')
        return value

    def snapshot(self):
        payload=dict(version=1,manifest=self.manifest,domain=self.domain,tick=self.tick,cursor=self.cursor,
                     down=self.down.hex(),pending=self.pending.hex(),log=self.log,head=self.head)
        return dict(payload=copy.deepcopy(payload),sha256=digest(payload))

    @classmethod
    def restore(cls,snapshot):
        p=copy.deepcopy(snapshot['payload'])
        if digest(p)!=snapshot['sha256'] or p['domain']!=digest(p['manifest']):raise ValueError('device snapshot identity mismatch')
        head=p['domain']
        for index,row in enumerate(p['log']):
            if row['previous']!=head or row['payload']['sequence']!=index:raise ValueError('broken event-log prefix')
            if row['sha256']!=digest(dict(previous=head,payload=row['payload'])):raise ValueError('broken event-log hash')
            head=row['sha256']
        if head!=p['head']:raise ValueError('wrong log tail')
        s=cls.__new__(cls);s.manifest=p['manifest'];s.domain=p['domain'];s.tick=p['tick'];s.cursor=p['cursor']
        s.down=bytearray.fromhex(p['down']);s.pending=bytearray.fromhex(p['pending']);s.log=p['log'];s.head=p['head']
        if len(s.down)!=256 or len(s.pending)!=256:raise ValueError('bad device extent')
        return s


def main():
    assert file_hash(EV/'stage55-batch-ledger.json')==LEDGER_SHA
    ledger=json.loads((EV/'stage55-batch-ledger.json').read_text())
    assert len(ledger['executions'])==4 and ledger['remaining_processes']==96 and ledger['goal_closed']
    state_path=ROOT/'runs/stage12-state-frame56a/state.json'
    state=json.loads(state_path.read_text());h=dict(state['host']['dict'])
    legacy=decode(h['input_calls']);assert len(legacy)==43
    assert all(q['args']==[18] and q['return_value']==0 for q in legacy)
    pump=decode(h['pump_blob42']);route=decode(h['input_route_blob45'])
    assert len(pump)==56 and len(route)==3372
    pump_pending=struct.unpack_from('<I',pump,8)[0]
    saved_msg=struct.unpack_from('<7I',pump,12)
    queued=struct.unpack_from('<I',route,2600)[0]
    assert pump_pending==0 and queued==0 and h['message_callback42'] is None and h['window_pending'] is None
    with (state_path.parent/'memory.bin').open('rb') as f:
        def read(addr,n):
            for r in state['regions']:
                if r['low']<=addr and addr+n<=r['high']+1:
                    f.seek(r['offset']+addr-r['low']);return f.read(n)
            raise ValueError(hex(addr))
        ret,vk=struct.unpack('<2I',read(state['registers']['esp'],8))
        assert (state['registers']['eip'],ret,vk)==(0x61000a00,0x477fb9,113)
        source_keyboard_poll_gate=struct.unpack('<I',read(0x876610,4))[0]
    checkpoints=ledger['executions'][-1]['checkpoint_files']
    for q in checkpoints:assert file_hash(ROOT/q['path'])==q['sha256']
    boundary=dict(checkpoint='frame56a',checkpoint_files=checkpoints,ledger_sha256=LEDGER_SHA,
        CPU_PC='0x61000a00',next_query=dict(name='GetAsyncKeyState',vk=113,return_pc='0x477fb9'),
        saved_pump_pending=pump_pending,saved_route_queued=queued,saved_inactive_last_MSG=saved_msg[1],
        pending_saved_keyboard_input=False,message_callback_pending=False,window_callback_pending=False,
        old_keyboard_queries=dict(count=43,only_vk=18,last_result=0,history_not_current_physical_proof=True),
        serialized_logical_keyboard_device_present=False,legacy_physical_state='unknown; not read or reset',
        arbitrary_guest_keyboard_caches='not exhaustively recovered; all guest bytes preserved',
        original_keyboard_poll_gate=source_keyboard_poll_gate,original_keyboard_poll_gate_not_modified=True,
        original_guest_active_desktop_mapping='not yet fully recovered',
        preserved_watch=dict(phase=6,advances=1,entries=1,tail_entries=0))
    (EV/'stage60-boundary-audit.json').write_text(json.dumps(boundary,indent=2)+'\n')
    manifest=dict(version=1,status='DRAFT_PREVIEW_ONLY_NOT_EXECUTION_APPROVAL',
        explicit_new_input_domain=True,acknowledge_legacy_physical_state_unknown=True,
        source_checkpoint='frame56a',source_checkpoint_files=checkpoints,
        target='bounded city behavior after explicit new test-device attachment; mixed historical prefix',
        keyboard_key_slots='256 slot storage; queries8..254; mouse1..6 and reserved endpoints refuse',
        initial_state='all256keyslotsup; no pending edges; no release events injected',
        virtual_desktop='explicit test desktop active with owned guest-thread access; not inferred from host foreground',
        shared_latch='one recent-press latch per key across guest threads, deterministic ordering',
        device_clock='separate explicit integer test clock; idle scenario independent of host time',
        future_input_API_policy='all keyboard reads/modifier projections use one device; unknown keyboard API refuses',
        future_queue_policy='preserve old queues; keyboard MSG/SYSKEY/CHAR can only originate from declared device; no live desktop keyboard source',
        timeline=[],audit_cap=1024,original_runtime_connected=False)
    (EV/'stage60-proposed-input-domain.json').write_text(json.dumps(manifest,indent=2)+'\n')
    # One handover/audit/replay demonstration, not another suite of per-key microtests.
    idle=PreviewDevice(manifest,boundary)
    idle_values=[idle.query(k,1,p) for k,p in [(113,'0x477fb9'),(116,'0x477fce'),(117,'0x477fe3'),(18,'generic_modifier_query')]]
    assert idle_values==[0,0,0,0]  # computed from explicitly declared empty device
    scripted=copy.deepcopy(manifest)
    scripted['timeline']=[dict(tick=10,vk=113,down=True),dict(tick=10,vk=116,down=True),dict(tick=20,vk=113,down=False)]
    device=PreviewDevice(scripted,boundary);device.advance(10)
    assert device.query(113,1,'controlled_demo')==0x8001
    frozen=device.snapshot();restored=PreviewDevice.restore(frozen)
    assert restored.snapshot()==frozen
    try:PreviewDevice(scripted,boundary,existing_device_snapshot=frozen)
    except ValueError:reset_refused=True
    else:raise AssertionError('restore must not reinitialize held keys')
    for q in (device,restored):
        assert q.query(113,1,'controlled_demo_after_restore')==0x8000
        assert q.query(116,2,'controlled_demo_second_consumer')==0x8001
        q.advance(20)
    assert device.snapshot()==restored.snapshot()
    (EV/'stage60-controlled-demo-trace.json').write_text(json.dumps(dict(idle=idle.snapshot(),scripted=frozen,
                       after_uninterrupted=device.snapshot(),after_restore=restored.snapshot(),
                       restore_lifecycle=dict(frozen_sha256=frozen['sha256'],no_DEVICE_ATTACH_during_restore=True,
                                              no_device_input_event_from_restore=True)),indent=2)+'\n')
    base=json.loads((EV/'stage59-postflight.json').read_text());cache={};checks=[]
    for q in base['checks']:
        p=q['path']
        if p not in cache:cache[p]=file_hash(p)
        checks.append(dict(path=p,expected_sha256=q['expected_sha256'],sha256=cache[p],unchanged=cache[p]==q['expected_sha256']))
    assert all(q['unchanged'] for q in checks)
    assert all(not Path(q['path']).exists() for q in base['absences'])
    assert all(os.readlink(q['path'])==q['target'] for q in base['DOS_namespace_symlinks'])
    assert file_hash(EV/'stage55-batch-ledger.json')==LEDGER_SHA
    (EV/'stage60-postflight.json').write_text(json.dumps(dict(hash_checks=len(checks),checks=checks,
         all_protected_inputs_unchanged=True,absences_unchanged=True,links_unchanged=True,
         original_ledger_unchanged=True),indent=2)+'\n')
    older=json.loads((EV/'stage51-batch-ledger.json').read_text())
    latest_cost=sum(json.loads((EV/p).read_text())['duration_seconds'] for p in
                   ['stage55-run02-original-caller.result.json','stage55-run03-first-city-frame.result.json','stage56-run04-city-frame.result.json'])
    result=dict(status='controlled_input_domain_design_preview_validated_not_connected',
        recommendation='explicit virtual-device attachment at frame56a for the bounded no-key city scenario',
        device_truth_not_constant_physical_zero=True,existing_guest_and_native_checkpoint_not_changed=True,
        input_domain_switch_requires_explicit_review_approval=True,original_runtime_connected=False,
        preview_simulated_idle_returns=idle_values,preview_nonzero_timeline_and_restore_log_passed=True,
        held_and_unconsumed_latches_restored_exactly=True,implicit_allup_reset_on_same_domain_restore_refused=reset_refused,
        original_Windows_global_physical_keyboard_equivalence_proven=False,
        original_Windows_cross_application_lowbit_competition_reproduced=False,
        complete_game_determinism_proven=False,existing_game_audio_live_clocks_unchanged=True,
        cost_comparison=dict(current_boundary='no loader replay; next authorized restore+run counts as one process; prior comparable whole process52.252s, hard cap60s',
             from_returnwatch51=dict(historical_processes=3,historical_whole_seconds=round(latest_cost,3),
                 needs_new_branch_observer_contract=True,existing_gate_would_reject_older_watch_state=True),
             from_read50=dict(historical_loader_processes=len(older['executions']),historical_then_city_total=12,
                 twelve_process_hard_budget_seconds=720,not_a_new_run_completion_prediction=True),
             full_CRT_startup='additional earlier startup chain not quantified here; not the minimal proposal',
             foreground_window='new user-approved focus scope, existing canBecomeKeyNO must change; physical event path still not system-global GetAsyncKeyState equivalence'),
        source_keyboard_history='43 observed Alt0 calls; F2 query is paused before a result; other physical key states not inferable',
        proposed_sidecar='evidence/stage60-proposed-input-domain.json',boundary_audit='evidence/stage60-boundary-audit.json',
        controlled_demo='evidence/stage60-controlled-demo-trace.json',
        original_instructions_executed=0,original_DLL_executed=False,host_input_or_window_used=False,
        global_input_hooks=False,new_TCC_permissions=False,files_uploaded=False,source_written=False,
        system_settings_changed=False,dependencies_installed=False,committed_or_published=False,
        original_batch=dict(used=4,remaining=96,closed=True,phase=6,productive_advances=1,scheduler_entries=1,tail_entries=0),
        protected_hash_checks=len(checks),all_protected_inputs_unchanged=True,original_ledger_unchanged=True)
    (EV/'stage60-result.json').write_text(json.dumps(result,indent=2)+'\n')
    report=f'''建议采用明确的受控测试键盘，在frame56a当前未完成的GetAsyncKeyState(113)前接入。它是新测试环境的输入设备，不是声称真实桌面物理键恒0。空时间线定义所有键松开，查询据设备down/pending计算；非空时间线能给8001/8000/1。该选择适合“从固定暂停态继续”的城市参考验证，不证明历史启动过程或真实全局键盘等价。

已只读核实：原暂停PC61000a00、return477fb9、参数113；历史43个键盘调用仅Alt18且均0，仍不能推断当前其他物理键或lowbit。saved pump pending0、route queued0、窗口/消息callback都无pending，旧pump里514是已消费的最后鼠标消息；没有可恢复的逻辑keyboard device字段，任意guest键缓存未穷举。原输入poll gate保存值{source_keyboard_poll_gate}，不更改。绑定旧三文件哈希，完整恢复旧guest/native后、开放emulation前作显式DEVICE_ATTACH：legacy状态unknown、new device全部松开、device ID/时间线SHA/查询日志头。保留旧历史和所有guest字节，既不清缓存也不注入release；同域后续恢复必须带sidecar的down/pending/clock cursor/shared consumption，禁止再次all-up初始化。

Win32前后台需要区分：键消息依赖focus，GetAsyncKeyState查询会话异步状态，不应把后台简单当成松开；active desktop/访问限制才可能导致API失败0。[微软键盘输入](https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input)、[GetAsyncKeyState](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getasynckeystate)。测试域显式给虚拟桌面active和guest线程访问，与host后台状态分开记；低位是共享可消费latch，不能宣称重现其他Windows应用竞争。GetAsync键盘族、鼠标消息中的键盘modifier投影及未来键消息必须同源，不能F键虚拟、Alt继续读host。此最小空timeline不会生成WM_KEY/CHAR；出现其他键盘API/事件须按真实支持范围拒绝，不能默认成功。原actor/game clocks仍live，本设计只令输入可重复，不保证整游戏完全确定性。

只跑一次stdlib受控切换/时间线审计演示，未重跑Stage59键盘微测试、未用AppKit或原运行：空设备4个模拟query都是有出处的0；非空设备有F2/F5按下，保存时F2已消费/F5未消费，恢复全payload/日志prefix相同，后续与不中断control完全相等。禁止用已有非零device状态重新initialize-all-up；restore无新DEVICE_ATTACH/键事件，输入日志有哈希链和consumer/tick，不是偷偷清按键。交付JSON明确DRAFT、不授权、不接原runtime、不改关闭96账本。

两方案比较：受控域不需TCC/全局监听/前台切换，最少新增宿主影响、便于固定无键城市场景；代价是必须明确批准新输入环境且不等价真实物理GetAsyncKeyState。前台自有窗口需用户允许改变现有nonkey/inactive约束（当前类canBecomeKeyNO），能测本窗口的实际按键事件，但时间和focus易变，仍不足以证明异步全局/跨应用低位；适合之后交互验证，不是当前最小续行方案。

成本：当前frame56a不重载，下一次恢复/执行仍只计一个进程；先前同类过程52.252秒，硬上限60秒。回returnwatch51的历史等效路径3进程{latest_cost:.3f}秒；read50到loader-return历史9进程，再3进程到此点，共12个历史过程，按单个60秒额度为720秒（12分钟）。这不是预测新14秒分片也12次完成，未知边界或source调整可增加次数。更早CRT从头链未在本次量化，不推荐为最小方案。更关键：旧观察器只接受latest checkpoint且watch state必须匹配；退回phase5会与现已推进1/phase6冲突，不能直接消耗现96回滚计数。若追求从早期全程同域，需明确新实验分支/全局计数合同，不得悄悄恢复0或重开新100预算。

当前结论：受控域在明确实验输入来源切换的前提下可以正当地继续当前暂停态之后的无键城市验证；现仅方案和预览通过，需审查接入代码/source pins及用户批准这个域切换后才可续行。原4used/96余closed、推进1/入口1/尾0保持；原指令新增0、原DLL0，原3482保护哈希/缺失18/链接2不变，无播放/上传/安装/宿主设置/提交/发布。主要证据evidence/stage60-result.json、stage60-boundary-audit.json、stage60-proposed-input-domain.json、stage60-controlled-demo-trace.json、stage60-postflight.json。
'''
    (ROOT/'reports/stage60-controlled-input-domain-short-proposal.md').write_text(report)
    print(json.dumps(dict(status=result['status'],original_instructions=0,used=4,remaining=96,
                          implicit_reset_refused=True,controlled_restore_trace_equal=True,hashes=len(checks))))


if __name__=='__main__':main()
