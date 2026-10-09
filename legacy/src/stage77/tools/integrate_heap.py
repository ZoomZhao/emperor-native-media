"""Compile the real imported-state heap adapter into an OPEN stage archive.

No library loading, no stub implementations, no old finite-heap object reuse.
"""
import hashlib,json,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage77'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 out=S/'builds/heap-integrated-open';out.mkdir(exist_ok=False);steps=[]
 def run(label,args):
  with(out/(label+'.stdout.log')).open('xb')as stdout,(out/(label+'.stderr.log')).open('xb')as stderr:r=subprocess.run([str(x)for x in args],cwd=out,stdout=stdout,stderr=stderr,timeout=30)
  steps.append(dict(label=label,command=[str(x)for x in args],returncode=r.returncode));assert r.returncode==0,label
 for src,obj in [('heap_native.c','heap-native.o'),('heap_frontend.c','heap-frontend.o')]:run('compile-'+obj,['/usr/bin/clang','-std=c11','-Wall','-Wextra','-Werror','-O2','-c',S/'tools'/src,'-o',obj])
 wrappers='''include llasm.llinc
include asm-calls.llinc
func stage77_HeapAlloc_c heap, flags, size
func stage77_HeapFree_c heap, flags, pointer
func stage77_HeapSize_c heap, flags, pointer
func stage77_HeapReAlloc_c heap, flags, pointer, size
funcp stage72_lookup_return pc
macro Call_Stage77_Return _numpop
 load tmp0, esp, 4
 add esp, esp, 4 + (\\_numpop * 4)
 call stage72_lookup_return tmp0
 tcall tmp0
endm
'''
 for name,n in [('HeapAlloc',3),('HeapFree',3),('HeapSize',3),('HeapReAlloc',4)]:wrappers+='proc %s public\nCall_Asm_Stack%d stage77_%s_c\nCall_Stage77_Return %d\nendp\n'%(name,n,name,n)
 (out/'heap-wrappers.llasm').write_text(wrappers)
 run('llasm-heap',[ROOT/'build/bin/llasm','heap-wrappers.llasm','-I',ROOT/'vendor-src/SR/SR/llasm-support','-I',ROOT/'vendor-src/SR/games/Septerra Core/SR-Septerra/llasm','-m64','-pic','-ptrofs','-O','-o','heap-wrappers.ll'])
 run('clang-heap',['/usr/bin/clang','-Wno-override-module','-O2','-c','heap-wrappers.ll','-o','heap-wrappers.o'])
 original=S/'builds/full-conservative-v1/original-full.o';assert original.is_file()
 run('archive',['/usr/bin/ar','rcs','phase8-with-imported-state-heap-OPEN.a',original,'heap-native.o','heap-frontend.o','heap-wrappers.o'])
 archive=out/'phase8-with-imported-state-heap-OPEN.a'
 nm=subprocess.run(['/usr/bin/nm',str(archive)],capture_output=True,text=True,timeout=10);assert nm.returncode==0
 definitions=set();references=set()
 for line in nm.stdout.splitlines():
  fields=line.split()
  if len(fields)==2 and fields[0]=='U':references.add(fields[1])
  elif len(fields)==3 and fields[1].upper()!='U':definitions.add(fields[2])
 unresolved=sorted(references-definitions)
 assert all('_'+name in definitions for name in ['HeapAlloc','HeapFree','HeapSize','HeapReAlloc']);assert '_stage77_active_heap_context'in unresolved and '_stage77_platform_boundary'in unresolved
 report=dict(status='FULL_ORIGINAL_OBJECT_WITH_REAL_CHECKPOINT_HEAP_ADAPTER_ARCHIVED_OPEN',steps=steps,archive=str(archive),archive_sha256=sha(archive),linked_guest_heap_ABI_symbols=['HeapAlloc','HeapFree','HeapSize','HeapReAlloc'],imported_native_heap_metadata_capacity=1000000,actual_validated_import=18131,actual_private_heap_byte_import=234881024,guest_pointer_values_are_uint32=True,old_64KiB_128_block_heap_objects_included=False,unresolved_external_symbols=unresolved,full_guest_memory_callbacks_and_CPU_binding_completed=False,complete_runnable_phase8=False,archive_loaded_or_game_logic_executed=False)
 (out/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(status=report['status'],unresolved_symbols=len(unresolved),heap_ABI_symbols=report['linked_guest_heap_ABI_symbols'])))
if __name__=='__main__':main()
