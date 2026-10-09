"""Selected round only: Python audit plus inherited macOS kernel policy."""
import os,stat,sys
from pathlib import Path
from common import ROOT,S,V
def check_path(path,out):
 if isinstance(path,int):return
 p=Path(os.fsdecode(path)).absolute();out=Path(out).absolute()
 if '..'in p.parts:raise PermissionError('WRITE_GATE_PARENT_TRAVERSAL')
 for x in (p,*p.parents):
  try:q=x.lstat()
  except FileNotFoundError:continue
  if stat.S_ISLNK(q.st_mode):raise PermissionError('WRITE_GATE_SYMLINK')
  if x==p and stat.S_ISREG(q.st_mode)and q.st_nlink!=1:raise PermissionError('WRITE_GATE_HARDLINK')
 p=p.resolve()
 if not(p==out or p.is_relative_to(out)):raise PermissionError('WRITE_GATE_OUTSIDE_SELECTED_ROUND '+str(p))
def install(out):
 out=Path(out).resolve()
 if out.parent!=S/'runs':raise PermissionError('WRITE_GATE_ROOT')
 def audit(event,args):
  if event=='open':
   p,mode,flags=args
   if (flags or 0)&(os.O_WRONLY|os.O_RDWR|os.O_CREAT|os.O_TRUNC|os.O_APPEND)or mode and any(x in mode for x in 'wax+'):check_path(p,out)
  elif event in ('os.mkdir','os.rmdir','os.remove','os.truncate','os.chmod','os.chown','os.utime','os.setxattr','os.removexattr'):check_path(args[0],out)
  elif event in ('os.rename','os.link','os.symlink'):raise PermissionError('WRITE_GATE_LINK_OR_RENAME_REFUSED')
 sys.addaudithook(audit)
def sandbox_command(out,command):
 return ['/usr/bin/sandbox-exec','-f',str(V/'configs/round-write-policy.sb'),'-D','OUT='+str(Path(out).resolve()),*map(str,command)]
