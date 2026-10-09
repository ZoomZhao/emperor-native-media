"""Exact existing clock, controlled input and cumulative Watch adapters."""
from core import *
import ctypes,subprocess
sys.path.insert(0,str(ROOT/'samples'))
from stage61_input import Device61
from stage54_watch import Watch
class Keyboard:
 def __init__(self,snapshot):
  data=json.loads(json.dumps(snapshot));self.meta=copy.deepcopy(data['meta']);self.device=Device61.restore(data['device']);contract=json.loads((ROOT/'evidence/stage61-approved-input-domain.json').read_text())
  if self.device.manifest!=contract or self.device.manifest['timeline']:raise Boundary('CONTROLLED_INPUT_DOMAIN_CHANGED')
  if self.snapshot()!=data:raise Boundary('CONTROLLED_INPUT_RESTORE_NOT_EXACT')
 def query(self,vk,tid,pc):return self.device.query(vk,tid,hex(pc))
 def snapshot(self):return {'device':self.device.snapshot(),'meta':copy.deepcopy(self.meta)}
 def original_physical_query(self,*a):raise Boundary('PHYSICAL_KEYBOARD_PROVIDER_FORBIDDEN')
class Clocks:
 def __init__(self):
  build=json.loads((V/'evidence/clock-build.json').read_text());verify(build['pins']);self.lib=ctypes.CDLL(str(V/'builds/clock.dylib'));self.fn=self.lib.s13_clock_get;self.fn.restype=ctypes.c_int;self.fn.argtypes=[ctypes.c_uint32,ctypes.c_void_p,ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)]
 def clock(self,kind,size,error):
  b=ctypes.create_string_buffer(size)if size else None;v=ctypes.c_uint32();e=ctypes.c_uint32(error)
  if self.fn(kind,b,size,ctypes.byref(v),ctypes.byref(e))!=1:raise Boundary('CLOCK_ABI_SCOPE')
  return v.value,e.value,b.raw if b is not None else b''
 def deadline_now(self):return int.from_bytes(self.clock(6,8,0)[2],'little')/10000000
 def api(self,name,args,error,m):
  kind,size={'GetTickCount':(7,0),'timeGetTime':(7,0),'QueryPerformanceCounter':(6,8),'QueryPerformanceFrequency':(5,8)}[name]
  if len(args)!=(1 if size else 0):raise Boundary('CLOCK_ARGC')
  value,error,body=self.clock(kind,size,error)
  if body:m.write_many([(args[0],body)],name)
  return value,error
class Observer:
 def __init__(self,seed,scheduler=0x4ac2f3,refresh=0x536b20,tail=0x4ac650,wrap=0x4ac4e2):
  self.w=Watch('cycle',json.loads(json.dumps(seed)));self.scheduler=scheduler;self.refresh=refresh;self.tail=tail;self.wrap=wrap;self.initial=self.w.snapshot()
  self._refresh_interests()
 def _refresh_interests(self):self.interests={self.scheduler,self.refresh,self.tail,self.wrap}|{f['ret']for f in self.w.state['frames']}
 def interested(self,pc):return pc in self.interests
 def code(self,tid,pc,sp,phase,gate,screen,stack_word):
  if pc==self.scheduler:self.w.scheduler_entry(tid,sp,stack_word,phase,gate)
  elif pc==self.refresh:self.w.refresh_entry(tid,sp,stack_word,screen)
  elif pc==self.tail:self.w.tail_entry(tid,sp,stack_word,phase)
  elif pc==self.wrap:self.w.wrap_jump(tid,sp,phase)
  else:self.w.returns(tid,pc,sp,phase)
  self._refresh_interests()
 def goal(self,tid,phase):
  s=self.w.snapshot()
  if tid!=1 or phase!=8 or s['phase']!=8 or s['tail_entries']!=self.initial['tail_entries']or s['tail_returns']!=self.initial['tail_returns']or len(s['advances'])!=len(self.initial['advances'])+1:raise Boundary('NATURAL_PHASE8_WATCH_GOAL')
  return s
def build():
 cmd=['/usr/bin/clang','-O2','-dynamiclib',str(ROOT/'samples/stage13_clock.c'),'-o',str(V/'builds/clock.dylib')];q=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30);(V/'evidence/clock-build.stdout').write_bytes(q.stdout);(V/'evidence/clock-build.stderr').write_bytes(q.stderr);assert not q.returncode;put(V/'evidence/clock-build.json',{'command':cmd,'returncode':q.returncode,'pins':{str(p.relative_to(ROOT)):sha(p)for p in (ROOT/'samples/stage13_clock.c',ROOT/'samples/stage13_clock_profile.h',V/'builds/clock.dylib')},'source_reused_unchanged':True,'native_clock':'CLOCK_MONOTONIC global since boot; QPC10M ticks/sec, DWORD milliseconds wrap','original_execution':0})
if __name__=='__main__':build()
