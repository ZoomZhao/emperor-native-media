import argparse,hashlib,json,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage20 import ThreadsRuntime,x
from stage7_runtime import Boundary
from stage12_state import restore_state,save_state
from stage21_cursor import NativeCursor
class CursorRuntime(ThreadsRuntime):
 extra_snapshot_fields=ThreadsRuntime.extra_snapshot_fields+['cursor_blob','cursor_calls']
 def __init__(self,save):super().__init__(save);self.cursor=NativeCursor();self.cursor_calls=[]
 @property
 def cursor_blob(self):return self.cursor.export()
 @cursor_blob.setter
 def cursor_blob(self,b):self.cursor.restore(b)
 def api(self,dll,name):
  if dll.lower()=='user32.dll'and name=='LoadCursorA'and self.args(2)[0]==0 and self.args(2)[1]in [32513,32644,32645,32646]:
   role=self.args(2)[1];h=self.cursor.load_c(self.cursor.state,role)
   if not h:raise Boundary('native_system_cursor_resource_scope',role=role)
   self.cursor_calls.append(dict(name=name,role=role,handle=h,real_NS_Cursor=True,exact_Windows_art=False,presentation='native IBeam / resize axes / open hand move cursor',onscreen_set=False));self.ret(h,2);return
  if dll.lower()=='user32.dll'and name=='CreateCursor':
   a=self.args(7)
   if a[0]!=0x400000 or a[3:5]!=[32,32]:raise Boundary('finite_native_cursor_scope',args=a)
   masks=[bytes(self.uc.mem_read(p,128))for p in a[5:]];h=self.cursor.create(*a[1:5],*masks)
   if not h:raise Boundary('native_cursor_creation_refused',args=a)
   pixels,hot=self.cursor.pixels(h);assert hot==a[1:3];expected=b''.join(bytes(([255]*3 if not((masks[0][i//8]>>(7-i%8))&1)and((masks[1][i//8]>>(7-i%8))&1)else[0]*3)+[0 if(masks[0][i//8]>>(7-i%8))&1 else 255])for i in range(1024));assert pixels==expected
   item=dict(name=name,args=a,handle=h,mask_sha256=[hashlib.sha256(b).hexdigest()for b in masks],native_RGBA_exact=True,hotspot=hot,onscreen_set=False);self.cursor_calls.append(item);self.api_trace.append(item);self.ret(h,7);return
  if dll.lower()=='user32.dll'and name=='DestroyCursor':
   h=self.args(1)[0]
   if not self.cursor.destroy_c(self.cursor.state,h):raise Boundary('native_cursor_destroy_scope',handle=h)
   self.ret(1,1);return
  super().api(dll,name)
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='threads21');p.add_argument('--tag',default='cursors21');a=p.parse_args();start=time.monotonic();r=CursorRuntime('ST9BASE');r.start_threads(worker_capacity=3);m=restore_state(r,a.resume)
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 s=r.thread_run(35);save_state(r,a.tag,'original WinMain, fixed startup thread group and real native cursor',m['milestones']);out=dict(result=s,elapsed=time.monotonic()-start,boot_events=r.boot_events,cursor_calls=r.cursor_calls,threads=r.thread_summary(),city_root_returned=False,city_scheduler_cycles=0,native_city_passed=False,original_code_patched=False);(ROOT/f'evidence/stage21-{a.tag}.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps({k:v for k,v in out.items()if k!='threads'}));r.cursor.dispose();r.dispose()
if __name__=='__main__':main()
