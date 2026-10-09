"""Build-only fixed SR guest-VA/return bridge. Never imports an emulator."""
import argparse,hashlib,json,re,shutil,struct,subprocess,sys
from prepare import prepare
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
E=R=None
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
ATTEMPT='build'
def command(label,args):
    label=label+'-'+ATTEMPT
    with(E/(label+'.stdout.log')).open('wb')as o,(E/(label+'.stderr.log')).open('wb')as e:
        q=subprocess.run([str(x)for x in args],cwd=R,stdout=o,stderr=e,timeout=30)
    assert q.returncode==0,(label,q.returncode)
    return dict(command=[str(x)for x in args],cwd=str(R),returncode=q.returncode)
def main():
    global E,R
    parser=argparse.ArgumentParser(description='Configuration-driven SR build; never loads generated code')
    parser.add_argument('--config',required=True);parser.add_argument('--output',required=True)
    args=parser.parse_args();R=(ROOT/args.output).resolve()
    assert R.is_relative_to(ROOT/'stage74/builds')
    for path,row in json.loads((ROOT/'evidence/stage71-fixed-tool-hashes.json').read_text()).items():assert sha(ROOT/path)==row['sha256']
    cfg,a,preparation=prepare(ROOT/args.config,R);E=R
    code=sorted(a['instructions'],key=lambda q:q['pc'])
    b=(R/'guestVA-six.exe').read_bytes();pe=struct.unpack_from('<I',b,60)[0];opt=pe+24
    prior_reloc=struct.unpack_from('<II',b,opt+96+8*5)
    steps=[command('stage73-SRW-rawVA',[ROOT/'build/bin/SRW-llasm','guestVA-six.exe','raw.llasm'])]
    data_labels={m[1]:int(m[1][4:],16)for p in R.glob('*_data.llinc')for m in re.finditer(r'^dlabel (loc_[A-F0-9]+)',p.read_text(),re.M)}
    idx=0;edits=[];returnPCs={q['end']for q in code if q['name']in ('call','call_indirect')};pushedPCs=set()
    for path in sorted(R.glob('*_code.llinc')):
        original=path.read_text();out=[]
        for line in original.splitlines():
            before=line
            line=re.sub(r'loc_[A-F0-9]+',lambda m:hex(data_labels[m[0]])if m[0]in data_labels else m[0],line)
            if line!=before and not line.startswith(';'):
                if line.startswith(('load ','load16s ','load16z ','load8s ','load8z ','store ','store16 ','store8 ')):
                    parts=line.split(',');address=parts[1].strip()
                    if address.startswith('0x'):
                        assert parts[0].split()[-1]!='tmpadr',before
                        out.append('mov tmpadr, '+address);parts[1]=' tmpadr';line=','.join(parts)
                edits.append(dict(kind='data_label_to_original_guest_VA',before=before,after=line))
            if line.startswith(';'):
                q=code[idx];idx+=1;mn=line[1:].split()[0]
                assert mn==q['name']or q['name']=='rep_stos'and mn=='rep'or q['name']=='jcc'and mn.startswith('j')or q['name']=='setcc'and mn.startswith('set')or q['name']in ('call_indirect','jmp_indirect')and mn==q['name'].split('_')[0],(hex(q['pc']),line,q['name'])
                out.append('call stage72_step 0x%x'%q['pc']);out.append(line);continue
            m=re.fullmatch(r'PUSH loc_([0-9A-F]+)',line)
            if m:
                pc=int(m[1],16);assert pc in returnPCs;pushedPCs.add(pc)
                replacement='PUSH 0x%x'%pc;edits.append(dict(kind='guest_CALL_return_binding',before=line,after=replacement,guest_pc=hex(pc)));line=replacement
            if line=='tcall tmp1'and q['name']=='ret':
                edits.append(dict(kind='guest_RET_resolver',before=line,after='call stage72_lookup_return tmp1 / tcall tmp0'))
                out+=['call stage72_lookup_return tmp1','tcall tmp0'];continue
            out.append(line)
        path.with_suffix('.original.llinc').write_text(original);path.write_text('\n'.join(out)+'\n')
    assert idx==len(code),(idx,len(code))
    text=(R/'raw.llasm').read_text().splitlines(True);aliases=[s for s in text if s.startswith('define ')];text=[s for s in text if not s.startswith('define ')];i=next(i for i,s in enumerate(text)if s.startswith('datasegment'))
    text[i:i]=aliases+['funcv stage72_step pc\n','funcp stage72_lookup_return pc\n'];(R/'bound.llasm').write_text(''.join(text))
    steps.append(command('stage73-llasm-bound',[ROOT/'build/bin/llasm','bound.llasm','-I',ROOT/'vendor-src/SR/SR/llasm-support','-m64','-pic','-ptrofs','-O','-o','unprotected.ll']))
    ir=(R/'unprotected.ll').read_text();lines=ir.splitlines();protected=[];ptrs={};n=0;loads=stores=0;function_data_references=[]
    for line in lines:
        if line.startswith('define '):ptrs={}
        m=re.match(r'(%\d+) = inttoptr i64 %\d+ to i(8|16|32)\*$',line)
        if m:ptrs[m[1]]=int(m[2])//8
        access=re.match(r'(?:%\d+ = )?(load|store) i(8|16|32)(?:, | )(.+)',line)
        if access:
            kind,bits,tail=access.groups();bits=int(bits);pm=re.search(r'i\d+\* (%\d+)',tail)
            if pm and pm[1]in ptrs:
                pointer=pm[1];size=ptrs[pointer];assert size==bits//8;n+=1;cast=f'%s73_ptr_{n}'
                protected+=[f'{cast} = bitcast i{bits}* {pointer} to i8*',f'call ccc void @stage72_guard(i8* {cast},i32 {size},i32 {2 if kind=="store"else 1}) nounwind']
                protected.append(line)
                if kind=='store':
                    value=tail.split(',',1)[0];v=f'%s73_value_{n}'
                    if value.startswith('%'):protected.append(f'{v} = zext i{bits} {value} to i64');value=v
                    else:value=str(int(value)&((1<<bits)-1))
                    protected.append(f'call ccc void @stage72_note_write(i8* {cast},i32 {size},i64 {value}) nounwind');stores+=1
                else:loads+=1
                continue
        protected.append(line)
    assert loads+stores>0,(loads,stores)
    for body in re.findall(r'^define .*?^}',ir,re.M|re.S):
        assert not re.search(r'getelementptr.*@_s',body),'native static data label remains in guest function'
    bindings=[(pc,'loc_'+format(pc,'X'))for pc in sorted(pushedPCs)]+[(int(cfg['stop_PC'],0),'stage73_stop')]
    entry='''
declare ccc void @stage72_guard(i8*,i32,i32) nounwind
declare ccc void @stage72_note_write(i8*,i32,i64) nounwind
declare ccc void @stage72_stop_c(%_cpu*) nounwind
define fastcc void @stage73_stop(%_cpu* %cpu) nounwind {
call ccc void @stage72_stop_c(%_cpu* %cpu) nounwind
ret void
}
define ccc void @stage72_entry(%_cpu* %cpu) nounwind {
call fastcc void @ENTRY_CONFIGURED(%_cpu* %cpu) nounwind
ret void
}
'''
    entry=entry.replace('ENTRY_CONFIGURED',cfg['entry_symbol'])
    typ='{i32,i8*}';table=',\n'.join(f'{typ} {{i32 {pc},i8* bitcast(void(%_cpu*)* @{name} to i8*)}}'for pc,name in bindings)
    entry+=f'@stage72_return_bindings = global [{len(bindings)} x {typ}] [\n{table}\n],align 8\n@stage72_return_bindings_count = global i32 {len(bindings)}\n'
    (R/'protected.ll').write_text('\n'.join(protected)+entry)
    runtime=(ROOT/'samples/stage72_runtime.c').read_text().replace('0x700ffd8c',cfg['stop_SP']or '0').replace('0x4ac333',cfg['stop_PC']);(R/'runtime.c').write_text(runtime)
    helper=(ROOT/'vendor-src/SR/SR/llasm-support/llasm_stos.c').read_text();before=helper
    helper=helper.replace('#include "llasm_cpu.h"','#include "stage72_runtime.h"')
    helper=helper.replace('x86_rep_stosd(CPU)','stage73_inner_rep_stosd(CPU)')
    aligned='*((uint32_t *)REG2PTR(dstptr)) = srcvalue;';unaligned='UNALIGNED_WRITE_32(dstptr, srcvalue)'
    assert helper.count(aligned)==2 and helper.count(unaligned)==2
    helper=helper.replace(aligned,aligned+'\n                stage72_note_write(REG2PTR(dstptr),4,srcvalue);').replace(unaligned,unaligned+'\n                stage72_note_write(REG2PTR(dstptr),4,srcvalue);')
    helper+='\nEXTERNC void CCALL x86_rep_stosd(CPU){stage72_validate_rep(cpu,32);stage73_inner_rep_stosd(cpu);}\n'
    (R/'rep-traced.c').write_text(helper)
    steps.append(command('stage73-clang-LLVM',['/usr/bin/clang','-Wno-override-module','-O2','-c','protected.ll','-o','bound.o']))
    for src,obj in [('runtime.c','runtime.o'),('rep-traced.c','rep.o')]:
        steps.append(command('stage73-compile-'+obj,['/usr/bin/clang','-std=c11','-O2','-DPTROFS_64BIT','-I',ROOT/'samples','-I',ROOT/'vendor-src/SR/SR/llasm-support','-c',src,'-o',obj]))
    if not cfg['runtime_eligible']:
        nm=subprocess.run(['/usr/bin/nm','-u',str(R/'bound.o')],capture_output=True,text=True,timeout=10)
        report=dict(status='SR_ARM64_OBJECT_BUILT_OPEN_CLOSURE_NOT_RUNNABLE',preparation=preparation,steps=steps,unresolved_symbols=nm.stdout.splitlines(),artifact=str(R/'bound.o'),sha256=sha(R/'bound.o'),edits=edits,guest_load_guards=loads,guest_store_guards_and_logs=stores,return_bindings=[hex(p)for p,n in bindings],runtime_eligible=False,generated_game_loaded_or_executed=False)
        (R/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in ['status','artifact','guest_load_guards','guest_store_guards_and_logs']}));return
    steps.append(command('stage73-link-UNEXECUTED',['/usr/bin/clang','-dynamiclib','bound.o','runtime.o','rep.o','-o','libstage73_SR_diff_UNEXECUTED.dylib']))
    u=subprocess.run(['/usr/bin/nm','-u',str(R/'libstage73_SR_diff_UNEXECUTED.dylib')],capture_output=True,text=True,timeout=10);assert u.returncode==0
    forbidden=[s for s in u.stdout.splitlines()if not any(x in s for x in ['_clock_gettime','_free','_malloc','_memcpy','_memset','_setjmp','_longjmp'])];assert not forbidden,forbidden
    result=dict(status='SR_GUEST_VA_6_RETURN_AND_GUARD_BRIDGE_BUILT_UNEXECUTED',original_bytes=preparation['original_bytes'],instructions=len(code),preparation=preparation,
        raw_guest_address_metadata=dict(prior_base_relocation_directory=prior_reloc,new_directory=prior_reloc,relocations_retained_for_decoding_then_generated_data_label_operands_bound_to_original_VA=True,source_instruction_bytes_unchanged=True),
        return_bindings=[dict(guest_PC=hex(p),native_symbol=n)for p,n in bindings],translation_binding_edits=edits,
        instruction_step_markers=len(code),guest_memory_load_guards=loads,guest_memory_store_guards_and_logs=stores,
        REP_helper=dict(vendor_source_sha256=hashlib.sha256(before.encode()).hexdigest(),traced_copy_sha256=sha(R/'rep-traced.c'),actual_store_log_insertions=4,CPU_commit_timing_preserved=True,invalid_whole_span_declined=True,no_generic_fault_equivalence=True),
        steps=steps,artifact=str(R/'libstage73_SR_diff_UNEXECUTED.dylib'),sha256=sha(R/'libstage73_SR_diff_UNEXECUTED.dylib'),library_loaded_or_executed=False,
        original_or_Unicorn_executed=False,actual_new_semantics_unvalidated_until_differential=True)
    (E/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k]for k in ['status','artifact','sha256','guest_memory_load_guards','guest_memory_store_guards_and_logs']}))
if __name__=='__main__':main()
