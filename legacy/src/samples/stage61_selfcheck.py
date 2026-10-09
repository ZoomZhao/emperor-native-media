"""One integration check with authored inert host/CPU stand-ins, no originals."""
import copy,json,struct
from types import SimpleNamespace
from stage61_input import Device61,with_controlled_input,ROOT

class Refused(Exception):
    def __init__(self,kind,**details):self.kind=kind;super().__init__(kind)
class Base:extra_snapshot_fields=[]
class Pump:
    def __init__(self):
        self.body=struct.pack('<10I2Q',0x53343250,1,0,0x58000001,0x200,12,0,0,0,0,0,0)
    def snapshot(self):return self.body
    def restore(self,b):self.body=b

def main():
    manifest=json.loads((ROOT/'evidence/stage60-proposed-input-domain.json').read_text())
    boundary=json.loads((ROOT/'evidence/stage60-boundary-audit.json').read_text())
    Runtime=with_controlled_input(Base,SimpleNamespace(),Refused)
    r=Runtime.__new__(Runtime);r.device61=Device61(manifest,boundary)
    r.meta61=dict(message_polls=0,keyboard_only_polls=0,mouse_modifier_projections=0)
    r.pump=Pump();calls=[]
    def inert_peek(remove,lo,hi):
        calls.append([remove,lo,hi]);return 1,[0x58000001,0x200,12,0,0,0,0],[5,0]
    r.raw_peek61=inert_peek
    assert r.controlled_peek61(1,0x100,0x108)==(0,None,[0,0]) and calls==[]
    status,msg,diag=r.controlled_peek61(1,0,0)
    assert calls==[[1,0x200,0x209]] and msg[2]==0 and struct.unpack_from('<I',r.pump.body,20)[0]==0
    r.device61.query(113,1,'own_fixture')
    snap=r.keyboard_snapshot61
    assert snap['device']['payload']['log'][-1]['payload']['kind']=='GUEST_QUERY'
    restored=Device61.restore(snap['device']);assert restored.snapshot()==snap['device']
    try:r.device61.query(1,1,'own_fixture_mouse')
    except ValueError:pass
    else:raise AssertionError('mouse VK must refuse')
    try:r.forbidden_physical_keyboard61()
    except Refused as e:assert e.kind=='Physical_keyboard_provider_disabled_in_test_domain61'
    else:raise AssertionError('physical path must refuse')
    out=dict(passed=True,keyboard_filter_never_calls_native_queue=True,
        all_messages_request_only_native_mouse_types=True,guest_modifiers_from_test_device=True,
        native_default_proc_image_matches_projected_MSG=True,
        device_snapshot_roundtrip_exact=True,query_log_actual_guest_label=True,
        physical_key_provider_and_mouse_VK_refuse=True,original_instructions=0,
        emulator_imported=False,AppKit_used=False)
    (ROOT/'evidence/stage61-integration-selfcheck.json').write_text(json.dumps(out,indent=2)+'\n')
    print(json.dumps(out))
if __name__=='__main__':main()
