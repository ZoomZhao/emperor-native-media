"""Full-size own pixel data; native Lock/Unlock/Blt/IsLost, no guest emu_start."""
from core import *
from models import *
import struct
def main():
 u=fixed();uc=u.Uc(4,4);base=0x40000000;uc.mem_map(base,16*1024*1024,3);budget=SharedBudget(1,3000000,30);m=Memory(uc,[(base,base+16*1024*1024,3)],budget);m.tid=1
 p0=base;p32=base+0x500000;p16=base+0x900000;d0=base+0xc00000;d1=d0+112;sr=d0+256;dr=sr+16
 n0=1440*900*4;n32=1024*768*4;n16=1024*768*2;pixels0=b'\xa5'*n0;pixels32=bytes(range(256))*(n32//256);pixels16=b'\x1f\x00'*(n16//2)
 for a,b in ((p0,pixels0),(p32,pixels32),(p16,pixels16),(d0,struct.pack('<I',108)),(d1,struct.pack('<I',108)),(sr,struct.pack('<4i',0,0,1024,768)),(dr,struct.pack('<4i',208,94,1232,862))):uc.mem_write(a,b)
 im=struct.pack('<12I',0x44443239,1,1024,768,555,0,1440,900,32,0,0,0);objects=[]
 for id in range(1,65):
  if id==1:q=(2,1,512,0,0,0,1440,900,5760,32,0,0)
  elif id==2:q=(2,1,2112,0,0,0,1024,768,4096,32,0,0)
  elif id==3:q=(2,1,2112,0,0,0,1024,768,2048,555,0,0)
  else:q=(0,)*12
  objects.append(struct.pack('<12I',*q))
 o0=0x30001020;o32=0x30001030;o16=0x30001040;host={'dd_objects':{o0:{'kind':2,'id':1,'guest_pixels':p0},o32:{'kind':2,'id':2,'guest_pixels':p32},o16:{'kind':2,'id':3,'guest_pixels':p16}},'dd_native_blob':im+b''.join(objects)+pixels0+pixels32+pixels16};bridge=SurfaceBridge(host,m);start=time.monotonic();rows=[]
 for name,args in [('2.25',[o16,0,d0,1,0]),('2.25',[o32,0,d1,1,0]),('2.32',[o32,0]),('2.32',[o16,0]),('2.5',[o0,dr,o32,sr,0,0]),('2.24',[o0])]:
  t=time.monotonic();before=budget.writes;assert bridge.invoke(name,args)==0;rows.append({'method':name,'seconds':time.monotonic()-t,'canonical_writes':budget.writes-before})
 assert budget.writes==2279094 and len(m.trace)==budget.writes and budget.steps==0
 out=m.read(p0,n0);expected=bytearray(pixels0)
 for y in range(768):expected[(94+y)*5760+208*4:(94+y)*5760+(208+1024)*4]=pixels32[y*4096:(y+1)*4096]
 assert out==expected and bridge.last_residency==[1,1,1440,900,5760,0,0]
 groups={}
 for row in m.trace:
  key=row[5];a=row[2];n=row[3];g=groups.setdefault(key,{'records':0,'minimum_address':a,'maximum_exclusive_address':a+n});g['records']+=1;g['minimum_address']=min(g['minimum_address'],a);g['maximum_exclusive_address']=max(g['maximum_exclusive_address'],a+n)
 put(V/'evidence/full-size-memory.json',{'status':'FULL_SIZE_OWN_LOCK_BLT_PERSISTENCE_PASS','methods':rows,'seconds':time.monotonic()-start,'platform_canonical_writes':budget.writes,'write_ranges':groups,'source32_SHA256':hashlib.sha256(pixels32).hexdigest(),'destination_SHA256':hashlib.sha256(out).hexdigest(),'IsLost_real_residency':bridge.last_residency,'own_input_pixels_not_game_assets':True,'emu_start_calls':0,'original_instructions':0,'guest_conversion_instructions_measured':False,'conditional_original_frame_pixel_stores':3145728,'conditional_frame_row_counter_stores':768,'conditional_guest_pixel_instruction_lower_bound':14942208,'conditional_frame_guest_plus_platform_writes':budget.writes+3145728+768,'conditional_plus_Stage73_isolated_actual_writes':budget.writes+3145728+768+49889,'whole_phase7_prefix_instruction_or_wall_bound_proven':False});print(json.dumps({'status':'FULL_SIZE_OWN_LOCK_BLT_PERSISTENCE_PASS','platform_records':budget.writes,'seconds':time.monotonic()-start,'residency':bridge.last_residency}))
if __name__=='__main__':main()
