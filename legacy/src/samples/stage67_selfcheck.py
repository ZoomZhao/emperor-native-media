"""Own window/DD ABI, detached pixels, staged scene and real GPU tests only."""
import ctypes,hashlib,json,struct,time
from pathlib import Path
from stage67_proxy import Session,Handle,RemoteError,unpack
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
U=ctypes.c_uint32
class Resource(ctypes.Structure):_fields_=[(n,U) for n in ['id','kind','x','y','n','pad']]+[('data',ctypes.c_ubyte*8192)]
class WClass(ctypes.Structure):_fields_=[('atom',U),('words',U*9),('name',ctypes.c_char*128)]
class WInfo(ctypes.Structure):_fields_=[(n,U) for n in ['handle','tid','creating','style','exstyle']]+[(n,ctypes.c_int32) for n in ['x','y','w','h']]+[('title',ctypes.c_char*256),('class_name',ctypes.c_char*128)]
class WImage(ctypes.Structure):_fields_=[(n,U) for n in ['magic','version','next','pad']]+[('resources',Resource*8),('cls',WClass),('window',WInfo)]
def own_window(window):
    image=WImage.from_buffer_copy(window.export());image.next=2
    r=image.resources[0];r.id=0x57000001;r.kind=3;r.n=256
    w=image.window;w.handle=0x58000001;w.tid=1;w.creating=1;w.style=0x90080000;w.exstyle=0x40000;w.w=384;w.h=256
    w.title=b'Stage67 own ABI scene';w.class_name=b'S67OwnClass';window.restore(bytes(image))
    life=struct.pack('<16I',0x4c493237,1,1,1,0,0,1,0x59000001,384,256,1536,1,0,0,0,0)+bytes(384*256*4)
    window.life_restore(life);assert window.life_export()==life
    return window.export(),life
def main():
    started=time.monotonic();a=b=None;checks=[]
    try:
        a=Session('ABI-a');window=a.create('window');image,life=own_window(window)
        pointer=window.fn['pointer'](window.state,0x58000001);assert isinstance(pointer,Handle)
        try:int(pointer)
        except TypeError:checks.append('opaque_remote_window_cannot_be_address')
        else:raise AssertionError('unsafe integer conversion')
        dd=a.create('dd',window.state.id,384,256);device=dd.fn['create'](dd.state,1);assert device
        assert dd.fn['coop'](dd.state,device,0x58000001,8)
        primary=dd.fn['surface'](dd.state,device,384,256,1536,32,0x200);src=dd.fn['surface'](dd.state,device,4,2,8,555,0x840);assert primary and src
        source=struct.pack('<8H',0x7fff,0x7c00,0x3e0,0x1f,0,1,32,1024);dd.pixels(src,source)
        assert dd.fn['lock'](dd.state,src)==1 and dd.fn['lock'](dd.state,src)==0
        assert dd.fn['blt'](dd.state,primary,0,0,src,(ctypes.c_int32*4)(0,0,4,2))==0
        assert dd.fn['unlock'](dd.state,src)==1 and dd.fn['unlock'](dd.state,src)==0
        assert dd.fn['blt'](dd.state,primary,0,0,src,(ctypes.c_int32*4)(0,0,4,2))==1
        body=dd.pixels(primary);values=struct.unpack('<8H',source)
        expected=[0xff000000|((v&0x7c00)<<9)|((v&0x3e0)<<6)|((v&31)<<3) for v in values]
        assert list(struct.unpack_from('<4I',body,0))+list(struct.unpack_from('<4I',body,1536))==expected
        checks+=['lock_unlock_error_codes','locked_Blt_rejected','RGB555_to_BGRA_exact','untouched_row_padding']
        native_out=(U*12)();raw=ctypes.create_string_buffer(384*256*4)
        assert dd.fn['present'](dd.state,primary,native_out,raw,len(raw),None)==1
        assert native_out[5]==0 and native_out[6]>=1 and raw.raw==body and dd.pixels(primary)==body
        assert native_out[8]==native_out[9]==native_out[10]==native_out[11]==0
        checks.append('real_Metal_GPU_zero_mismatch_no_focus_change')
        cursor=a.create('cursor',window.state.id);status,previous=cursor.select(0x57000001);assert status
        cursor_blob=cursor.snapshot();assert cursor_blob
        pump=a.create('pump',window.state.id)
        assert pump.fn['inject'](pump.state,0x200,10,20)==1
        first=pump.peek(0,0x200,0x209);second=pump.peek(0,0x200,0x209);assert first[0]>0 and first[1]==second[1]
        removed=pump.peek(1,0x200,0x209);assert removed[1]==first[1]
        checks.append('real_mouse_MSG_FIFO_no_remove_then_remove')
        snapshot=a.snapshot([window,dd,cursor,pump]);state=unpack(snapshot['state']);before=a.snapshot([window,dd,cursor,pump])
        assert snapshot==before
        # Staging a fresh child preserves the active scene if any import fails.
        b=Session('ABI-b');other=b.create('window')
        invalid=bytearray(image);invalid[0]^=1
        try:other.restore(bytes(invalid))
        except RemoteError:pass
        else:raise AssertionError('invalid image accepted')
        assert a.snapshot([window,dd,cursor,pump])==snapshot
        other.restore(image);other.life_restore(life);dd2=b.create('dd',other.state.id,384,256)
        dd2.restore(dd.export());assert dd2.export()==dd.export() and dd2.info(src)['locked']==0
        cursor2=b.create('cursor',other.state.id);cursor2.restore(cursor_blob);assert cursor2.snapshot()==cursor_blob
        pump2=b.create('pump',other.state.id);pump2.restore(pump.snapshot());pump2.route_restore(pump.route_snapshot())
        assert pump2.snapshot()==pump.snapshot() and pump2.route_snapshot()==pump.route_snapshot()
        checks+=['same_old_window_LifeImage_DD_binary_blobs','invalid_staging_keeps_active_scene_exact','cursor_and_pump_state_exact']
        # New helper owns all pointers; original surface identifiers and refs persist.
        assert dd2.info(primary)==dd.info(primary) and dd2.pixels(primary)==body
        native_out2=(U*12)();raw2=ctypes.create_string_buffer(len(body));assert dd2.fn['present'](dd2.state,primary,native_out2,raw2,len(raw2),None)
        assert native_out2[5]==0 and raw2.raw==body
        diagnostics=[window.diagnostic(),other.diagnostic()]
        assert all(q['main_thread']==1 and q['key_window']==q['NSApp_isActive']==q['activation_requests']==0 for q in diagnostics)
        result=dict(status='own_old_UI_ABI_proxy_scene_passed',checks=checks,seconds=time.monotonic()-started,
            diagnostics=diagnostics,presentation_stats=list(native_out),restored_presentation_stats=list(native_out2),
            snapshot_sha256=snapshot['sha256'],surface_sha256=hashlib.sha256(body).hexdigest(),
            window_blob_bytes=len(image),life_blob_bytes=len(life),DD_blob_bytes=len(dd.export()),
            parent_Python_AppKit=False,no_game_no_Unicorn=True,original_instructions_executed=0,no_process21=True,
            a_timings=a.timings,b_timings=b.timings)
    finally:
        codes=[]
        for s in [b,a]:
            if s is not None:codes.append(s.close())
    assert codes==[0,0];result['helper_normal_exit_codes']=codes
    (E/'stage67-selfcheck.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)
if __name__=='__main__':main()
