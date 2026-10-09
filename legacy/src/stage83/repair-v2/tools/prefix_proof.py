"""Static data/stack/Blt proof; no original instruction or adapter execution."""
import json,struct,sys
sys.dont_write_bytecode=True
from common import ROOT,V,put
from binary_host import HostStore
s=json.loads((V/'configs/prepared-source.json').read_text());h=HostStore(V/'prepared-host/host.manifest.json');f=(ROOT/'runs/stage12-state-cycle62p19/memory.bin').open('rb')
def read(a,n=4):
 for r in s['regions']:
  if r['low']<=a and a+n<=r['high']+1:f.seek(r['offset']+a-r['low']);return f.read(n)
 raise ValueError('unmapped')
def word(a):return struct.unpack('<I',read(a))[0]
sp=s['registers']['esp'];assert word(sp+16)==0x52bff0
# Four POP+RET restores SP+20, and ADD SP,32 restores caller's local frame.
after_unlocks_sp=sp+20+32;src_rect_ptr=after_unlocks_sp+8;dst_rect_ptr=after_unlocks_sp+24
src_rect=struct.unpack('<4i',read(src_rect_ptr,16));dst_rect=struct.unpack('<4i',read(dst_rect_ptr,16));assert src_rect==(0,0,1024,768)and dst_rect==(208,94,1232,862)
src_obj=word(0x10de4a8);dst_obj=word(0x10de4ac);vtable=word(dst_obj);target=word(vtable+20);assert tuple(h['imports'][target])==('authored.dd','2.5')
blob=h['dd_native_blob'];sid=h['dd_objects'][src_obj]['id'];did=h['dd_objects'][dst_obj]['id'];source=struct.unpack_from('<12I',blob,48+(sid-1)*48);dest=struct.unpack_from('<12I',blob,48+(did-1)*48);assert source[6:10]==(1024,768,4096,32)and dest[6:10]==(1440,900,5760,32)
current_writes=1591309;minimum_rectangle_records=1024*768;original_full_sync_records=dest[8]*dest[7]//4
put(V/'evidence/fixed-prefix-proof.json',{'status':'FIXED_SOURCE_TO_FIRST_FRAME_HAS_STATIC_WRITE_AND_CYCLE_BLOCKERS','original_instructions_executed':0,'conversion_return_PC':word(sp+16),'caller_local_SP_after_two_stdcall_Unlocks':after_unlocks_sp,'next_Blt_PC':0x52c047,'Blt_target':target,'Blt_import':h['imports'][target],'source_rect_pointer':src_rect_ptr,'source_rect':src_rect,'destination_rect_pointer':dst_rect_ptr,'destination_rect':dst_rect,'source_surface_header':source,'destination_surface_header':dest,'current_conversion_write_records':current_writes,'next_successful_Blt_rectangle_minimum_canonical_4byte_records':minimum_rectangle_records,'current_plus_rectangle_records':current_writes+minimum_rectangle_records,'existing_platform_full_surface_sync_canonical_records':original_full_sync_records,'current_plus_existing_full_sync_records':current_writes+original_full_sync_records,'unchanged_A_write_cap':2000000,'successful_next_Blt_exceeds_cap_even_without_later_logic':True,'Blt_success_not_observed_in_this_task':True,'Blt_writes_conditional_on_same_successful_legacy_contract':True,'whole_prefix_dependency_closure_complete':False,'proven_reasons_no_runtime_plan_with_current_source_scope':['A6M instruction cap already smaller than 7,559,981 conversion prefix','Captured next calls Unlock,Unlock,Blt; only pure Unlock reuse tested; Blt still refuses','Honest next successful full-rectangle Blt crosses unchanged2M write cap','Sourcephase22 targetphase8 requires forbidden cycle wrap; current progress gate refuses before tail'],'unresolved_after_Blt':['remaining refresh consumers/callbacks and state guards','scheduler precheck direct closure and dynamic dispatch certificate','all phase22..50 and0..7 bodies if cross-cycle were separately allowed','old progress observer counters and thread schedule limits']})
print(json.dumps({'current_plus_Blt_minimum_records':current_writes+minimum_rectangle_records,'legacy_full_sync_records':current_writes+original_full_sync_records,'cap':2000000,'original_execution':0}))
