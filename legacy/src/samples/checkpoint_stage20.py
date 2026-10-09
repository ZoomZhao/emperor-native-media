#!/usr/bin/env python3
import argparse,json,os,struct,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage19 import BootRuntime,x,u
from stage20_threads import ThreadKernel
from stage7_runtime import Boundary,SP,SENTINEL,THUNKS
from stage12_state import restore_state,save_state
from auto_i386_analysis import PE
class ThreadsRuntime(ThreadKernel,BootRuntime):
 extra_snapshot_fields=BootRuntime.extra_snapshot_fields+['guest_threads20','thread_owners','current_tid','next_tid','thread_log','thread_quantum','worker_first','thread_active','thread_prefer','thread_waits','thread_switches','process_done','worker_capacity']
 def __init__(self,save):super().__init__(save);self.thread_active=False
 @property
 def guest_threads20(self):
  return {tid:{k:(v.__getstate__()if k=='context'else v)for k,v in t.items()}for tid,t in self.threads.items()}
 @guest_threads20.setter
 def guest_threads20(self,records):
  cls=type(self.uc.context_save());self.threads={}
  for tid,record in records.items():
   t=dict(record);state=t['context'];assert state[1]==len(state[0])and state[2]==u.UC_ARCH_X86 and state[3]==u.UC_MODE_32;c=cls.__new__(cls);c.__setstate__(state);t['context']=c;self.threads[tid]=t
 def thread_code_section(self,address):
  section=self.pe.section(address);return section if section and section['flags']&0x20000000 else None
 def api(self,dll,name):
  if self.thread_active and self.thread_api(dll,name):return
  super().api(dll,name)
 def start_threads(self,quantum=1000,worker_first=False,worker_capacity=2):self.activate_threads(quantum,worker_first,worker_capacity);self.thread_active=True
class SampleRuntime(ThreadsRuntime):
 def __init__(self,meta,tag):
  super().__init__('ST9BASE');self.authored=PE(str(ROOT/'samples/stage10-thread20.exe'));self.sample_meta=meta;self.sample_sink=ROOT/'runs'/('stage20-'+tag+'.bin');self.sink_fd=None;self.sink_payload=None
  self.map(self.authored.base,4096,u.UC_PROT_READ);self.uc.mem_write(self.authored.base,self.authored.data[:4096])
  for s in self.authored.sections:
   perms=u.UC_PROT_READ|(u.UC_PROT_WRITE if s['flags']&0x80000000 else 0)|(u.UC_PROT_EXEC if s['flags']&0x20000000 else 0);self.map(s['va'],max(s['size'],s['rawsize']),perms);self.uc.mem_write(s['va'],self.authored.data[s['rawptr']:s['rawptr']+s['rawsize']])
  from inspect_pe import inspect
  for dll in inspect(ROOT/'samples/stage10-thread20.exe')['imports']:
   for symbol in dll['symbols']:
    name=symbol['name'];address=next((p for p,(d,n)in self.imports.items()if d.lower()==dll['dll'].lower()and n==name),None)
    if address is None:address=THUNKS+len(self.imports)*16;self.imports[address]=(dll['dll'],name);self.uc.mem_write(address,b'\x0f\x0b')
    self.word(int(symbol['iat_va'],16),address)
  self.uc.mem_write(SP-4,struct.pack('<I',SENTINEL));self.uc.reg_write(x.UC_X86_REG_ESP,SP-4);self.uc.reg_write(x.UC_X86_REG_EIP,meta['entry']);self.enable_sparse()
 def thread_code_section(self,address):return self.authored.section(address)if getattr(self,'authored',None)else super().thread_code_section(address)
 def validate_block(self,uc,address,size,user):
  s=self.authored.section(address)
  if s and s['flags']&0x20000000 and address+size<=s['va']+s['size']:return
  super().validate_block(uc,address,size,user)
 def api(self,dll,name):
  if dll.lower()=='kernel32.dll' and name=='CreateFileA':
   a=self.args(7)
   if self.cstring(a[0])!=b'C:\\EmperorRotMK[ZeaS]\\ST10-thread20.bin' or a[1:]!=[0x40000000,0,0,2,0x80,0]:raise Boundary('authored_output_file_scope')
   self.sink_fd=os.open(self.sample_sink,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600);self.ret(0x54000001,7);return
  if dll.lower()=='kernel32.dll' and name=='WriteFile':
   a=self.args(5)
   if a[0]!=0x54000001 or a[2]!=self.sample_meta['output_size']or a[4]:raise Boundary('authored_output_write_scope')
   self.sink_payload=bytes(self.uc.mem_read(a[1],a[2]));n=os.write(self.sink_fd,self.sink_payload);assert n==a[2];self.word(a[3],n);self.ret(1,5);return
  if dll.lower()=='kernel32.dll' and name=='CloseHandle' and self.args(1)[0]==0x54000001:os.close(self.sink_fd);self.sink_fd=None;self.ret(1,1);return
  if dll.lower()=='kernel32.dll' and name=='ExitProcess':
   if any(t['status']!='exited'for tid,t in self.threads.items()if tid!=1):raise Boundary('authored_process_exit_with_live_worker')
   self.process_done=True;self.uc.emu_stop();return
  super().api(dll,name)
def main():
 p=argparse.ArgumentParser();p.add_argument('--sample',action='store_true');p.add_argument('--quantum',type=int,default=1000);p.add_argument('--worker-first',action='store_true');p.add_argument('--tag',default='thread');a=p.parse_args();started=time.monotonic()
 if a.sample:
  from stage20_sample import assess
  m=json.loads((ROOT/'evidence/stage10-thread20-source.json').read_text());r=SampleRuntime(m,a.tag);r.start_threads(a.quantum,a.worker_first);s=r.thread_run(20);body=r.sink_payload;assessment=assess(body,m)if body else dict(all_passed=False);assert not body or r.sample_sink.read_bytes()==body;output=dict(sample=True,assessment=assessment,original_game_code_executed=False)
 else:
  r=ThreadsRuntime('ST9BASE');prior=restore_state(r,'boot-version');r.uc.hook_del(r.distance_hook);r.distance_hook=None;r.start_threads(a.quantum,a.worker_first);s=r.thread_run(30);save_state(r,a.tag,'bounded original WinMain with finite guest threads',prior['milestones']);output=dict(sample=False,source='boot-version',state=a.tag,boot_events=r.boot_events,original_city_root_returned=False,scheduler_city_cycles=0,original_code_patched=False,thread_calls_skipped=False,game_fields_filled=False,fields_state_equivalence_proven=False,population=r.readword(0x130f988),treasury=r.readword(0x130f970))
 output.update(elapsed_seconds=round(time.monotonic()-started,3),result=s,thread_log=r.thread_log,threads=r.thread_summary(),thread_switches=r.thread_switches,thread_waits=r.thread_waits,policy=r.guest_thread_metadata);(ROOT/f'evidence/stage20-{a.tag}.json').write_text(json.dumps(output,indent=2)+'\n');print(json.dumps({k:v for k,v in output.items()if k not in ['threads','thread_log','boot_events']},ensure_ascii=False));r.dispose()
 if a.sample:assert s['status']=='process_complete'and assessment['all_passed'],output
if __name__=='__main__':main()
