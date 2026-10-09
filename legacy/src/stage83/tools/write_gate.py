"""Per-process output gate; never changes global macOS security settings."""
import os,stat,sys
from pathlib import Path
from common import ROOT,S
CLAIMS=tuple(S/'evidence'/('original-'+n+'-attempt.claim.json')for n in ('ABC','capture','reference','native'))
def check_path(path,out):
 if isinstance(path,int):return
 p=Path(os.fsdecode(path)).absolute();out=Path(out).absolute()
 if '..'in p.parts:raise PermissionError('WRITE_GATE_PARENT_TRAVERSAL')
 for x in (p,*p.parents):
  try:q=x.lstat()
  except FileNotFoundError:continue
  if stat.S_ISLNK(q.st_mode):raise PermissionError('WRITE_GATE_SYMLINK')
  if x==p and stat.S_ISREG(q.st_mode)and q.st_nlink!=1:raise PermissionError('WRITE_GATE_HARDLINK')
 resolved=p.resolve()
 if not(resolved==out or resolved.is_relative_to(out)or resolved in CLAIMS):raise PermissionError('WRITE_GATE_OUTSIDE_APPROVED_OUTPUT '+str(resolved))
def install(out):
 out=Path(out).resolve()
 if not out.is_relative_to(S/'runs')or out==S/'runs':raise PermissionError('WRITE_GATE_ROOT')
 def audit(event,args):
  if event=='open':
   p,mode,flags=args
   if (flags or 0)&(os.O_WRONLY|os.O_RDWR|os.O_CREAT|os.O_TRUNC|os.O_APPEND)or mode and any(x in mode for x in 'wax+'):check_path(p,out)
  elif event in ('os.mkdir','os.rmdir','os.remove','os.truncate','os.chmod','os.chown','os.utime','os.setxattr','os.removexattr'):check_path(args[0],out)
  elif event in ('os.rename','os.link','os.symlink'):raise PermissionError('WRITE_GATE_LINK_OR_RENAME_REFUSED')
 sys.addaudithook(audit)
def sandbox_command(out,command):
 values={'OUT':str(Path(out).resolve()),**dict(zip(('CLAIM_ROOT','CLAIM_A','CLAIM_B','CLAIM_C'),map(str,CLAIMS)))}
 cmd=['/usr/bin/sandbox-exec','-f',str(S/'configs/abc-write-policy.sb')]
 for k,v in values.items():cmd+=['-D',k+'='+v]
 return cmd+list(map(str,command))
