import argparse,hashlib,json,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_cursor21 import CursorRuntime,x
from stage7_runtime import Boundary
from checkpoint_stage12 import u
from stage17_file import NativeSavedFile
from stage12_state import restore_state,save_state
class StartupRuntime(CursorRuntime):
 extra_snapshot_fields=CursorRuntime.extra_snapshot_fields+['startup_file_identity','startup_file_IO','startup_file_calls']
 def __init__(self,save):
  super().__init__(save);self.startup_files={};self.startup_aliases={};self._startup_identity={};self.startup_file_IO=[];self.startup_file_calls=[]
  rows=json.loads((ROOT/'evidence/stage19-resources-hashed.json').read_text())['rows'][-5:]+json.loads((ROOT/'evidence/stage21-additional-inputs.json').read_text())['rows']
  for row in rows:
   p=Path(row['copy_path']);assert p.resolve().is_relative_to(ROOT);body=p.read_bytes();assert len(body)==row['copy_size']and hashlib.sha256(body).hexdigest()==row['copy_sha256'];assert hashlib.sha256(Path(row['original_path']).read_bytes()).hexdigest()==row['original_sha256'];rel=row['relative_path'].replace('/','\\').encode();canonical=b'C:\\Emperor\\'+rel;fd=NativeSavedFile(p);info=fd.info();assert fd.read(0,len(body))==body;self.startup_files[canonical.lower()]=(fd,body);self._startup_identity[canonical.lower()]=(info[0],info[4],info[5],row['copy_sha256']);self.startup_aliases[rel.lower()]=canonical;self.startup_aliases[canonical.lower()]=canonical
  self._startup_snapshot_identity=dict(self._startup_identity)
 @property
 def startup_file_identity(self):return self._startup_snapshot_identity
 @startup_file_identity.setter
 def startup_file_identity(self,v):
  assert v=={k:self._startup_identity[k]for k in v};self._startup_snapshot_identity=dict(v)
 def native_for_handle(self,h):
  f=self.files.get(h,{});p=f.get('path');return self.startup_files[p][0]if p in self.startup_files and f.get('payload')==self.startup_files[p][1]else super().native_for_handle(h)
 def native_for_path(self,p):return self.startup_files[self.startup_aliases[p.lower()].lower()][0]if p.lower()in self.startup_aliases else super().native_for_path(p)
 def record_native_read(self,item,backing):
  if any(v[0]is backing for v in self.startup_files.values()):self.startup_file_IO.append(item)
  else:super().record_native_read(item,backing)
 def api(self,dll,name):
  if dll.lower()=='kernel32.dll'and name=='GetPrivateProfileStringA'and self.cstring(self.args(6)[5]).lower()==b'c:\\emperor\\emperor.ini':
   a=self.args(6);section=self.cstring(a[0]);key=self.cstring(a[1]);measured=json.loads((ROOT/'evidence/stage21-ini-Wine.json').read_text());assert measured['all_passed']
   if section.lower()!=b'config'or key.lower()not in [b'cddrive',b'soundfrags']or a[4]!=10:raise Boundary('present_INI_query_unmeasured_scope',section=section.decode(),key=key.decode(),size=a[4])
   fd,body=self.startup_files[b'c:\\emperor\\emperor.ini'];actual=fd.read(0,len(body));assert actual==body;lines=actual.splitlines();assert lines[0]==b'[Config]';values=dict(line.split(b'=',1)for line in lines[1:]if b'='in line)
   if key.lower()==b'cddrive':value=values[b'CDDrive'];assert value==b'Z'
   else:
    measured=json.loads((ROOT/'evidence/stage21-sound-ini-Wine.json').read_text());assert measured['all_passed']and b'SoundFrags'not in values;value=self.cstring(a[2]);assert value==b'32'
   out=value+b'\0';self.require_region(a[3],len(out),u.UC_PROT_WRITE,'present INI query output');self.uc.mem_write(a[3],out);self.last_error=measured['last_error'];item=dict(name=name,path='C:\\Emperor\\Emperor.ini',key=key.decode(),return_value=len(value),last_error=self.last_error,actual_readonly_bytes=len(actual),sha256=hashlib.sha256(actual).hexdigest(),same_Wine_output_tail_error_contract=True);self.startup_file_calls.append(item);self.api_trace.append(item);self.ret(len(value),6);return
  if dll.lower()=='kernel32.dll'and name in ['CreateFileA','GetFullPathNameA']and self.cstring(self.args(7 if name=='CreateFileA'else 4)[0]).lower()in self.startup_aliases:
   a=self.args(7 if name=='CreateFileA'else 4);path=self.startup_aliases[self.cstring(a[0]).lower()];self._startup_snapshot_identity[path.lower()]=self._startup_identity[path.lower()];item=dict(name=name,path=path.decode(),return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),source_sha256=self._startup_identity[path.lower()][3]);self.startup_file_calls.append(item);self.api_trace.append(item)
   if name=='CreateFileA':
    if a[1]!=0x80000000 or a[2]&~7 or a[4]!=3 or a[5]!=0x80 or a[6]:raise Boundary('startup_fixed_file_access_scope',args=a)
    if a[3]and(self.readword(a[3])!=12 or self.readword(a[3]+4)!=0 or self.readword(a[3]+8)not in (0,1)):raise Boundary('startup_fixed_file_security_scope',args=a)
    if any(f.get('path')==path.lower()for f in self.files.values()):raise Boundary('startup_fixed_file_multiple_handle_scope',path=path.decode())
    h=self.next_handle;self.next_handle+=1;self.files[h]=dict(path=path.lower(),payload=self.startup_files[path.lower()][1],position=0);self.ret(h,7)
   else:
    if a[1]<=len(path):self.ret(len(path)+1,4);return
    self.require_region(a[2],len(path)+1,u.UC_PROT_WRITE,'fixed startup full path');self.uc.mem_write(a[2],path+b'\0')
    if a[3]:self.require_region(a[3],4,u.UC_PROT_WRITE,'fixed startup basename');self.word(a[3],a[2]+path.rfind(b'\\')+1)
    self.ret(len(path),4)
   return
  super().api(dll,name)
 def dispose(self):
  for fd,body in self.startup_files.values():fd.dispose()
  self.cursor.dispose();super().dispose()
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='system21');p.add_argument('--tag',default='startup21');p.add_argument('--quantum',type=int,default=1000);a=p.parse_args();start=time.monotonic();r=StartupRuntime('ST9BASE');r.start_threads(worker_capacity=3);m=restore_state(r,a.resume);r.thread_quantum=a.quantum;assert a.quantum in (100,1000,10000)
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 before=dict(pc=hex(r.uc.reg_read(x.UC_X86_REG_EIP)),registers={k:hex(r.uc.reg_read(v))for k,v in [('esi',x.UC_X86_REG_ESI),('edi',x.UC_X86_REG_EDI),('ebx',x.UC_X86_REG_EBX)]});s=r.thread_run(35,max_steps=200000);save_state(r,a.tag,'original WinMain startup with fixed readonly dependencies',m['milestones']);out=dict(result=s,elapsed=time.monotonic()-start,continuation_before=before,quantum=a.quantum,boot_events=r.boot_events,startup_file_calls=r.startup_file_calls,startup_file_IO=r.startup_file_IO,threads=r.thread_summary(),city_root_returned=False,city_scheduler_cycles=0,native_city_passed=False,original_code_patched=False,Map_version_policy='isolated copy used by approved EN baseline, hash pinned; original and copy not merged or written');(ROOT/f'evidence/stage21-{a.tag}.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps({k:v for k,v in out.items()if k!='threads'}));r.dispose()
if __name__=='__main__':main()
