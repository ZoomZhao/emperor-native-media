"""Read-only original import/register-call grouping and local adapter audit."""
import collections,hashlib,json,re,struct,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage76';sys.path.insert(0,str(ROOT/'samples'))
from auto_i386_analysis import PE
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 pe=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe');assert sha(pe.path)=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
 full=json.loads((ROOT/'stage75/evidence/exact-whole-phase8-closure-v3.json').read_text());ins={q['pc']:q for q in full['instructions']};source=json.loads((ROOT/'stage75/evidence/result.json').read_text())['full_phase8_static']['edges']
 edges={int(r['PC'],0):r for r in source if r['category']!='reviewed_static_jump_domain'}
 groups=collections.defaultdict(list);known={}
 for pc,row in edges.items():
  if row['import_name']:groups[row['import_name']].append(hex(pc))
 # Original register loads, validated from the exact original CFG decode.
 specs=[(0x76de58,0x76de40,'esi',0x7a91f4,'VirtualFree'),(0x76deb3,0x76de40,'esi',0x7a91f4,'VirtualFree'),(0x76e719,0x76e705,'ebp',0x7a91f0,'VirtualAlloc'),(0x76e733,0x76e705,'ebp',0x7a91f0,'VirtualAlloc'),(0x774c6c,0x774c60,'esi',0x7a92a0,'GetProcAddress'),(0x774c7d,0x774c60,'esi',0x7a92a0,'GetProcAddress'),(0x774c8a,0x774c60,'esi',0x7a92a0,'GetProcAddress')]
 for pc,load,reg,slot,name in specs:
  assert ins[pc]['mnemonic']=='call'and ins[pc]['operand']==reg
  assert ins[load]['mnemonic']=='mov'and ins[load]['operand']==reg+', dword ptr ['+str(slot)+']'
  for q in ins.values():
   if load<q['pc']<pc:assert not(q['operand'].startswith(reg+',')and q['mnemonic']not in ('cmp','test','push')),(hex(pc),q)
  known[hex(pc)]=dict(import_name='KERNEL32.dll.'+name,slot=hex(slot),register=reg,original_load_PC=hex(load),load_bytes=ins[load]['bytes'],call_bytes=ins[pc]['bytes'],proof='exact source register load; no original register redefinition; intervening stdcall preserves nonvolatile register is a platform ABI obligation')
  groups['KERNEL32.dll.'+name].append(hex(pc))
 categories={
 'heap_allocation_ownership':(['HeapAlloc','HeapFree','HeapSize','HeapReAlloc'],'samples/stage11_native_runtime.c','real finite native semantics; cannot import checkpoint handles/allocations/heap_next; 64KiB/128 blocks/32 regions; not a complete checkpoint allocator'),
 'synchronization':(['InitializeCriticalSection','EnterCriticalSection','LeaveCriticalSection'],'samples/stage20_threads.py','real cooperative reference ownership/wait semantics; coupled to engine/scheduler; no matching standalone native adapter'),
 'virtual_memory':(['VirtualAlloc','VirtualFree'],None,'no existing local implementation; reservation vs commit/decommit/release/protection and guest arena accounting needed'),
 'exceptions':(['RaiseException'],None,'no implemented SEH dispatch/unwind; FS:[0] bookkeeping does not implement exceptions'),
 'diagnostic_IO':(['GetModuleFileNameA','GetStdHandle','WriteFile'],'samples/checkpoint_stage10.py','GetModuleFileNameA explicitly unimplemented; GetStdHandle only finite no-stdio reference profile; WriteFile supports authored sink/save-specific paths, not CRT stderr'),
 'dynamic_loader':(['LoadLibraryA','GetProcAddress'],'samples/stage7_runtime.py','finite engine thunk registry and reviewed specific DLL exports; no general native User32 loader/export semantics')}
 rows=[]
 for category,(names,path,note)in categories.items():
  apis=[dict(API='KERNEL32.dll.'+n,sites=groups['KERNEL32.dll.'+n],site_count=len(groups['KERNEL32.dll.'+n]))for n in names]
  rows.append(dict(category=category,unique_API_count=len(apis),site_count=sum(len(r['sites'])for r in apis),APIs=apis,existing_source=path,source_SHA256=sha(ROOT/path)if path else None,reuse_assessment=note,complete_native_semantics_for_fixed_checkpoint=False))
 globalspecs=[(0x7689de,0x88aa00,'CRT_exit_function_pointer',1),(0x7703ad,0x1e58524,'CRT_allocation_failure_handler',1),(0x77158e,0x1e58630,'CRT_error_banner_hook',0),(0x77bb25,0x88a3d4,'CXX_new_failure_handler',1)]
 callbacks=[]
 for pc,slot,name,args in globalspecs:
  value=struct.unpack('<I',pe.read(slot,4))[0]if slot<0x900000 else 0
  callbacks.append(dict(PC=hex(pc),slot=hex(slot),classification=name,arguments=args,file_initial_value=hex(value),fixed_global_value_not_universal_runtime_target=True))
 def cstring(addr):
  body=bytearray()
  for k in range(512):
   b=pe.read(addr+k,1)[0]
   if not b:return body.decode('ascii')
   body.append(b)
  raise ValueError('unterminated')
 ui=[dict(PC='0x774c9a',API='USER32.dll.'+cstring(0x7cec88),result_slot='0x1e586ac'),dict(PC='0x774cac',API='USER32.dll.'+cstring(0x7cec74),result_slot='0x1e586b0'),dict(PC='0x774cbd',API='USER32.dll.'+cstring(0x7cec98),result_slot='0x1e586a8')]
 assert cstring(0x7ceca4).lower()=='user32.dll'
 remaining=set(edges)-{int(x,0)for g in groups.values()for x in g};assert len(remaining)==10
 report=dict(status='FULL_PHASE8_CALL_PARTITION_NO_EXECUTION',original_IAT_sites=29,original_unique_IAT_APIs=14,original_non_IAT_dynamic_sites=17,proved_register_IAT_sites=7,known_platform_call_sites=36,known_unique_kernel32_APIs=len(groups),groups=rows,register_IAT_proofs=known,business_virtual_calls=[dict(PC=hex(p),operand=ins[p]['operand'])for p in [0x4aef47,0x4aef92,0x4aefaa]],CRT_global_callbacks=callbacks,User32_dynamic_exports=ui,remaining_ten_site_partition=dict(business_virtual=3,CRT_global_handlers=4,User32_dynamic_export_calls=3),remaining_runtime_semantics_unimplemented=True,new_original_or_Unicorn_or_generated_library_runs=0,original_FS_access_sites=[hex(q['pc'])for q in ins.values()if'fs:'in q['operand']])
 (S/'evidence/call-partition.json').write_text(json.dumps(report,indent=2)+'\n');(S/'configs/proved-register-IAT.json').write_text(json.dumps(known,indent=2)+'\n')
 print(json.dumps(dict(platform_sites=36,kernel32_APIs=len(groups),remaining=report['remaining_ten_site_partition'],groups=[{k:r[k]for k in ['category','unique_API_count','site_count']}for r in rows],UI=ui,CRT=callbacks)))
if __name__=='__main__':main()
