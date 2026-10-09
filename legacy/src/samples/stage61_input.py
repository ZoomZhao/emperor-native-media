"""Approved empty test keyboard; no physical-key read or event listener."""
import copy
import hashlib
import json
import struct
from pathlib import Path
from stage60_controlled_input_plan import PreviewDevice, digest

ROOT=Path(__file__).resolve().parents[1]
CONTRACT=ROOT/'evidence/stage61-approved-input-domain.json'

class Device61(PreviewDevice):
    def _record(self,kind,**fields):
        if kind=='SIMULATED_QUERY':kind='GUEST_QUERY'
        return super()._record(kind,**fields)

def with_controlled_input(Base,x,Boundary):
    class ControlledInputRuntime61(Base):
        extra_snapshot_fields=Base.extra_snapshot_fields+['keyboard_snapshot61']

        def __init__(self,*args,**kwargs):
            super().__init__(*args,**kwargs)
            self.device61=None
            self.meta61=dict(version=1,message_polls=0,keyboard_only_polls=0,
                mouse_modifier_projections=0,host_keyboard_queries=0,restore_count=0)
            self.raw_peek61=self.pump.peek
            self.pump.peek=self.controlled_peek61
            self.window.alt=self.forbidden_physical_keyboard61

        def forbidden_physical_keyboard61(self,*args,**kwargs):
            raise Boundary('Physical_keyboard_provider_disabled_in_test_domain61')

        @property
        def keyboard_snapshot61(self):
            if self.device61 is None:return None
            return dict(device=self.device61.snapshot(),meta=copy.deepcopy(self.meta61))

        @keyboard_snapshot61.setter
        def keyboard_snapshot61(self,value):
            if value is None:raise Boundary('Cannot_reset_existing_test_device61')
            self.device61=Device61.restore(value['device'])
            contract=json.loads(CONTRACT.read_text())
            if self.device61.manifest!=contract or contract['timeline']:
                raise Boundary('Controlled_keyboard_identity_or_empty_timeline_changed61')
            self.meta61=copy.deepcopy(value['meta'])
            # Exact getter equality is required by the existing full restore.

        def attach61(self):
            if self.device61 is not None:
                before=self.device61.snapshot()
                self.meta61['restore_count']+=1
                record=dict(kind='EXACT_DEVICE_RESTORE',saved_device_sha256=before['sha256'],
                    saved_log_head=before['payload']['head'],new_DEVICE_ATTACH=False,
                    down_preserved=True,pending_preserved=True,no_injected_release=True)
            else:
                boundary=json.loads((ROOT/'evidence/stage60-boundary-audit.json').read_text())
                manifest=json.loads(CONTRACT.read_text())
                sp=self.uc.reg_read(x.UC_X86_REG_ESP)
                actual=(self.current_tid,self.uc.reg_read(x.UC_X86_REG_EIP),self.readword(sp),self.readword(sp+4))
                if actual!=(1,0x61000a00,0x477fb9,113):
                    raise Boundary('Explicit_input_attach_boundary_changed61',actual=list(actual))
                pump_before=self.pump.snapshot();route_before=self.pump.route_snapshot()
                if struct.unpack_from('<I',pump_before,8)[0] or struct.unpack_from('<I',route_before,2600)[0]:
                    raise Boundary('Cannot_discard_pending_legacy_input61')
                if self.message_callback42 is not None or self.window_pending is not None:
                    raise Boundary('Cannot_attach_during_legacy_callback61')
                if len(self.input_calls)!=43 or any(q['args']!=[18] or q['return_value']!=0 for q in self.input_calls):
                    raise Boundary('Legacy_keyboard_history_changed_before_attach61')
                context=bytes(self.uc.context_save())[40:]
                self.device61=Device61(manifest,boundary)
                assert self.pump.snapshot()==pump_before and self.pump.route_snapshot()==route_before
                assert bytes(self.uc.context_save())[40:]==context
                assert actual==(self.current_tid,self.uc.reg_read(x.UC_X86_REG_EIP),self.readword(sp),self.readword(sp+4))
                record=dict(kind='EXPLICIT_DEVICE_ATTACH',boundary=boundary,
                    original_guest_writes=0,old_queues_unchanged=True,legacy_physical_state='unknown',
                    new_device_initial_state='all keys up; no pending edges',
                    no_release_injected=True,device=self.device61.snapshot())
            (ROOT/f'evidence/stage61-{self.tag54}-input-entry.json').write_text(json.dumps(record,indent=2)+'\n')

        def require_device61(self):
            if self.device61 is None:raise Boundary('Test_keyboard_not_attached61')
            return self.device61

        def modifier_bits61(self):
            d=self.require_device61()
            return (4 if d.down[16] or d.down[160] or d.down[161] else 0)|(8 if d.down[17] or d.down[162] or d.down[163] else 0)

        def controlled_peek61(self,remove,lo=0,hi=0):
            self.require_device61();self.meta61['message_polls']+=1
            if (lo,hi)==(0x100,0x108):
                self.meta61['keyboard_only_polls']+=1
                # Empty timeline yields an actually empty test-device key queue.
                # No native keyboard/flags NSEvent is polled or consumed.
                return 0,None,[0,0]
            if (lo,hi)==(0,0):lo,hi=0x200,0x209
            if (lo,hi) not in [(0x205,0x205),(0x200,0x209)]:
                raise Boundary('Controlled_message_filter_not_supported61',filters=[lo,hi])
            status,msg,diag=self.raw_peek61(remove,lo,hi)
            if status>0:
                msg=list(msg)
                projected=(msg[2]&~12)|self.modifier_bits61()
                if projected!=msg[2]:
                    # Normal API output projection, never old guest-memory mutation.
                    # Keep the native pending/default-proc image consistent with MSG32.
                    image=bytearray(self.pump.snapshot())
                    struct.pack_into('<I',image,20,projected);self.pump.restore(bytes(image))
                    self.meta61['mouse_modifier_projections']+=1
                msg[2]=projected
            return status,msg,diag

        def api(self,dll,name):
            if dll.casefold()=='user32.dll':
                if name=='GetAsyncKeyState':
                    a=self.args(1);pc=self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))
                    try:value=self.require_device61().query(a[0],self.current_tid,hex(pc))
                    except ValueError as error:
                        raise Boundary('Controlled_keyboard_query_refused61',args=a,reason=str(error),return_pc=hex(pc))
                    item=dict(name=name,args=a,return_pc=hex(pc),return_value=value,
                        backend='explicit controlled keyboard test device',domain=self.device61.domain,
                        event_log_head=self.device61.head,physical_keyboard_equivalence=False,
                        host_keyboard_read=False,shared_lowbit_consumption=True)
                    self.input_calls.append(item);self.api_trace.append(item);self.ret(value,1);return
                if name in ['GetKeyState','GetKeyboardState','SetKeyboardState','GetKeyboardLayout',
                            'GetKeyboardLayoutList','MapVirtualKeyA','MapVirtualKeyW','ToAscii','ToUnicode',
                            'RegisterHotKey','UnregisterHotKey','keybd_event','SendInput']:
                    raise Boundary('Other_keyboard_API_requires_contract61',name=name)
                if name in ['TranslateMessage','DispatchMessageA']:
                    a=self.args(1);msg=list(struct.unpack('<7I',bytes(self.uc.mem_read(a[0],28))))
                    if 0x100<=msg[1]<=0x109:
                        raise Boundary('Keyboard_message_outside_empty_test_timeline61',MSG32=msg)
                    if 0x200<=msg[1]<=0x209 and msg[2]&12!=self.modifier_bits61():
                        raise Boundary('Mouse_keyboard_modifier_domain_mismatch61',MSG32=msg)
            return super().api(dll,name)

        def thread_run(self):
            self.attach61()
            try:
                from stage20_threads import ThreadKernel
                return ThreadKernel.thread_run(self,10,max_steps=200000)
            finally:
                (ROOT/f'evidence/stage61-{self.tag54}-input-exit.json').write_text(json.dumps(dict(
                    device=self.device61.snapshot(),meta=self.meta61,
                    host_keyboard_read=False,original_DLL_executed=False,
                    actual_original_segment_ceiling_seconds=10),indent=2)+'\n')
    return ControlledInputRuntime61
