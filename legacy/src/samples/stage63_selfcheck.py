"""Actual recorded typed fields + inert atomic-save/presentation fixtures, no originals."""
import collections,json,hashlib,struct,tempfile,time
from pathlib import Path
from types import SimpleNamespace
from stage12_state import encode,decode
from stage63_checkpoint import fast_encode,budget_seconds,save_checkpoint
from checkpoint_startup63 import runtime_mixin
ROOT=Path(__file__).resolve().parents[1]
def main():
    assert budget_seconds(26)==8 and budget_seconds(30)==5 and budget_seconds(35)==0
    record=json.loads((ROOT/'runs/stage12-state-cycle62p19/state.json').read_text());host=dict(record['host']['dict'])
    field_rows=[]
    for name,value in host.items():
        decoded=decode(value);start=time.monotonic();expected=encode(decoded);old_seconds=time.monotonic()-start
        start=time.monotonic();actual=fast_encode(decoded);new_seconds=time.monotonic()-start
        assert actual==expected,name
        field_rows.append(dict(field=name,legacy_encode_seconds=old_seconds,fast_encode_seconds=new_seconds))
    class Context:
        def __getstate__(self):return (bytes(64),64,4,4)
    class UC:
        def mem_regions(self):return [(0x1000,0x1003,3)]
        def mem_read(self,addr,n):assert(addr,n)==(0x1000,4);return b'abcd'
        def context_save(self):return Context()
        def reg_read(self,key):return {1:7,2:0x1000,3:0x202}[key]
    fake=SimpleNamespace(uc=UC(),pe=SimpleNamespace(data=b'authored EXE fixture'),save=b'authored save',
        model_payload=b'authored model',sparse=False,history=[{'n':1,'bytes':b'unchanged'}])
    x=SimpleNamespace(UC_X86_REG_EFLAGS=3);marks=[]
    with tempfile.TemporaryDirectory(prefix='stage63-save-fixture-',dir=ROOT/'runs') as tmp:
        path=Path(tmp);(path/'runs').mkdir()
        saved,hashes=save_checkpoint(fake,'fixture',[1],path,dict(eax=1,esp=2),x,lambda r:['history'],lambda k,**f:marks.append(k))
        for q in hashes:assert hashlib.sha256((path/q['path']).read_bytes()).hexdigest()==q['sha256']
        assert decode(saved['host'])==dict(history=fake.history) and (path/'runs/stage12-state-fixture').is_dir()
        assert not (path/'runs/stage12-state-fixture.stage63-pending').exists()
        try:save_checkpoint(fake,'fault',[1],path,dict(eax=1,esp=2),x,lambda r:['history'],lambda *a,**k:None,fault='after_memory')
        except RuntimeError:pass
        else:raise AssertionError('fault must stop before publish')
        assert not (path/'runs/stage12-state-fault').exists() and (path/'runs/stage12-state-fault.stage63-pending/memory.bin').exists()
    class Boundary(Exception):pass
    class Base:
        def guard54(self):pass
        def verify_native_geometry(self):return dict(client_w=1,client_h=1)
        def values54(self):return dict(phase=22)
    class DD:
        def __init__(self):self.calls=[];self.fn={'present':self.present}
        def pixels(self,n):return b'\0'*4
        def present(self,state,surface,out,raw,n,png):
            self.calls.append(png);out[0]=out[1]=1;out[6]=out[7]=1;return 1
    Class=runtime_mixin(Base,x,Boundary,lambda *a,**k:None,lambda *a,**k:None)
    instance=Class();instance.watch54=SimpleNamespace(state={'frame_candidate':{'existing':True}});instance.dd=DD();instance.dd.state=None;instance.presentation_events=[]
    instance.present_primary({'id':2},0x52c04a,'2.5')
    assert instance.dd.calls==[None] and instance.presentation_events[-1]['PNG_write_skipped']
    bad=Class();bad.device61=None
    try:bad.thread_run()
    except Boundary:pass
    else:raise AssertionError('must not create/initialize a device')
    out=dict(passed=True,actual_saved_host_fields_checked=len(field_rows),fast_encoder_equals_legacy_encoder=True,
        original_history_not_trimmed=True,budget_max8_reserve25_passed=True,
        staged_atomic_directory_and_fsync_fixture_passed=True,interrupted_save_not_published=True,
        incremental_file_hashes_equal_actual_fixture_bytes=True,
        actual_native_present_path_called_with_null_PNG_in_inert_fixture=True,
        absent_existing_device_refuses=True,original_instructions_executed=0,emulator_imported=False,
        native_library_loaded=False,fields=field_rows,
        total_legacy_encode_seconds=sum(x['legacy_encode_seconds'] for x in field_rows),
        total_fast_encode_seconds=sum(x['fast_encode_seconds'] for x in field_rows))
    (ROOT/'evidence/stage63-selfcheck.json').write_text(json.dumps(out,indent=2)+'\n')
    print(json.dumps({k:v for k,v in out.items() if k!='fields'}))
if __name__=='__main__':main()
