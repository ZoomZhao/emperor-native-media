import argparse,hashlib,json,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_startup21 import StartupRuntime,x
from stage22_ini import NativeINI
from stage7_runtime import Boundary
from checkpoint_stage12 import u
from stage12_state import restore_state,save_state
class ProfileRuntime(StartupRuntime):
 extra_snapshot_fields=StartupRuntime.extra_snapshot_fields+['ini_calls']
 def __init__(self,save):super().__init__(save);self.ini=NativeINI();self.ini_calls=[];m=json.loads((ROOT/'evidence/stage22-ini-native-regression.json').read_text());assert m['all_passed']and m['source_sha256']==self._startup_identity[b'c:\\emperor\\emperor.ini'][3]
 def api(self,dll,name):
  if dll.lower()=='kernel32.dll'and name in ['GetPrivateProfileStringA','GetPrivateProfileIntA']:
   a=self.args(6 if name=='GetPrivateProfileStringA'else 4);path=self.cstring(a[-1])
   if path.lower()==b'c:\\emperor\\emperor.ini':
    fd,expected=self.startup_files[path.lower()];body=fd.read(0,len(expected));assert body==expected;section=self.cstring(a[0])if a[0]else None;key=self.cstring(a[1])if a[1]else None;item=dict(name=name,section=section.decode()if section else None,key=key.decode()if key else None,path=path.decode(),readonly_source_sha256=hashlib.sha256(body).hexdigest(),return_pc=hex(self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))),backend='native arm64 generic ASCII INI parser; 36 same-Wine queries matched')
    if name=='GetPrivateProfileStringA':
     if a[4]>512:raise Boundary('fixed_INI_buffer_scope',size=a[4])
     fallback=self.cstring(a[2],512)if a[2]else None;self.require_region(a[3],max(a[4],1),u.UC_PROT_READ|u.UC_PROT_WRITE,'INI buffer');initial=bytes(self.uc.mem_read(a[3],a[4]));v,e,result=self.ini.string(body,section,key,fallback,a[4],initial);assert len(result)==a[4]
     if a[4]:self.uc.mem_write(a[3],result)
     item.update(fallback=fallback.decode()if fallback is not None else None,size=a[4],output_hex=result.hex())
    else:v,e=self.ini.integer(body,section,key,a[2]);item['integer_default']=a[2]
    self.last_error=e;item.update(return_value=v,last_error=e);self.ini_calls.append(item);self.api_trace.append(item);self.ret(v,len(a));return
  super().api(dll,name)
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='next21');p.add_argument('--tag',default='profile22');a=p.parse_args();start=time.monotonic();r=ProfileRuntime('ST9BASE');r.start_threads(worker_capacity=3);m=restore_state(r,a.resume);r.thread_quantum=10000
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 s=r.thread_run(35,max_steps=200000);save_state(r,a.tag,'original WinMain with generic fixed-baseline INI reader',m['milestones']);out=dict(source=a.resume,state=a.tag,result=s,elapsed=time.monotonic()-start,boot_events=r.boot_events,ini_calls=r.ini_calls,threads=r.thread_summary(),city_root_returned=False,city_scheduler_cycles=0,native_city_passed=False,original_code_patched=False);(ROOT/f'evidence/stage22-{a.tag}.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps({k:v for k,v in out.items()if k!='threads'}));r.dispose()
if __name__=='__main__':main()
