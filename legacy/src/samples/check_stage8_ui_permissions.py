#!/usr/bin/env python3
"""Read-only permission preflight. Does not request TCC access or send events."""
import ctypes, json, os, subprocess
from pathlib import Path

root=Path(__file__).resolve().parents[1]
cg=ctypes.CDLL('/System/Library/Frameworks/CoreGraphics.framework/CoreGraphics')
ax=ctypes.CDLL('/System/Library/Frameworks/ApplicationServices.framework/ApplicationServices')
for function in [cg.CGPreflightScreenCaptureAccess,cg.CGPreflightPostEventAccess,ax.AXIsProcessTrusted]:
    function.restype=ctypes.c_bool
ancestors=[];pid=os.getpid();seen=set()
while pid>1 and pid not in seen and len(ancestors)<12:
    seen.add(pid)
    try:
        result=subprocess.run(['/bin/ps','-p',str(pid),'-o','pid=,ppid=,comm='],capture_output=True,text=True,timeout=5)
    except PermissionError as error:
        ancestors.append(dict(query_pid=pid,error=str(error)));break
    if result.returncode!=0:
        ancestors.append(dict(query_pid=pid,error=result.stderr.strip(),returncode=result.returncode));break
    fields=result.stdout.strip().split(None,2)
    if len(fields)<3:break
    ancestors.append(dict(pid=int(fields[0]),ppid=int(fields[1]),executable=fields[2]));pid=int(fields[1])
label=os.environ.get('STAGE8_PREFLIGHT_LABEL','sandboxed')
assert label in ('sandboxed','unsandboxed')
result=dict(label=label,screen_capture_preflight=bool(cg.CGPreflightScreenCaptureAccess()),
            event_post_preflight=bool(cg.CGPreflightPostEventAccess()),
            accessibility_process_trusted=bool(ax.AXIsProcessTrusted()),process_ancestry=ancestors,
            requested_or_changed_TCC_permissions=False,sent_events=False,captured_screen=False,
            responsible_TCC_application='not determined by these process-level probes')
(root/f'evidence/stage8-ui-permissions-{label}.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
