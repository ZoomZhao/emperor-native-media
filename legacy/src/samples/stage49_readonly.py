"""Owned finite Win32 readonly contract over one frozen, authorized game tree."""
import os,errno,stat,re,json,hashlib,ctypes
from pathlib import Path
from stage46_find import GAME,R
from checkpoint_startup34 import ChunkedSavedFile
class PolicyError(Exception):pass
ERRORS={errno.EACCES:5,errno.EPERM:5,errno.ENOTDIR:3,errno.EMFILE:4,errno.ENFILE:4,errno.ENAMETOOLONG:206,errno.EIO:1117}
def normalize(raw,cwd=b'C:\\Emperor'):
 if not raw or len(raw)>=260 or b'\0'in raw:raise PolicyError('empty, NUL, or unsupported MAX_PATH length')
 try:text=raw.decode('gbk').replace('/','\\');current=cwd.decode('gbk').replace('/','\\')
 except UnicodeError:raise PolicyError('invalid ACP936 path')
 if current.casefold()!='c:\\emperor':raise PolicyError('virtual cwd changed outside currently owned root contract')
 if text.startswith('\\\\'):raise PolicyError('UNC, extended and device namespaces unsupported')
 if len(text)>1 and text[1]==':':
  if text[:3].casefold()!='c:\\':raise PolicyError('outside C drive or ambiguous drive-relative path')
  pieces=text[3:].split('\\')
  if not pieces or pieces[0].casefold()!='emperor':raise PolicyError('outside authorized virtual game tree')
  pieces=pieces[1:]
 elif text.startswith('\\'):
  pieces=text[1:].split('\\')
  if not pieces or pieces[0].casefold()!='emperor':raise PolicyError('rooted path outside game tree')
  pieces=pieces[1:]
 else:pieces=text.split('\\')
 parts=[]
 for component in pieces:
  if component in ['', '.']:continue
  if component=='..':
   if not parts:raise PolicyError('directory escape')
   parts.pop();continue
  if component.endswith((' ','.'))or any(ord(c)<32 or c in ':*?"<>|'for c in component):raise PolicyError('unsupported ambiguous or invalid component')
  base=component.split('.')[0].casefold()
  if base in ['con','prn','aux','nul']or re.fullmatch(r'(com|lpt)[1-9]',base):raise PolicyError('DOS device name')
  if re.search(r'~[0-9]',component):raise PolicyError('unverified DOS short-name alias')
  parts.append(component)
 full=('C:\\Emperor'+('\\'+'\\'.join(parts)if parts else '')).encode('gbk')
 if len(full)>=260:raise PolicyError('normalized MAX_PATH exceeded')
 return parts,full
class NativeFDFile(ChunkedSavedFile):
 def __init__(self,path,fd):
  self.path=Path(path);meta=json.loads((R/'evidence/stage49-readonly-build.json').read_text());lib=Path(meta['library']);assert hashlib.sha256(lib.read_bytes()).hexdigest()==meta['sha256'];self.lib=ctypes.CDLL(str(lib));ptr=ctypes.c_void_p;sz=ctypes.c_size_t;q=ctypes.c_uint64
  for name,(ret,args)in {'delete':(None,[ptr]),'read':(ctypes.c_int,[ptr,q,ptr,sz,ctypes.POINTER(sz)]),'info':(ctypes.c_int,[ptr,ptr,sz])}.items():
   f=getattr(self.lib,'s17_file_'+name);f.restype=ret;f.argtypes=args;setattr(self,name+'_c',f)
  self.attributes_c=self.lib.s17_file_basic_attributes;self.attributes_c.restype=ctypes.c_int;self.attributes_c.argtypes=[ptr,ctypes.POINTER(ctypes.c_uint32)]
  self.convert_c=self.lib.s17_time_convert;self.convert_c.restype=ctypes.c_int;self.convert_c.argtypes=[ctypes.c_uint32,q,ptr,sz]
  self.equal_c=self.lib.s17_identifier_equal;self.equal_c.restype=ctypes.c_int;self.equal_c.argtypes=[ctypes.c_char_p,ctypes.c_char_p,ctypes.POINTER(ctypes.c_uint32)]
  f=self.lib.s49_file_from_fd;f.restype=ptr;f.argtypes=[ctypes.c_int];self.state=f(fd);assert self.state
class FrozenTree:
 def __init__(self):
  self.root=GAME.absolute();assert self.root==self.root.resolve() and not self.root.is_symlink();self.fd=os.open(self.root,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW);s=os.fstat(self.fd);self.identity=(s.st_dev,s.st_ino)
  self.manifest={}
  for q in json.loads((R/'evidence/stage48-postflight.json').read_text())['checks']:
   p=Path(q['path'])
   if p.is_relative_to(self.root):
    key=str(p.relative_to(self.root)).casefold();assert key not in self.manifest or self.manifest[key]==q['sha256'];self.manifest[key]=q['sha256']
  assert len(self.manifest)>1000
 def close(self):
  if self.fd is not None:os.close(self.fd);self.fd=None
 def open(self,parts):
  s=os.fstat(self.fd);assert(s.st_dev,s.st_ino)==self.identity;parent=os.dup(self.fd);actual=[];trace=[]
  try:
   if not parts:return dict(fd=parent,path=self.root,actual=[],stat=os.fstat(parent),trace=trace,error=0)
   for index,component in enumerate(parts):
    with os.scandir(parent)as entries:names=[e.name for e in entries if e.name.casefold()==component.casefold()]
    if len(names)>1:raise PolicyError('ambiguous case-insensitive pathname')
    name=names[0]if names else component;final=index==len(parts)-1;flags=os.O_RDONLY|os.O_NOFOLLOW|(0 if final else os.O_DIRECTORY)
    if names and stat.S_ISLNK(os.stat(name,dir_fd=parent,follow_symlinks=False).st_mode):raise PolicyError('symlink refused before traversal')
    try:child=os.open(name,flags,dir_fd=parent)
    except OSError as e:
     if e.errno==errno.ELOOP:raise PolicyError('symlink refused')
     code=(2 if final else 3)if e.errno==errno.ENOENT else ERRORS.get(e.errno)
     if code is None:raise PolicyError('unreviewed POSIX errno '+str(e.errno))
     trace.append(dict(component=component,actual_name=name,native_errno=e.errno,win32_error=code,final_component=final,parent_device=os.fstat(parent).st_dev,parent_inode=os.fstat(parent).st_ino,actual_readonly_openat=True));os.close(parent);parent=None;return dict(fd=None,path=self.root.joinpath(*(actual+parts[index:])),actual=actual+parts[index:],trace=trace,error=code)
    os.close(parent);parent=child;actual.append(name);st=os.fstat(parent);trace.append(dict(component=component,actual_name=name,native_device=st.st_dev,native_inode=st.st_ino,actual_readonly_openat=True))
    if not final and not stat.S_ISDIR(st.st_mode):raise PolicyError('non-directory path ancestor')
   if not(stat.S_ISREG(st.st_mode)or stat.S_ISDIR(st.st_mode)):raise PolicyError('nonregular file/device refused')
   return dict(fd=parent,path=self.root.joinpath(*actual),actual=actual,stat=st,trace=trace,error=0)
  except:
   if parent is not None:os.close(parent)
   raise
 def bind(self,resolved):
  assert not resolved['error']and stat.S_ISREG(resolved['stat'].st_mode);relative='/'.join(resolved['actual']);wanted=self.manifest.get(relative.casefold())
  if not wanted:raise PolicyError('existing file not in frozen input manifest')
  size=resolved['stat'].st_size
  if size>64*1024*1024:raise PolicyError('existing file exceeds bounded64MiB contract')
  native=NativeFDFile(resolved['path'],resolved['fd'])
  try:
   body=native.read(0,size);got=hashlib.sha256(body).hexdigest()
   if len(body)!=size or got!=wanted:raise PolicyError('frozen file size/hash changed')
   return native,body,wanted
  except:native.dispose();raise
