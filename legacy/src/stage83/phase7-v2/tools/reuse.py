"""New adapter version; all previous pinned sources remain untouched."""
from core import *
for name in ('models.py','thread_kernel.py','adapter.py','fixture.py'):
 text=(OLD/'tools'/name).read_text()
 if name=='models.py':
  text=text.replace("V/'evidence/build", "OLD/'evidence/build").replace("V/'builds/", "OLD/'builds/")
  text=text.replace('clock=time.monotonic):\n  build=', 'clock=None):\n  from integrate import Clocks\n  build=')
  text=text.replace('self.clock_source=clock;', 'self.clock_source=clock if clock is not None else Clocks().deadline_now;')
 if name=='adapter.py':
  text=text.replace('from thread_kernel import ThreadKernel','from thread_kernel import ThreadKernel\nfrom integrate import Clocks,Keyboard,Observer')
  text=text.replace('self.uc=uc;self.threads=', 'self.clocks=Clocks();self.keyboard=Keyboard(host()["keyboard_snapshot61"]);self.observer=None;self.uc=uc;self.threads=')
  text=text.replace("if self.thread_api(dll,name):return", "if name in ('GetTickCount','timeGetTime','QueryPerformanceCounter','QueryPerformanceFrequency'):\n   value,error=self.clocks.api(name,self.args(1 if name.startswith('Query')else 0),self.last_error,self.m);self.last_error=error;self.ret(value,1 if name.startswith('Query')else 0);return\n  if dll.lower()=='user32.dll'and name=='GetAsyncKeyState':\n   value=self.keyboard.query(self.args(1)[0],self.current_tid,self.readword(self.uc.reg_read(x.UC_X86_REG_ESP)));self.ret(value,1);return\n  if self.thread_api(dll,name):return")
  text=text.replace("g=self.goal_contract\n", "g=self.goal_contract\n   if self.observer:self.observer.code(tid,pc,uc.reg_read(x.UC_X86_REG_ESP),self.readword(g['phase_address']),0,1,self.readword(uc.reg_read(x.UC_X86_REG_ESP)))\n")
  text=text.replace('self.goal=True;uc.emu_stop();return',"if self.observer:self.observer.goal(tid,self.readword(g['phase_address']))\n    self.goal=True;uc.emu_stop();return")
 if name=='fixture.py':
  text=text.replace("('authored.callback','invoke')", "('authored.callback','invoke'),('kernel32.dll','GetTickCount'),('winmm.dll','timeGetTime'),('kernel32.dll','QueryPerformanceCounter'),('kernel32.dll','QueryPerformanceFrequency'),('user32.dll','GetAsyncKeyState')")
  text=text.replace("api('WaitForSingleObjectEx',[done,200,1]);api('GetCurrentThreadId',[])","api('GetTickCount',[]);store(DATA+0x20);api('timeGetTime',[]);store(DATA+0x24);api('QueryPerformanceCounter',[DATA+0x28]);api('QueryPerformanceFrequency',[DATA+0x30]);api('GetAsyncKeyState',[113]);store(DATA+0x38);api('GetCurrentThreadId',[])")
  text=text.replace("struct.pack('<II',phase,8)","struct.pack('<II',DATA+0x108,0xcc)")
  marker=' threads={}';extra=" b,e,api,store,call=program(CODE+0x900);api('WaitForSingleObjectEx',[done,200,1]);e(b'\\xc7\\x05'+struct.pack('<II',phase,8));e(b'\\xc3');uc.mem_write(CODE+0x900,bytes(b))\n"
  text=text.replace(marker,extra+marker)
  text=text.replace("((1,main),(2,", "((1,CODE+0x900),(2,")
  text=text.replace("wait=None\n", "wait=None\n  if tid==1:sp-=4;uc.reg_write(x.UC_X86_REG_ESP,sp);uc.mem_write(sp,struct.pack('<I',main))\n")
  text=text.replace("return r,{'src':", "from integrate import Observer\n seed=json.loads(json.dumps(host()['watch_state54']));seed['frames'][-1]['sp']=r.threads[1]['context'] and STACK+4096-260;seed['frames'][-1]['ret']=main;r.observer=Observer(seed)\n return r,{'src':")
 (V/'tools'/name).write_text(text)
put(V/'evidence/reuse.json',{'previous_sources':{name:sha(OLD/'tools'/name)for name in ('models.py','thread_kernel.py','adapter.py','fixture.py')},'new_sources':{name:sha(V/'tools'/name)for name in ('models.py','thread_kernel.py','adapter.py','fixture.py')},'original_execution':0,'source_input_not_cropped':True,'own_fixture_has_real_pending_subroutine_and_RET_before_clock_keyboard_renderer_and_goal':True})
