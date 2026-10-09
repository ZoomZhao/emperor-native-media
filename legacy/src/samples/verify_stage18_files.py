#!/usr/bin/env python3
"""Finite readonly file contracts: actual EOF and per-resource end-relative seek."""
import json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage18 import PixelsRuntime,CAMPAIGN,ELEVATION,PIXELS,x
from stage7_runtime import GUEST_NAME,SP,Boundary
r=PixelsRuntime('ST9BASE');records=[];sp=SP-0x1000
def invoke(name,args):
 r.uc.reg_write(x.UC_X86_REG_ESP,sp);r.uc.mem_write(sp,struct.pack('<'+'I'*(len(args)+1),0x62000000,*args));r.api('kernel32.dll',name);return r.uc.reg_read(x.UC_X86_REG_EAX)
for path,payload,backing in [(GUEST_NAME,r.save,r.native_save),(CAMPAIGN,r.campaign_payload,r.native_campaign),(ELEVATION,r.elevation_payload,r.native_elevation),(PIXELS,r.pixels_payload,r.native_pixels)]:
 assert backing.read(0,len(payload))==payload and backing.read(len(payload),16)==b''
 h=r.next_handle;r.next_handle+=1;r.files[h]=dict(path=path.lower(),payload=payload,position=0)
 assert invoke('SetFilePointer',[h,0,0,2])==len(payload)
 b=SP-0x2000;n=b+128;r.uc.mem_write(b,bytes([0xa5])*32);assert invoke('ReadFile',[h,b,16,n,0])==1 and r.readword(n)==0 and bytes(r.uc.mem_read(b,32))==bytes([0xa5])*32
 assert invoke('SetFilePointer',[h,0xffffffff,0,2])==len(payload)-1
 assert invoke('ReadFile',[h,b,16,n,0])==1 and r.readword(n)==1 and bytes(r.uc.mem_read(b,1))==payload[-1:]
 before=r.files[h]['position'];assert invoke('SetFilePointer',[h,0xffffffff,0,0])==0xffffffff and r.files[h]['position']==before and r.last_error==131
 try:invoke('SetFilePointer',[h,0,SP-0x2200,0])
 except Boundary:assert r.files[h]['position']==before
 else:raise AssertionError('unmeasured high-word pointer accepted')
 records.append(dict(path=path.decode(),size=len(payload),entire_native_read_equal=True,end_relative_seek_uses_this_resource_size=True,EOF_count_zero_and_destination_untouched=True,last_byte_read_correct=True,negative_seek_refused_without_position_change=True,high_word_pointer_refused_without_position_change=True))
result=dict(all_passed=True,records=records,self_authored_platform_contract_probe=True,original_city_logic_not_executed=True,native_city_passed=False);(ROOT/'evidence/stage18-file-contracts.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));r.dispose()
