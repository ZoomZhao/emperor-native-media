"""Permission comparison before permitting helper AppKit/window commands."""
import ctypes,json,os,select,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
def own_context():
    sandbox=ctypes.CDLL('/usr/lib/system/libsystem_sandbox.dylib',use_errno=True);call=sandbox.sandbox_check;call.restype=ctypes.c_int
    # Apple arm64 variadic calls require declaring every fixed argument.
    call.argtypes=[ctypes.c_int,ctypes.c_char_p,ctypes.c_int]
    flag=ctypes.c_int.in_dll(sandbox,'SANDBOX_CHECK_NO_REPORT').value
    cases=[('own_workspace_write','file-write-data',1,str(ROOT/'samples/stage66_helper.m')),
        ('original_workspace_write','file-write-data',1,'<LOCAL_ORIGINAL_PROJECT>'),
        ('system_file_write','file-write-data',1,'/etc/hosts'),
        ('launchservices_lookup','mach-lookup',2,'com.apple.coreservices.launchservicesd'),
        ('windowserver_lookup','mach-lookup',2,'com.apple.windowserver.active'),
        ('user_preferences_lookup','mach-lookup',2,'com.apple.cfprefsd.agent')]
    permissions={'sandbox_present':call(os.getpid(),None,0)}
    for name,operation,kind,value in cases:
        ctypes.set_errno(0);permissions[name]=call(os.getpid(),operation.encode(),kind|flag,ctypes.c_char_p(value.encode()))
        if permissions[name]<0:raise RuntimeError('sandbox query failed: '+name+' errno='+str(ctypes.get_errno()))
    security=ctypes.CDLL('/System/Library/Frameworks/Security.framework/Security');session=security.SessionGetInfo
    session.argtypes=[ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];session.restype=ctypes.c_int32
    sid=ctypes.c_uint32();attrs=ctypes.c_uint32();status=session(0xffffffff,ctypes.byref(sid),ctypes.byref(attrs))
    return dict(pid=os.getpid(),uid=os.getuid(),euid=os.geteuid(),session_status=status,session=sid.value,
        session_attributes=attrs.value,permissions=permissions,AppKit_constructed=False)
def request(process,command,index=1):
    start=time.monotonic();process.stdin.write(json.dumps(dict(id=index,command=command))+'\n');process.stdin.flush()
    if not select.select([process.stdout],[],[],8)[0]:raise TimeoutError('own IPC response exceeded8seconds')
    line=process.stdout.readline()
    if not line:raise RuntimeError('helper exited before response')
    response=json.loads(line);assert response['id']==index
    return response['body'],time.monotonic()-start
def main():
    parent=own_context();started=time.monotonic();p=None
    with (E/'stage66-context-helper.stderr.log').open('w') as error:
        try:
            p=subprocess.Popen([str(ROOT/'runs/stage66-ui-helper'),str(ROOT)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,
                stderr=error,text=True,start_new_session=False)
            child,seconds=request(p,'HELLO')
            same=all(parent[k]==child[k] for k in ['uid','euid','session_status','session','session_attributes','permissions'])
            assert child['ppid']==parent['pid'] and child['pid']==p.pid
            closed,_=request(p,'CLOSE',2);p.stdin.close();code=p.wait(timeout=8)
            record=dict(status='same_context_confirmed' if same else 'context_mismatch_STOP',parent=parent,child=child,
                exact_matrix_match=same,helper_is_direct_child=True,hello_seconds=seconds,process_seconds=time.monotonic()-started,
                helper_normal_exit_code=code,no_AppKit_constructed=True,no_game_loaded=True,
                sandbox_policy_queried_not_changed=True,no_sandbox_extensions_or_credentials_transferred=True,
                matrix_is_scoped_not_all_operations_proof=True)
            (E/'stage66-context-comparison-v2.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record),flush=True)
            assert same and code==0
        finally:
            if p is not None and p.poll() is None:p.terminate();p.wait(timeout=8)
if __name__=='__main__':main()
