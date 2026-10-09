"""Read current object, immutable PE table and registration references only."""
from core import *
sys.path.insert(0,str(ROOT/'samples'))
from auto_i386_analysis import PE,decode as dis
p=json.loads((OLD/'evidence/cpu-restore.json').read_text())['thread_records']['4']['public_CPU'];h=host();pe=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe');base=p['EDI'];count=int.from_bytes(read_guest(base+44,4),'little');obj=int.from_bytes(read_guest(base+48,4),'little');vt=int.from_bytes(read_guest(obj,4),'little')if obj else None;target=int.from_bytes(read_guest(vt,4),'little')if vt else None
def rows(pc,n):
 out=[]
 for _ in range(n):
  try:i=dis(pe,pc);out.append(i);pc=i['end']
  except Exception as e:out.append({'pc':pc,'boundary':str(e)});break
 return out
refs=[]
if vt:
 pattern=struct.pack('<I',vt);at=0
 while True:
  at=pe.data.find(pattern,at)
  if at<0:break
  section=next((s for s in pe.sections if s['rawptr']<=at<s['rawptr']+s['rawsize']),None)
  if section:refs.append({'address':section['va']+at-section['rawptr'],'section':section['name'],'flags':section['flags'],'bytes_around_immediate':pe.data[max(0,at-3):at+12].hex()})
  at+=1
abi={'site':0x5fc139,'opcode':pe.read(0x5fc139,2).hex(),'ECX_object_from_ESI':True,'stack_u32_flag':1,'CALL_pushes_guest_return':0x5fc13b,'callee_pop_4bytes_expected_from_caller_but_not_callee_verified':True,'caller_flags_or_x87_equivalence_not_inferred':True}
result={'status':'CURRENT_OBJECT_TABLE_AND_CALL_ABI_REVIEW','source_pins':PINS,'thread4_current_PC':p['EIP'],'queue_base':base,'current_queue_count':count,'current_pending_object_slot':base+48,'current_object':obj,'current_object_vtable':vt,'current_table_target':target,'table_slot_readonly_section':pe.section(vt)if vt else None,'table_first_bytes':pe.read(vt,16).hex()if vt else None,'vtable_literal_references':refs,'object_size_first_32bytes':read_guest(obj,32).hex()if obj else None,'call_ABI':abi,'caller_rows':rows(0x5fc11a,20),'target_rows':rows(target,45)if target else [],'registration_proven':False,'all_future_queue_object_types_proven':False,'original_execution':0}
put(V/'evidence/callback-proof.json',result);print(json.dumps({k:result[k]for k in ('current_queue_count','current_object','current_object_vtable','current_table_target','vtable_literal_references','call_ABI')}))
