#!/usr/bin/env python3
"""Passive original archive/field observations; never fill city data."""
import hashlib,json,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage18 import PixelsRuntime,x,u
from stage12_state import restore_state,save_state
FIELDS={0x130f988:('population',4),0x130f970:('treasury',4),0xc5cae0:('signed_year',2),0xc82eec:('calendar_counter_pending_UI_binding',4)}
class Observer(PixelsRuntime):
 extra_snapshot_fields=PixelsRuntime.extra_snapshot_fields+['field_writes','archive_field_calls']
 def __init__(self,save):
  super().__init__(save);self.field_writes=[];self.archive_field_calls=[]
  for addr,(label,size) in FIELDS.items():self.uc.hook_add(u.UC_HOOK_MEM_WRITE,self.field_hook,user_data=(addr,label,size),begin=addr,end=addr+size-1)
  self.uc.hook_add(u.UC_HOOK_CODE,self.archive_hook,begin=0x780533,end=0x780533)
 def field_hook(self,uc,access,addr,size,value,meta):
  base,label,width=meta
  if len(self.field_writes)<128:self.field_writes.append(dict(field=label,field_address=hex(base),address=hex(addr),size=size,old_hex=bytes(uc.mem_read(addr,size)).hex(),new_value=value&((1<<(8*size))-1),raw_engine_value=value,original_writer_pc=hex(uc.reg_read(x.UC_X86_REG_EIP))))
 def archive_hook(self,uc,pc,size,unused):
  sp=uc.reg_read(x.UC_X86_REG_ESP);dest=self.readword(sp+4);length=self.readword(sp+8)
  if dest in FIELDS:self.archive_field_calls.append(dict(field=FIELDS[dest][0],destination=hex(dest),length=length,return_pc=hex(self.readword(sp)),archive_this=hex(uc.reg_read(x.UC_X86_REG_ECX))))
def main():
 start=time.monotonic();r=Observer('ST9BASE');m=restore_state(r,'file-open')
 if r.first_save_read is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 s=r.segment()
 while s['status']=='budget_yield' and time.monotonic()-start<40:s=r.segment()
 save_state(r,'fields',m['phase'],m['milestones']+[dict(name='passive fields replay',result=s)])
 values={label:dict(address=hex(addr),raw_hex=bytes(r.uc.mem_read(addr,n)).hex(),unsigned=int.from_bytes(r.uc.mem_read(addr,n),'little'),signed=int.from_bytes(r.uc.mem_read(addr,n),'little',signed=True)) for addr,(label,n) in FIELDS.items()}
 out=dict(elapsed_seconds=round(time.monotonic()-start,3),result=s,field_writes=r.field_writes,archive_field_calls=r.archive_field_calls,field_values=values,original_instructions_not_patched=True,city_data_not_filled=True,source_state='file-open',state='fields',native_city_passed=False,original_root_returned=s['status']=='returned')
 (ROOT/'evidence/stage18-fields-observed.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps({k:v for k,v in out.items() if k!='result'}));r.dispose()
if __name__=='__main__':main()
