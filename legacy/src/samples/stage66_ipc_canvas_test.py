"""Own canvas IPC proof: same sandbox, no AppKit in Python, no game runtime."""
import argparse,base64,hashlib,json,os,select,subprocess,time
from pathlib import Path
from stage66_context_test import own_context
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--width',type=int,default=1024);parser.add_argument('--height',type=int,default=768)
    parser.add_argument('--label',default='stage66');args=parser.parse_args()
    assert args.label.replace('-','').isalnum() and 0<args.width<=4096 and 0<args.height<=4096 and args.width*args.height*4<=32*1024*1024
    assert args.width*args.height*4%8==0
    label=args.label
    started=time.monotonic();parent=own_context();rows=[];wire_rows=[];state=None
    pattern=bytes((17,53,101,255,221,193,137,255))*((args.width*args.height*4)//8)
    expected=hashlib.sha256(pattern).hexdigest()
    authored=dict(version=1,surface_id=1,refs=1,width=args.width,height=args.height,pitch=args.width*4,format='RGBA8',
        pixels_base64=base64.b64encode(pattern).decode(),window_shown=False,host_addresses_serialized=False)
    for ordinal in [1,2]:
        p=None;sequence=0
        with (E/f'{label}-canvas-helper{ordinal}.stderr.log').open('w') as error:
            try:
                p=subprocess.Popen([str(ROOT/'runs/stage66-ui-helper'),str(ROOT)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,
                    stderr=error,text=True,start_new_session=False)
                def request(command,**fields):
                    nonlocal sequence
                    sequence+=1;t=time.monotonic();body=json.dumps(dict(id=sequence,command=command,**fields),separators=(',',':'))+'\n'
                    p.stdin.write(body);p.stdin.flush()
                    if not select.select([p.stdout],[],[],8)[0]:raise TimeoutError('IPC response budget exceeded')
                    response_line=p.stdout.readline()
                    if not response_line:raise RuntimeError('native helper exited before '+command)
                    response=json.loads(response_line);assert response['id']==sequence
                    wire_rows.append(dict(helper=ordinal,command=command,request_bytes=len(body),response_bytes=len(response_line),seconds=time.monotonic()-t))
                    if 'error' in response['body']:raise RuntimeError(response['body']['error'])
                    return response['body']
                child=request('HELLO')
                assert child['ppid']==os.getpid() and child['pid']==p.pid
                assert all(child[k]==parent[k] for k in ['uid','euid','session_status','session','session_attributes','permissions'])
                resume_state=state
                imported=request('SURFACE_IMPORT',state=authored if state is None else state);assert imported['imported']
                exported=request('SURFACE_EXPORT')
                raw=base64.b64decode(exported['pixels_base64'],validate=True);assert raw==pattern and hashlib.sha256(raw).hexdigest()==expected
                assert all(exported[k]==authored[k] for k in ['version','surface_id','refs','width','height','pitch','format','host_addresses_serialized'])
                (E/f'{label}-helper{ordinal}-surface-proof.json').write_text(json.dumps(dict(child=child,pixel_bytes=len(raw),
                    pixel_sha256=expected,metadata_exact=True,readback_exact=True,cross_process_restore=ordinal==2),indent=2)+'\n')
                window=request('WINDOW_CREATE')
                assert window['window_created'] and window['visible'] and not window['key'] and not window['main'] and not window['NSApp_active']
                assert window['draw_calls']>=1 and window['main_thread'] and window['activation_requests']==0
                after=request('HELLO');assert after['AppKit_constructed'] and after['permissions']==parent['permissions']
                state=request('SURFACE_EXPORT');assert base64.b64decode(state['pixels_base64'])==pattern
                if ordinal==2:assert state==resume_state
                closed=request('CLOSE');assert closed['closed'] and not closed['window_visible']
                p.stdin.close();code=p.wait(timeout=8);assert code==0
                rows.append(dict(helper=ordinal,pid=p.pid,normal_exit_code=code,permissions=child['permissions'],window=window,
                    surface_pixel_sha256=expected,surface_bytes=len(pattern),restored_from_first=ordinal==2,closed=closed,
                    permission_matrix_after_UI_unchanged=True,complete_defined_state_restored=ordinal==2))
            except Exception as ex:
                failure=dict(status='own_prototype_failed_STOP',failed_helper=ordinal,error=repr(ex),parent=parent,rows=rows,wire_rows=wire_rows,
                    helper_pid=p.pid if p else None,helper_poll=p.poll() if p else None,
                    game_loaded=False,original_process21_started=False)
                (E/f'{label}-ipc-canvas-failure.json').write_text(json.dumps(failure,indent=2)+'\n');raise
            finally:
                if p is not None and p.poll() is None:p.terminate();p.wait(timeout=8)
    result=dict(status='own_IPC_window_surface_restore_passed',rows=rows,wire_rows=wire_rows,seconds=time.monotonic()-started,
        parent=parent,explicit_pipes_only=True,identical_scoped_permission_matrix=True,AppKit_loaded_in_Python=False,
        no_game_loaded=True,no_original_DLL=True,original_process21_started=False,helper_source_alarm_seconds=45,outer_hard_seconds=60)
    (E/f'{label}-ipc-canvas-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)
if __name__=='__main__':main()
