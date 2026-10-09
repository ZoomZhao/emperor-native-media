"""Own NSEvent fixtures only. No Unicorn, original EXE/DLL or physical key injection."""
import argparse
import concurrent.futures
import hashlib
import json
import re
import struct
import sys
from pathlib import Path
from stage59_keys import NativeKeys59,KeyboardScopeError,OriginalKeyboardAPI59

ROOT=Path(__file__).resolve().parents[1]
EV=ROOT/'evidence'
SNAP=ROOT/'runs/stage59-event-state.bin'
SDK=Path('<LOCAL_MACHINE_PATH>')
LEDGER_SHA='61d81a93ce89a0c8b79bec344f31960d34362dc52e2b23f4c8e7af3bd85c7353'
CASES=[]


def passed(name,**evidence):CASES.append(dict(name=name,passed=True,**evidence))


def rejected(name,action,keys):
    before=keys.snapshot()
    try:action()
    except KeyboardScopeError as error:
        assert keys.snapshot()==before
        passed(name,reason=str(error),semantic_state_unchanged=True);return
    raise AssertionError('expected rejection '+name)


def background(keys):
    diag=keys.diagnostic()
    assert diag['main_thread']==1 and diag['app_active']==diag['key_window']==diag['can_become_key']==0
    assert diag['pid']!=diag['frontmost_pid']
    return diag


def codes():
    # Test source is installed Apple SDK, independent of the native lookup table.
    # SDK header contains legacy copyright bytes; names/hex constants are ASCII.
    text=SDK.read_text(encoding='latin1')
    result={0x6f+n:int(re.search(r'\bkVK_F'+str(n)+r'\s*=\s*(0x[0-9A-Fa-f]+)',text).group(1),16) for n in range(1,21)}
    return result


class Registers:ESP=1;EAX=2;EIP=3
class FakeGuest:
    def __init__(self,vk,pc=0x12345678):
        self.regs={1:0x1000,2:0xfedcba98,3:0x2000}
        self.memory=struct.pack('<2I',pc,vk)
    def reg_read(self,key):return self.regs[key]
    def reg_write(self,key,value):self.regs[key]=value
    def mem_read(self,address,n):return self.memory[address-0x1000:address-0x1000+n]


def create():
    key_codes=codes();keys=NativeKeys59()
    try:
        before_diag=background(keys)
        passed('actual native nonkey inactive task panel; no focus or activation',diagnostic=before_diag)
        rejected('unobserved current key state refuses instead of zero',lambda:keys.query_event_state(0x71),keys)
        rejected('background original system async scope refuses without consuming latch',lambda:keys.query_original_scope(0x71),keys)
        initial=keys.diagnostic()['accepted_events']
        assert keys.fixture_event(key_codes[0x71],True)==0
        diag=background(keys)
        assert diag['accepted_events']==initial+1 and diag['delivered']>=1
        first=keys.query_event_state(0x71)
        assert first==0x8001 and (first&0x8001)==0x8001
        passed('actual NSWindow.sendEvent to first responder F2 produces original mask8001',value=first,diagnostic=diag)
        second=keys.query_event_state(0x71)
        assert second==0x8000 and second&0x8001!=0x8001
        passed('shared recent-press latch consumed; second observer sees held8000',first=first,second=second)
        assert keys.fixture_event(key_codes[0x71],True,repeat=True)==0
        assert keys.query_event_state(0x71)==0x8000
        passed('AppKit repeat while held does not invent another physical-down edge',policy='local model, not full Windows repeat proof')
        assert keys.fixture_event(key_codes[0x71],False)==0
        assert keys.query_event_state(0x71)==0
        passed('known key-up returns real zero only after an actual owned release event')
        assert keys.fixture_event(key_codes[0x71],True)==keys.fixture_event(key_codes[0x71],False)==0
        short_press=keys.query_event_state(0x71)
        assert short_press==1 and short_press&0x8001!=0x8001 and keys.query_event_state(0x71)==0
        passed('press then release before polling yields1 then0, never8001',value=short_press)
        assert keys.fixture_event(key_codes[0x71],True)==0
        assert keys.query_event_state(0x71)==0x8001
        passed('a new genuine down after up latches again')
        for vk in (0x74,0x75):assert keys.fixture_event(key_codes[vk],True)==0
        assert keys.query_event_state(0x74)==keys.query_event_state(0x75)==0x8001
        passed('same mapping/state algorithm covers original F5/F6 without caller-address patches')
        before=keys.snapshot();accepted=keys.diagnostic()['accepted_events']
        assert keys.fixture_event(key_codes[0x71],False,other_window=True)==0
        assert keys.snapshot()==before and keys.diagnostic()['accepted_events']==accepted
        passed('event belonging to another window cannot change owned key state',diagnostic=keys.diagnostic())
        assert keys.fixture_event(0,True)==0
        assert keys.snapshot()==before
        passed('unsupported non-function hardware key ignored without reading any text property')
        rejected('unsupported VK_F21 refuses',lambda:keys.query_event_state(0x84),keys)
        # Existing local held keys may contain consumed latches; release before each independent key.
        mapping=[]
        for vk,code in sorted(key_codes.items()):
            assert keys.fixture_event(code,False)==0;keys.query_event_state(vk)
            assert keys.fixture_event(code,True)==0
            value=keys.query_event_state(vk)
            assert value==0x8001
            assert keys.fixture_event(code,False)==0 and keys.query_event_state(vk)==0
            mapping.append(dict(vk=vk,hardware_code=code,value=value))
        passed('all20 function keys mapped from independent installed Apple SDK through real NSEvents',mapping=mapping)
        assert keys.fixture_event(key_codes[0x71],True)==0
        with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
            values=list(pool.map(keys.query_event_state,[0x71]*8))
        assert sorted(values)==[0x8000]*7+[0x8001]
        passed('eight concurrent process-local consumers share one latch atomically',values=values,
               Windows_other_app_competition_not_reproduced=True)
        held=keys.snapshot()
        keys.restore(held,replay=True);assert keys.snapshot()==held
        assert keys.query_event_state(0x71)==0x8000
        passed('frozen replay of held/consumed state cannot manufacture a new pressed bit')
        assert keys.fixture_event(key_codes[0x74],True)==0
        pending=keys.snapshot()
        keys.restore(pending,replay=True);assert keys.snapshot()==pending
        assert keys.query_event_state(0x74)==0x8001 and keys.query_event_state(0x74)==0x8000
        passed('unconsumed frozen edge restored once, then consumed; no added event',snapshot_events=struct.unpack_from('<Q',pending,32)[0])
        # Live host state is not a rewindable snapshot; invalidate rather than play old edges.
        keys.restore(pending,replay=False)
        fields=struct.unpack('<8I2Q',keys.snapshot())
        assert fields[2]==fields[3]==fields[4]==0
        rejected('live restore leaves key state unknown instead of pretending up',lambda:keys.query_event_state(0x71),keys)
        assert keys.fixture_event(key_codes[0x71],True,repeat=True)==0
        assert keys.query_event_state(0x71)==0x8000
        passed('live restore followed by repeat learns held level without manufacturing a press')
        assert keys.fixture_event(key_codes[0x71],False)==keys.fixture_event(key_codes[0x71],True)==0
        assert keys.query_event_state(0x71)==0x8001
        passed('fresh release/down after live invalidation creates a legitimate new local event')
        invalid=bytearray(keys.snapshot());struct.pack_into('<I',invalid,12,1<<31)
        rejected('invalid snapshot bitmask rejected before replacing state',lambda:keys.restore(bytes(invalid),replay=True),keys)
        # The prepared generic ABI refuses while preserving EAX/SP/PC for any caller address.
        for vk,pc in [(0x71,0x477fb9),(0x74,0x477fce),(0x75,0x477fe3),(0x70,0xabcdef01)]:
            guest=FakeGuest(vk,pc);regs=guest.regs.copy()
            rejected('generic original ABI scope refuses '+hex(vk),lambda:OriginalKeyboardAPI59(keys,guest,Registers).call(),keys)
            assert guest.regs==regs
        passed('refused generic ABI leaves guest EAX/SP/PC unchanged; not keyed to original addresses')
        # Separate process will restore this exact event history: F2 consumed, F5 pending.
        assert keys.fixture_event(key_codes[0x74],False)==0;keys.query_event_state(0x74)
        assert keys.fixture_event(key_codes[0x74],True)==0
        frozen=keys.snapshot();SNAP.write_bytes(frozen)
        final=background(keys)
        assert 'unicorn' not in sys.modules
        return dict(snapshot=str(SNAP),snapshot_sha256=hashlib.sha256(frozen).hexdigest(),bytes=len(frozen),
                    native_diagnostic=final,real_physical_key_events_tested=False,synthetic_NSEvents_only=True,
                    original_API_unblocked=False,characters_read=False,global_input_hook_or_monitor=False)
    finally:keys.dispose()


def restore():
    control=json.loads((EV/'stage59-probe-create.json').read_text())['result']
    body=SNAP.read_bytes();assert hashlib.sha256(body).hexdigest()==control['snapshot_sha256']
    keys=NativeKeys59()
    try:
        keys.restore(body,replay=True);assert keys.snapshot()==body
        passed('independent process restores frozen48bytes exactly; no NSEvents posted',bytes=len(body))
        assert keys.query_event_state(0x71)==0x8000
        assert keys.query_event_state(0x74)==0x8001 and keys.query_event_state(0x74)==0x8000
        passed('independent restored F2 consumed/F5 pending produce same once-only query future')
        rejected('restored history still cannot answer original global async query',lambda:keys.query_original_scope(0x71),keys)
        diag=background(keys);assert diag['fixture_events']==diag['delivered']==0
        assert 'unicorn' not in sys.modules
        return dict(native_diagnostic=diag,original_API_unblocked=False,new_native_key_events=0)
    finally:keys.dispose()


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--mode',choices=['create','restore'],required=True)
    args=parser.parse_args()
    assert hashlib.sha256((EV/'stage55-batch-ledger.json').read_bytes()).hexdigest()==LEDGER_SHA
    result=create() if args.mode=='create' else restore()
    assert hashlib.sha256((EV/'stage55-batch-ledger.json').read_bytes()).hexdigest()==LEDGER_SHA
    output=dict(mode=args.mode,passed=all(q['passed'] for q in CASES),case_count=len(CASES),cases=CASES,
                result=result,original_instructions_executed=0,original_DLL_executed=False,
                original_ledger_unchanged=True,original_budget=dict(used=4,remaining=96,closed=True))
    (EV/('stage59-probe-'+args.mode+'.json')).write_text(json.dumps(output,indent=2)+'\n')
    print(json.dumps(dict(mode=args.mode,passed=output['passed'],cases=len(CASES))))


if __name__=='__main__':main()
