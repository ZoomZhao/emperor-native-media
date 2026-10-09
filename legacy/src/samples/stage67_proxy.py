"""Python-side own adapter proxy. Remote handles never convert to addresses."""
import base64,ctypes,hashlib,json,os,select,subprocess,time
from dataclasses import dataclass
from pathlib import Path
from stage66_context_test import own_context
ROOT=Path(__file__).resolve().parents[1]
@dataclass(frozen=True)
class Handle:
    session:object
    id:int
class RemoteError(Exception):pass
def pack(v):
 if isinstance(v,bytes):return {'bytes':base64.b64encode(v).decode()}
 if isinstance(v,(tuple,list)):return [pack(x) for x in v]
 if isinstance(v,dict):return {k:pack(x) for k,x in v.items()}
 return v
def unpack(v):
 if isinstance(v,dict) and 'bytes' in v:return base64.b64decode(v['bytes'],validate=True)
 if isinstance(v,list):return [unpack(x) for x in v]
 if isinstance(v,dict):return {k:unpack(x) for k,x in v.items()}
 return v
class Session:
 def __init__(self,label):
  self.sequence=0;self.timings=[];self.error=(ROOT/f'evidence/stage67-{label}-helper.stderr.log').open('w')
  self.process=subprocess.Popen([str(ROOT/'runs/stage67-native/stage67-ui-helper'),str(ROOT/'samples/stage67_server.py')],
      stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=self.error,text=True,start_new_session=False)
  try:
   self.hello=self.call('HELLO');parent=own_context()
   assert self.hello['ppid']==os.getpid() and self.hello['pid']==self.process.pid
   assert all(self.hello[k]==parent[k] for k in ['uid','euid','session_status','session','session_attributes','permissions'])
   assert self.hello['native_UI_entry'] and self.hello['no_guest_modules']
  except Exception:
   self.close();raise
 def call(self,op,**fields):
  start=time.monotonic();self.sequence+=1
  self.process.stdin.write(json.dumps(dict(id=self.sequence,op=op,**fields),separators=(',',':'))+'\n');self.process.stdin.flush()
  if not select.select([self.process.stdout],[],[],8)[0]:raise TimeoutError('own helper response exceeded 8seconds')
  line=self.process.stdout.readline()
  if not line:raise RemoteError('helper exited before '+op)
  r=json.loads(line);assert r['id']==self.sequence
  self.timings.append(dict(op=op,seconds=time.monotonic()-start))
  if not r['ok']:raise RemoteError(r['error']+': '+r['message'])
  return r['result']
 def create(self,kind,*args):return Proxy(self,self.call('NEW',kind=kind,args=list(args)))
 def snapshot(self,objects):return self.call('SCENE_EXPORT',targets=[q.state.id for q in objects])
 def close(self):
  try:
   if self.process.poll() is None:self.call('CLOSE');self.process.stdin.close();self.process.wait(timeout=8)
  finally:
   if self.process.poll() is None:self.process.terminate();self.process.wait(timeout=8)
   self.error.close()
  return self.process.returncode
class FunctionMap:
 def __init__(self,obj):self.obj=obj
 def __getitem__(self,name):
  def call(*args):
   wire=[];buffers={}
   for n,a in enumerate(args):
    if isinstance(a,Handle):
     if a.session is not self.obj.session:raise RemoteError('foreign-session handle')
     wire.append({'state':a.id})
    elif isinstance(a,bytes):wire.append({'cstring':base64.b64encode(a).decode()})
    elif isinstance(a,ctypes.Array) or hasattr(a,'_obj'):
     b=a._obj if hasattr(a,'_obj') else a;size=ctypes.sizeof(b);buffers[n]=(b,size)
     wire.append({'buffer':base64.b64encode(ctypes.string_at(ctypes.addressof(b),size)).decode()})
    elif isinstance(a,ctypes._SimpleCData):wire.append(a.value)
    elif a is None or isinstance(a,(int,float)):wire.append(a)
    else:raise RemoteError('unsupported ABI argument')
   r=self.obj.session.call('FN',target=self.obj.state.id,name=name,args=wire)
   for n,b in r['buffers'].items():
    dest,size=buffers[int(n)];raw=base64.b64decode(b,validate=True);assert len(raw)==size;ctypes.memmove(ctypes.addressof(dest),raw,size)
   value=r['return_value']
   if isinstance(value,dict) and 'remote_window' in value:return Handle(self.obj.session,value['remote_window'])
   return value
  return call
class Proxy:
 def __init__(self,session,i):self.session=session;self.state=Handle(session,i);self.fn=FunctionMap(self)
 def __getattr__(self,name):
  def method(*args):return unpack(self.session.call('METHOD',target=self.state.id,name=name,args=pack(list(args))))
  return method
 def dispose(self):self.session.call('DISPOSE',target=self.state.id);self.state=None
