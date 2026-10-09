"""Full byte-ordered guest CALL/RET/target binding and scalar-memory guards.

All runtime helpers stay unresolved. REP/FS and platform callbacks are explicit
gaps, never declared covered because a scalar LLVM access was instrumented.
"""
import collections,hashlib,json,re,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage81';SOURCE=S/'builds/fresh-full'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 out=S/'builds/full-bound-guarded-open';out.mkdir(exist_ok=False);steps=[]
 def run(label,args):
  with(out/(label+'.stdout.log')).open('xb')as stdout,(out/(label+'.stderr.log')).open('xb')as stderr:q=subprocess.run([str(x)for x in args],cwd=out,stdout=stdout,stderr=stderr,timeout=30)
  steps.append(dict(label=label,command=[str(x)for x in args],returncode=q.returncode));assert q.returncode==0,label
 closure=json.loads((S/'evidence/known-method-completed-closure.json').read_text());code=closure['instructions'];original=json.loads((SOURCE/'result.json').read_text());assert original['flags_fixed_point_converged']
 labels={m[1]:int(m[1][4:],16)for p in SOURCE.glob('*_data.llinc')for m in re.finditer(r'^dlabel (loc_[0-9A-F]+)',p.read_text(),re.M)}
 for p in SOURCE.glob('*_data.llinc'):(out/p.name).write_bytes(p.read_bytes())
 for p in ['extern.llinc','macros.llinc']:(out/p).write_bytes((SOURCE/p).read_bytes())
 idx=0;edits=[];returns=set();ret_count=dynamic_count=data_count=IAT_reads=0
 externs=set(re.findall(r'^proc (\w+) external$',(SOURCE/'combined.llasm').read_text(),re.M))
 for path in sorted(SOURCE.glob('*_code.llinc')):
  q=None;rows=[]
  for line in path.read_text().splitlines():
   if line.startswith(';'):
    q=code[idx];idx+=1;rows+=['call stage72_step '+hex(q['pc']),line];continue
   before=line
   line=re.sub(r'loc_[0-9A-F]+',lambda m:hex(labels[m[0]])if m[0]in labels else m[0],line)
   if line!=before:
    data_count+=1;edits.append(dict(kind='data_label_guest_VA',PC=hex(q['pc']),before=before,after=line))
    if line.startswith(('load ','load8s ','load8z ','load16s ','load16z ','store ','store8 ','store16 ')):
     parts=line.split(',');address=parts[1].strip()
     if address.startswith('0x'):rows.append('mov tmpadr, '+address);parts[1]=' tmpadr';line=','.join(parts)
   # SR's IAT register-load optimisation yields a native procedure address.
   # Restore the original read of the guest IAT cell; do not truncate native PC.
   m=re.fullmatch(r'mov (\w+), (\w+)',line)
   if q and m and m[2]in externs:
    operand=re.fullmatch(r'(\w+), dword ptr \[(\d+)\]',q['operand']);assert q['mnemonic']=='mov'and operand and operand[1]==m[1],(q,line)
    rows+=['mov tmpadr, '+hex(int(operand[2])),'load '+m[1]+', tmpadr, 4'];IAT_reads+=1;edits.append(dict(kind='original_guest_IAT_read',PC=hex(q['pc']),slot=hex(int(operand[2]))));continue
   m=re.fullmatch(r'PUSH loc_([0-9A-F]+)',line)
   if m:
    pc=int(m[1],16);assert q['mnemonic']=='call'and pc==q['end'],(q,line);returns.add(pc);line='PUSH '+hex(pc);edits.append(dict(kind='guest_CALL_return_PC',PC=hex(q['pc']),return_PC=hex(pc)))
   dynamic=re.fullmatch(r'tcall (\w+)',line)
   if q and dynamic and q['mnemonic']in ('call','jmp')and dynamic[1]not in externs and re.fullmatch(r'(?:e[a-z]+|tmp\w+)',dynamic[1]):
    rows+=['call stage77_lookup_target '+hex(q['pc'])+', '+dynamic[1],'tcall tmp0'];dynamic_count+=1;edits.append(dict(kind='dynamic_target_resolver',PC=hex(q['pc'])));continue
   if q and line=='tcall tmp1'and q['mnemonic']=='ret':
    rows+=['call stage72_lookup_return tmp1','tcall tmp0'];ret_count+=1;continue
   rows.append(line)
  (out/path.name).write_text('\n'.join(rows)+'\n')
 assert idx==len(code)and ret_count==sum(q['mnemonic']=='ret'for q in code),(idx,ret_count)
 text=(SOURCE/'combined.llasm').read_text();where=text.index('datasegment');text=text[:where]+'funcv stage72_step pc\nfuncp stage72_lookup_return pc\nfuncp stage77_lookup_target pc, target\n'+text[where:];(out/'bound.llasm').write_text(text)
 run('llasm',[ROOT/'build/bin/llasm','bound.llasm','-I',ROOT/'vendor-src/SR/SR/llasm-support','-m64','-pic','-ptrofs','-O','-o','bound.ll'])
 ir=(out/'bound.ll').read_text();protected=[];pointers={};counter=loads=stores=0;unknown_access=[];native_data=[]
 for line in ir.splitlines():
  if line.startswith('define '):pointers={}
  m=re.match(r'(%\d+) = inttoptr i64 %\d+ to i(8|16|32)\*$',line)
  if m:pointers[m[1]]=int(m[2])//8
  access=re.match(r'(?:%\d+ = )?(load|store) i(8|16|32)(?:, | )(.+)',line)
  if access:
   kind,bits,tail=access.groups();bits=int(bits);m=re.search(r'i\d+\* (%\d+)',tail)
   if m and m[1]in pointers:
    pointer=m[1];size=pointers[pointer];assert size==bits//8;counter+=1;cast='%s77_ptr_'+str(counter)
    protected +=[cast+' = bitcast i'+str(bits)+'* '+pointer+' to i8*','call ccc void @stage72_guard(i8* '+cast+',i32 '+str(size)+',i32 '+str(2 if kind=='store'else 1)+') nounwind',line]
    if kind=='store':
     value=tail.split(',',1)[0]
     if value.startswith('%'):v='%s77_value_'+str(counter);protected.append(v+' = zext i'+str(bits)+' '+value+' to i64');value=v
     else:value=str(int(value)&((1<<bits)-1))
     protected.append('call ccc void @stage72_note_write(i8* '+cast+',i32 '+str(size)+',i64 '+value+') nounwind');stores+=1
    else:loads+=1
    continue
  if 'getelementptr'in line and re.search(r'@_(?:data|rdata|s\w+)',line):native_data.append(line[:200])
  protected.append(line)
 # Create pointer tables inside the module, where private continuation labels
 # are accessible. No source RVA is rebound to a different guest RVA.
 names={}
 for name in re.findall(r'^define \w+ fastcc void @(\w+)\(%_cpu\* %cpu\)',ir,re.M):
  if name.startswith('loc_'):names[int(name[4:],16)]=name
  elif name.startswith('function_'):names[int(name[9:],16)]=name
 names[0x4aeeb0]='entry_phase8_full'
 missing=sorted(returns-set(names));assert not missing,[hex(p)for p in missing]
 bindings=[(pc,names[pc])for pc in sorted(returns)]
 helper='\ndeclare ccc void @stage72_guard(i8*,i32,i32) nounwind\ndeclare ccc void @stage72_note_write(i8*,i32,i64) nounwind\n'
 helper+='@stage72_return_bindings = global [%d x {i32,i8*}] [%s],align 8\n'%(len(bindings),','.join('{i32,i8*} {i32 %d,i8* bitcast(void(%%_cpu*)* @%s to i8*)}'%(pc,name)for pc,name in bindings))
 helper+='@stage72_return_bindings_count = global i32 %d\n'%len(bindings)
 helper+='@stage77_structural_code_bindings = global [%d x {i32,i8*}] [%s],align 8\n'%(len(names),','.join('{i32,i8*} {i32 %d,i8* bitcast(void(%%_cpu*)* @%s to i8*)}'%(pc,name)for pc,name in sorted(names.items())))
 (out/'guarded.ll').write_text('\n'.join(protected)+helper)
 run('clang',['/usr/bin/clang','-Wno-override-module','-O2','-c','guarded.ll','-o','guarded-full.o'])
 heap=ROOT/'stage77/builds/heap-integrated-open';run('archive',['/usr/bin/ar','rcs','phase8-guarded-with-heap-OPEN.a','guarded-full.o',heap/'heap-native.o',heap/'heap-frontend.o',heap/'heap-wrappers.o',ROOT/'stage78/builds/full-helpers-open/guarded-helpers.o',ROOT/'stage78/builds/full-helpers-open/helper-frontend.o',ROOT/'stage79/builds/core-state-open/thread_native.o',ROOT/'stage79/builds/core-state-open/thread_frontend.o',ROOT/'stage79/builds/core-state-open/thread-wrappers.o',ROOT/'stage79/builds/core-state-open/guest_memory.o'])
 report=dict(status='FULL_GUEST_BINDING_SCALAR_GUARDS_AND_HEAP_ARCHIVE_BUILT_OPEN',steps=steps,original_instructions=len(code),original_instruction_bytes_changed=0,instruction_step_markers=idx,guest_CALL_return_PC_edits=len(returns),guest_RETURN_resolvers=ret_count,generic_dynamic_target_resolvers=dynamic_count,original_guest_IAT_read_restorations=IAT_reads,guest_data_VA_bindings=data_count,scalar_guest_memory_load_guards=loads,scalar_guest_memory_store_guards_and_ordered_logs=stores,structural_native_code_bindings=len(names),native_static_data_references_remaining=native_data,REP_and_FS_callback_contracts_compiled=True,REP_and_FS_actual_guest_mapping_integrated=False,per_callsite_allowed_dynamic_target_domains_proved=False,guest_CPU_full_memory_mapping_and_context_restorer_completed=False,unsupported_source_operand_decoder_sites=original['unsupported_operand_decoder_count'],full_relocation_coverage_complete=False,complete_runnable_phase8=False,generated_game_loaded_or_executed=False,archive_sha256=sha(out/'phase8-guarded-with-heap-OPEN.a'),edits=edits)
 (out/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in ['status','guest_CALL_return_PC_edits','guest_RETURN_resolvers','generic_dynamic_target_resolvers','scalar_guest_memory_load_guards','scalar_guest_memory_store_guards_and_ordered_logs','native_static_data_references_remaining']}))
if __name__=='__main__':main()
