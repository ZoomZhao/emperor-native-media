import hashlib,json,re,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage79'
def main():
 out=S/'builds/core-state-open';out.mkdir(exist_ok=False);steps=[]
 def run(label,args):
  with(out/(label+'.stdout.log')).open('xb')as a,(out/(label+'.stderr.log')).open('xb')as b:q=subprocess.run([str(x)for x in args],stdout=a,stderr=b,cwd=out,timeout=30)
  steps.append(dict(label=label,returncode=q.returncode,command=[str(x)for x in args]));assert not q.returncode,label
 for src in ['guest_memory','thread_native','thread_frontend']:run('clang-'+src,['/usr/bin/clang','-std=c11','-O2','-Wall','-Wextra','-Werror','-c',S/'tools'/(src+'.c'),'-o',out/(src+'.o')])
 text='include llasm.llinc\ninclude asm-calls.llinc\nfuncp stage72_lookup_return pc\nmacro Call_Stage79_Return _numpop\nload tmp0, esp, 4\nadd esp, esp, 4 + (\\_numpop * 4)\ncall stage72_lookup_return tmp0\ntcall tmp0\nendm\n'
 for name,args in [('GetCurrentThreadId',[]),('GetLastError',[]),('SetLastError',['value']),('TlsGetValue',['slot']),('TlsSetValue',['slot','value'])]:
  text+='func stage79_%s_c %s\n'%(name,', '.join(args));text+='proc %s public\nCall_Asm_Stack%d stage79_%s_c\nCall_Stage79_Return %d\nendp\n'%(name,len(args),name,len(args))
 (out/'thread-wrappers.llasm').write_text(text);run('llasm-thread-wrappers',[ROOT/'build/bin/llasm',out/'thread-wrappers.llasm','-I',ROOT/'vendor-src/SR/SR/llasm-support','-I',ROOT/'vendor-src/SR/games/Septerra Core/SR-Septerra/llasm','-m64','-pic','-ptrofs','-O','-o',out/'thread-wrappers.ll']);run('clang-thread-wrappers',['/usr/bin/clang','-O2','-Wno-override-module','-c',out/'thread-wrappers.ll','-o',out/'thread-wrappers.o'])
 objects=[ROOT/'stage78/builds/full-bound-guarded-open/guarded-full.o']+[ROOT/'stage77/builds/heap-integrated-open'/n for n in ['heap-native.o','heap-frontend.o','heap-wrappers.o']]+[ROOT/'stage78/builds/full-helpers-open'/n for n in ['guarded-helpers.o','helper-frontend.o']]+list(out.glob('*.o'))
 run('archive',['/usr/bin/ar','rcs',out/'phase8-core-state-OPEN.a']+objects);run('symbols',['/usr/bin/nm','-g',out/'phase8-core-state-OPEN.a']);defined=set();undefined=set()
 for line in(out/'symbols.stdout.log').read_text().splitlines():
  m=re.fullmatch(r'\s*(?:[0-9a-fA-F]+\s+)?([A-Za-z])\s+(\S+)',line)
  if m:(undefined if m[1]=='U'else defined).add(m[2])
 names=['GetCurrentThreadId','GetLastError','SetLastError','TlsGetValue','TlsSetValue'];assert all('_'+n in defined for n in names)
 result=dict(status='CORE_STATE_MEMORY_AND_FIVE_THREAD_API_ABIS_COMPILED_OPEN',steps=steps,thread_APIs_defined=names,remaining_external_symbols=sorted(undefined-defined),native_CPU_engine_sidecar_restorer_connected=False,native_code_pointer_offset_compatible_guest_arena_proved=False,guest_memory_mapper_and_scalar_SR_guard_hooks_connected=False,SEH_implemented=False,missing_known_virtual_targets=['0x546be0','0x546c40'],game_library_loaded_or_executed=False,complete_phase8_runnable=False,archive_sha256=hashlib.sha256((out/'phase8-core-state-OPEN.a').read_bytes()).hexdigest());(out/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k]for k in ['status','thread_APIs_defined','missing_known_virtual_targets','complete_phase8_runnable']}))
if __name__=='__main__':main()
