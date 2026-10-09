"""Full original dependency-superset SR conversion diagnostic, never execution.

Unimplemented APIs remain unresolved symbols. No fake platform returns, throw
stubs, type pruning or conditional branch pruning is introduced. Arithmetic
flag equivalence remains unproved for this broad diagnostic build.
"""
import argparse,collections,hashlib,json,re,struct,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage78';sys.path.insert(0,str(ROOT/'samples'))
from auto_i386_analysis import PE
from operand_formats import legacy_decode as decode
from build_pe import write_pe
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--closure',required=True);p.add_argument('--flags');p.add_argument('--output',required=True);p.add_argument('--entry',default='0x4aeeb0');p.add_argument('--symbol',default='entry_phase8_full');a=p.parse_args();out=(ROOT/a.output).resolve();assert out.is_relative_to(S/'builds');out.mkdir(exist_ok=False)
 for path,row in json.loads((ROOT/'evidence/stage71-fixed-tool-hashes.json').read_text()).items():assert sha(ROOT/path)==row['sha256']
 pe=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe');assert sha(pe.path)=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
 cfg=json.loads((ROOT/a.closure).read_text());code=cfg['instructions'];pages={};relocs=set();decode_unknown=[]
 def page(addr,codepage=False):
  base=addr&~4095
  if base not in pages:
   section=pe.section(addr);assert section
   b=bytearray(4096)
   if not codepage:
    n=max(0,min(4096,section['va']+section['rawsize']-base))
    if n:b[:n]=pe.read(base,n)
   pages[base]=dict(data=b,flags=0x60000020 if codepage else section['flags'])
  return pages[base]['data']
 for q in code:
  pc=q['pc'];body=bytes.fromhex(q['bytes']);assert pe.read(pc,len(body))==body
  for i,v in enumerate(body):page(pc+i,True)[(pc+i)&4095]=v
  try:
   decoded=decode(pe,pc);assert decoded['end']==q['end']
   for op in decoded['args']:
    if op['kind']=='mem'and op['width']==4 and pe.section(op['disp']):
     relocs.add(op['site']-pe.base);page(op['disp'])
  except (ValueError,AssertionError,KeyError)as e:decode_unknown.append(dict(PC=hex(pc),mnemonic=q['mnemonic'],reason=str(e)))
 # Keep original import descriptors/names/ILT/IAT intact at original RVAs.
 opt=struct.unpack_from('<I',pe.data,60)[0]+24;import_RVA,import_size=struct.unpack_from('<II',pe.data,opt+104);section=pe.section(pe.base+import_RVA)
 for base in range(section['va']&~4095,section['va']+section['rawsize'],4096):page(base)
 all_domains=json.loads((ROOT/'stage75/configs/crt-jump-domains.json').read_text())['domains'];selected_PCs={q['pc']for q in code};domains={p:r for p,r in all_domains.items()if int(p,0)in selected_PCs}
 for row in domains.values():
  for slot in row['rows']:
   address=int(slot['slot'],0)
   for i,v in enumerate(pe.read(address,4)):page(address+i,True)[(address+i)&4095]=v
   relocs.add(address-pe.base)
 # SR has a finite section limit. Merge selected pages within each original PE
 # section, retaining zero padding and explicit ignored code intervals.
 sections=[]
 for original in pe.sections:
  selected=[(base,row)for base,row in sorted(pages.items())if original['va']<=base<original['va']+original['size']]
  if not selected:continue
  low=selected[0][0];high=selected[-1][0]+4096;body=bytearray(high-low)
  for base,row in selected:body[base-low:base-low+4096]=row['data']
  sections.append(dict(name=original['name'],rva=low-pe.base,data=body,flags=original['flags']))
 # SRW rejects ordinal imports even when unreachable. Keep every original
 # import descriptor actually referenced by the analyzed instructions. This
 # changes compiler metadata only; original IAT VAs and source bytes stay fixed.
 def cstring(addr):
  b=bytearray()
  for k in range(512):
   c=pe.read(addr+k,1)[0]
   if not c:return b.decode('ascii')
   b.append(c)
  raise ValueError('import name budget')
 descriptors=[];slot_DLL={}
 for k in range(100):
  raw=pe.read(pe.base+import_RVA+20*k,20);oft,time,forward,name,iat=struct.unpack('<5I',raw)
  if not any((oft,time,forward,name,iat)):break
  dll=cstring(pe.base+name);descriptors.append((dll,raw))
  for j in range(4096):
   thunk=struct.unpack('<I',pe.read(pe.base+(oft or iat)+4*j,4))[0]
   if not thunk:break
   slot_DLL[pe.base+iat+4*j]=dll
 referenced={int(m[1])for q in code for m in re.finditer(r'dword ptr \[(\d+)\]',q['operand'])};required_DLLs={slot_DLL[x]for x in referenced if x in slot_DLL};entry_PC=int(a.entry,0);assert any(q['pc']==entry_PC for q in code)
 directory=b''.join(raw for dll,raw in descriptors if dll in required_DLLs)+bytes(20);directory_RVA=(max(s['rva']+len(s['data'])for s in sections)+4095)&~4095
 sections.append(dict(name='.impdir',rva=directory_RVA,data=bytearray(directory),flags=0x40000040))
 roots=[int(x,0)for x in cfg['function_entries']];aliases={pc:'function_%x'%pc for pc in roots};aliases[entry_PC]=a.symbol
 write_pe(out/'original-superset.exe',sections,entry_PC-pe.base,sorted(relocs),{v:k for k,v in aliases.items()},max_bytes=64<<20)
 image=bytearray((out/'original-superset.exe').read_bytes());struct.pack_into('<II',image,152+104,directory_RVA,len(directory));(out/'original-superset.exe').write_bytes(image)
 fixture=PE(str(out/'original-superset.exe'))
 for q in code:assert fixture.read(q['pc'],q['end']-q['pc'])==bytes.fromhex(q['bytes'])
 ignored=[]
 code_pages={base:dict(flags=row['flags'])for row in sections if row['flags']&0x20 for base in range(pe.base+row['rva'],pe.base+row['rva']+len(row['data']),4096)}
 for base,row in sorted(code_pages.items()):
  if not row['flags']&0x20:continue
  intervals=sorted((max(q['pc'],base),min(q['end'],base+4096))for q in code if q['pc']<base+4096 and q['end']>base);at=base
  for lo,hi in intervals:
   if at<lo:ignored.append((at,lo-at))
   at=max(at,hi)
  if at<base+4096:ignored.append((at,base+4096-at))
 texts={'global_aliases.sci':''.join('loc_%X,%s\n'%(x,y)for x,y in aliases.items()),'ignored_areas.sci':''.join('loc_%X,%d\n'%x for x in ignored),'external_procedures.sci':'','instruction_flags.sci':'','SR.cfg':'esp_dword_aligned=yes\nebp_dword_aligned=no\n','extern.llinc':'','macros.llinc':'','relocations.csv':'','fixup_do_not_interpret_as_code.sci':''.join('loc_%X\n'%int(slot['slot'],0)for row in domains.values()for slot in row['rows'])}
 if a.flags:
  metadata=json.loads((ROOT/a.flags).read_text());texts['instruction_flags.sci']=''.join('loc_%X,%s,%s\n'%(int(q['site'],0),q['sr_mask'],q['sr_input_mask'])for q in metadata['flags']['annotations'])
 for name,text in texts.items():(out/name).write_text(text)
 steps=[]
 def run(label,args):
  with(out/(label+'.stdout.log')).open('xb')as o,(out/(label+'.stderr.log')).open('xb')as e:q=subprocess.run([str(x)for x in args],cwd=out,stdout=o,stderr=e,timeout=30)
  steps.append(dict(label=label,command=[str(x)for x in args],returncode=q.returncode));return q.returncode==0
 sr=run('SRW',[ROOT/'build/bin/SRW-llasm','original-superset.exe','raw.llasm']);fixes=[];fallthroughs=[];table_bindings=[];translation_count=0
 if sr:
  source=(out/'raw.llasm').read_text();aliases_lines=[s for s in source.splitlines(True)if s.startswith('define ')];source=''.join(s for s in source.splitlines(True)if not s.startswith('define '));offset=source.index('datasegment');source=source[:offset]+''.join(aliases_lines)+source[offset:]
  imports={pc:r for pc,r in json.loads((S/'configs/proved-register-IAT.json').read_text()).items()if int(pc,0)in selected_PCs};idx=0
  for path in sorted(out.glob('*_code.llinc')):
   original=path.read_text();rows=[];current=None;opened=False
   for line in original.splitlines():
    if line.startswith('proc '):
     name=line.split()[1]
     if opened:
      next_pc=int(name[4:],16)if name.startswith('loc_')else int(name.rsplit('_',1)[-1],16)
      assert current is not None and current['end']==next_pc and current['mnemonic']not in ('jmp','ret'),(current,name)
      rows+=['tcall '+name,'endp'];fallthroughs.append(dict(from_PC=hex(current['pc']),to_PC=hex(next_pc),original_CFG_fallthrough_verified=True))
     opened=True
    elif line=='endp':opened=False
    if line.startswith(';'):
     if idx<len(code):current=code[idx];idx+=1
    m=re.fullmatch(r'tcall (\w+)',line)
    if current and m and hex(current['pc'])in imports:
     binding=imports[hex(current['pc'])];name=binding['import_name'].split('.')[-1];fixes.append(dict(PC=hex(current['pc']),before=line,after='tcall '+name,source_original_instruction_unchanged=True));line='tcall '+name
    table=re.fullmatch(r'mov tmpadr, loc_([0-9A-F]+)\[(-?\d+)\]',line)
    if table:
     assert current and hex(current['pc'])in domains,(current,line)
     value=int(table[1],16)+int(table[2]);assert value==int(domains[hex(current['pc'])]['table'],0)
     table_bindings.append(dict(PC=hex(current['pc']),original_table_VA=hex(value),before=line,after='mov tmpadr, '+hex(value)))
     line='mov tmpadr, '+hex(value)
    rows.append(line)
   path.with_suffix('.original.llinc').write_text(original);path.write_text('\n'.join(rows)+'\n')
  translation_count=idx;assert idx==len(code),(idx,len(code));assert len(fixes)==len(imports),(fixes,imports)
  (out/'combined.llasm').write_text(source)
  llvm=run('llasm',[ROOT/'build/bin/llasm','combined.llasm','-I',ROOT/'vendor-src/SR/SR/llasm-support','-m64','-pic','-ptrofs','-O','-o','full.ll'])
  if llvm:run('clang-original',['/usr/bin/clang','-Wno-override-module','-O2','-c','full.ll','-o','original-full.o'])
 # Rebuild only existing REAL heap adapters. Keep every unsupported platform
 # API unresolved; the old explicit forbidden API wrappers are not included.
 for src,obj in [('stage10_native_platform.c','platform-profile.o'),('stage11_native_runtime.c','finite-heap.o')]:run('clang-'+obj,['/usr/bin/clang','-std=c11','-O2','-I',ROOT/'samples','-c',ROOT/'samples'/src,'-o',obj])
 real='''include llasm.llinc
include asm-calls.llinc
funcp stage72_lookup_return pc
macro Call_Stage76_Return _numpop
    load tmp0, esp, 4
    add esp, esp, 4 + (\_numpop * 4)
    call stage72_lookup_return tmp0
    tcall tmp0
endm
funcp stage11_HeapAlloc_c heap, flags, size
func stage11_HeapFree_c heap, flags, *pointer
func stage11_HeapSize_c heap, flags, *pointer
funcp stage11_HeapReAlloc_c heap, flags, *pointer, size
'''
 for name,nargs in [('HeapAlloc',3),('HeapFree',3),('HeapSize',3),('HeapReAlloc',4)]:real+='proc %s public\nCall_Asm_Stack%d stage11_%s_c\nCall_Stage76_Return %d\nendp\n'%(name,nargs,name,nargs)
 (out/'real-heap-wrappers.llasm').write_text(real)
 if run('llasm-real-heap',[ROOT/'build/bin/llasm','real-heap-wrappers.llasm','-I',ROOT/'vendor-src/SR/SR/llasm-support','-I',ROOT/'vendor-src/SR/games/Septerra Core/SR-Septerra/llasm','-m64','-pic','-ptrofs','-O','-o','real-heap-wrappers.ll']):run('clang-real-heap',['/usr/bin/clang','-Wno-override-module','-O2','-c','real-heap-wrappers.ll','-o','real-heap-wrappers.o'])
 objects=list(out.glob('*.o'));symbols=[]
 if (out/'original-full.o').exists():
  nm=subprocess.run(['/usr/bin/nm','-u',str(out/'original-full.o')],capture_output=True,text=True,timeout=10);symbols=nm.stdout.splitlines()
 if len(objects)>=3:run('archive',['/usr/bin/ar','rcs','phase8-full-OPEN.a']+[p.name for p in objects])
 report=dict(status='FULL_DEPENDENCY_SR_DIAGNOSTIC_OBJECT_BUILT'if(out/'original-full.o').exists()else'FULL_SR_CONVERSION_BLOCKED_PLATFORM_COMPONENTS_BUILT',scope='full static dependency superset; no type or input condition pruning',source_instruction_bytes_changed=0,input_conditions_added=0,source_instructions=len(code),source_code_bytes=cfg['code_bytes'],source_function_entries=len(roots),sparse_PE_SHA256=sha(out/'original-superset.exe'),compiler_import_descriptor_DLLs=sorted(required_DLLs),compiler_import_selection_basis='all original IAT absolute references in complete selected original code, including register loads; original slots unchanged; unrelated ordinal descriptors excluded from compiler fixture only',reconstructed_relocations=len(relocs),own_decoder_unsupported_sites=decode_unknown,arithmetic_flags_metadata_verified=False,flags_equivalence_claimed=False,flags_fixed_point_converged=metadata.get("statistics",{}).get("converged",False)if a.flags else False,flags_unproved_contracts=len(metadata["flags"]["incoming_or_unknown_contracts"])if a.flags else None,relocation_coverage_complete=False,unsupported_operand_decoder_count=len(decode_unknown),register_import_bindings=fixes,translated_original_instruction_comments=translation_count,automatic_verified_fallthrough_bridges=fallthroughs,automatic_original_guest_table_address_bindings=table_bindings,steps=steps,undefined_original_object_symbols=symbols,objects=[dict(path=str(p),sha256=sha(p))for p in objects],archive_sha256=sha(out/'phase8-full-OPEN.a')if(out/'phase8-full-OPEN.a').exists()else None,real_existing_native_heap_APIs=4,finite_heap_adapter_compatible_with_real_checkpoint=False,ABI_wrappers_have_guest_return_bridge=True,complete_runnable_phase8=False,generated_library_loaded=False,original_or_Unicorn_or_generated_game_executed=False)
 (out/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in ['status','source_instructions','source_code_bytes','source_function_entries','reconstructed_relocations','register_import_bindings','undefined_original_object_symbols','complete_runnable_phase8']}))
if __name__=='__main__':main()
