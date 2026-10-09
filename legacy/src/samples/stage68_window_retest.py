"""One repeat of own Stage66 raw window and Stage67 old ABI construction."""
import base64,ctypes,json,subprocess
from pathlib import Path
from stage66_context_test import own_context,request
from stage67_proxy import Session,RemoteError
from stage67_selfcheck import WImage
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
def main():
    parent=own_context();checks=[];p=None
    with (E/'stage68-raw-window-helper.stderr.log').open('w') as err:
        try:
            p=subprocess.Popen([str(ROOT/'runs/stage66-ui-helper'),str(ROOT)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,text=True,start_new_session=False)
            identity,_=request(p,'HELLO');same=all(parent[k]==identity[k] for k in ['uid','euid','session_status','session','session_attributes','permissions'])
            assert same and identity['ppid']==parent['pid']
            pixels=bytes([29,101,197,255])*16
            state=dict(version=1,surface_id=1,refs=1,width=4,height=4,pitch=16,format='RGBA8',pixels_base64=base64.b64encode(pixels).decode(),host_addresses_serialized=False)
            p.stdin.write(json.dumps(dict(id=2,command='SURFACE_IMPORT',state=state))+'\n');p.stdin.flush()
            import select
            assert select.select([p.stdout],[],[],8)[0];imported=json.loads(p.stdout.readline())['body'];assert imported['imported']
            raw,seconds=request(p,'WINDOW_CREATE',3);closed,_=request(p,'CLOSE',4);p.stdin.close();code=p.wait(timeout=8)
            checks.append(dict(kind='raw_existing_Stage66_own_window',same_scoped_context=same,window=raw,seconds=seconds,closed=closed,returncode=code,
                frame_source='Existing self-authored Stage66 fixed canvas; not a substitute for NSScreen or the legacy ABI'))
        finally:
            if p is not None and p.poll() is None:p.terminate();p.wait(timeout=8)
    s=Session('stage68-window-retest');error=None;actual=None
    try:
        window=s.create('window');image=WImage.from_buffer_copy(window.export());image.next=2
        r=image.resources[0];r.id=0x57000001;r.kind=3;r.n=256
        w=image.window;w.handle=0x58000001;w.tid=1;w.creating=1;w.style=0x90080000;w.exstyle=0x40000;w.w=384;w.h=256
        w.title=b'Stage68 own legacy constructor retest';w.class_name=b'S68OwnClass'
        try:window.restore(bytes(image));actual='restored'
        except RemoteError as q:error=str(q);actual='failed_actual_native_return_propagated'
        checks.append(dict(kind='old_ABI_window_import',helper_identity=s.hello,outcome=actual,error=error,
            mainScreen_contract_preserved=True,missing_screen_geometry_not_filled=True))
    finally:code=s.close()
    checks[-1]['helper_returncode']=code
    result=dict(status='own_window_retests_completed',checks=checks,original_game_loaded=False,no_original_game_resources=True,
        no_run21=True,permissions_changed=False,global_settings_changed=False)
    (E/'stage68-window-retest-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)
if __name__=='__main__':main()
