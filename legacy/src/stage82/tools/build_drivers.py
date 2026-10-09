import argparse,json,re,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
from common import ROOT,S,API,put,sha
TYPE='%_cpu = type {[9 x i32],i32,i32,i32,[8 x double],i64,i64,i8*,i8*}\n'
def build(out,authored,config=None):
 out.mkdir(exist_ok=False);steps=[]
 def run(label,args):
  with(out/(label+'.stdout.log')).open('xb')as o,(out/(label+'.stderr.log')).open('xb')as e:q=subprocess.run(list(map(str,args)),cwd=ROOT,stdout=o,stderr=e,timeout=30)
  steps.append({'command':list(map(str,args)),'returncode':q.returncode});assert not q.returncode,label
 scope=json.loads((config or (S/'configs/authored.json'if authored else S/'configs/original.json')).read_text());assert (scope['kind']=='authored')==authored;pcs=sorted({r['pc']for r in scope['instructions']});entry=scope['entry'];ret=scope['return']
 header='#define S82_MODE '+str(2 if authored else 1)+'\n#define S82_ENTRY '+str(entry)+'u\n#define S82_RETURN '+str(ret)+'u\n#define S82_TARGET_COUNT '+str(0 if authored else 2020)+'u\nstatic const uint32_t s82_allowed[]={'+','.join(str(p)+'u'for p in pcs)+'};\n';(out/'build_scope.h').write_text(header)
 text=TYPE if authored else (ROOT/'stage81/builds/full-separated-v5/game.ll').read_text()
 if not authored:
  names={n for n,k in API.values()}|{n.lstrip('_')for n in json.loads((ROOT/'stage81/evidence/result.json').read_text())['unimplemented_Win32_API_symbols']}
  text='\n'.join(line for line in text.splitlines()if not(line.startswith('declare ')and any('@'+n+'('in line for n in names)))+'\n'
 text+='\ndeclare void @s82_complete() noreturn\ndeclare i8* @s82_api_dispatch(i8*,i32)\ndeclare void @s82_unimplemented(i32) noreturn\ndefine fastcc void @s82_completion(%_cpu* %cpu){\ncall void @s82_complete()\nunreachable\n}\n'
 for id,(name,n)in API.items():
  symbol='Guest'+name if id>=13 else name;text+='define fastcc void @'+symbol+'(%_cpu* %cpu){\n%p = bitcast %_cpu* %cpu to i8*\n%t = call i8* @s82_api_dispatch(i8* %p,i32 '+str(id)+')\n%f = bitcast i8* %t to void(%_cpu*)*\nmusttail call fastcc void %f(%_cpu* %cpu)\nret void\n}\n'
 if authored:
  text+='''declare void @stage72_step(i32)
declare void @stage72_guard(i8*,i32,i32)
declare void @stage72_note_write(i8*,i32,i64)
declare i8* @stage72_lookup_return(i32)
@stage72_return_bindings_count = global i32 0
@stage72_return_bindings = global [0 x {i32,i8*}] zeroinitializer
@stage77_structural_code_bindings = global [0 x {i32,i8*}] zeroinitializer
define fastcc void @own_phase(%_cpu* %cpu){
call void @stage72_step(i32 2684354816)
%op = getelementptr %_cpu,%_cpu* %cpu,i32 0,i32 5
%o = load i64,i64* %op
%va = add i64 %o,2701131776
%d = inttoptr i64 %va to i32*
%db = bitcast i32* %d to i8*
call void @stage72_guard(i8* %db,i32 4,i32 1)
%v = load i32,i32* %d
%eax = getelementptr %_cpu,%_cpu* %cpu,i32 0,i32 0,i32 0
store i32 %v,i32* %eax
call void @stage72_step(i32 2684354821)
%sum = add i32 %v,7
store i32 %sum,i32* %eax
call void @stage72_step(i32 2684354824)
call void @stage72_guard(i8* %db,i32 4,i32 2)
store i32 %sum,i32* %d
%wide = zext i32 %sum to i64
call void @stage72_note_write(i8* %db,i32 4,i64 %wide)
call void @stage72_step(i32 2684354829)
%sp = getelementptr %_cpu,%_cpu* %cpu,i32 0,i32 0,i32 4
%s = load i32,i32* %sp
%s64 = zext i32 %s to i64
%sa = add i64 %o,%s64
%st = inttoptr i64 %sa to i32*
%stb = bitcast i32* %st to i8*
call void @stage72_guard(i8* %stb,i32 4,i32 1)
%r = load i32,i32* %st
%pop = add i32 %s,4
store i32 %pop,i32* %sp
%target = call i8* @stage72_lookup_return(i32 %r)
%f = bitcast i8* %target to void(%_cpu*)*
musttail call fastcc void %f(%_cpu* %cpu)
ret void
}
'''
 else:
  remaining=[n.lstrip('_')for n in json.loads((ROOT/'stage81/evidence/result.json').read_text())['unimplemented_Win32_API_symbols']if n.lstrip('_')not in {n for n,k in API.values()}]
  for id,n in enumerate(remaining,100):text+='define fastcc void @'+n+'(%_cpu* %cpu){\ncall void @s82_unimplemented(i32 '+str(id)+')\nunreachable\n}\n'
 if authored and scope.get('authored_IR'):
  begin=text.index('declare void @stage72_step');text=text[:begin]+(ROOT/scope['authored_IR']).read_text()
 entry_symbol='own_phase'
 if not authored:
  matches=set(re.findall(r'i32 '+str(entry)+r',i8\* bitcast\(void\(%_cpu\*\)\* @(\w+) to i8\*\)',text));assert len(matches)==1,matches;entry_symbol=matches.pop()
 text+='define ccc void @s82_entry(%_cpu* %cpu){\ncall fastcc void @'+entry_symbol+'(%_cpu* %cpu)\nret void\n}\n';(out/'driver.ll').write_text(text);run('clang-ir',['/usr/bin/clang','-Wno-override-module','-O2','-c',out/'driver.ll','-o',out/'driver.o'])
 inc=['-I'+str(p)for p in [out,S/'tools',ROOT/'stage80/tools',ROOT/'stage79/tools',ROOT/'stage78/tools',ROOT/'stage77/tools']]
 sources=[S/'tools'/p for p in ('native_driver.c','runtime.c','heap_connection.c','platform_native.c','domains.c')]+[ROOT/'stage79/tools/guest_memory.c',ROOT/'stage79/tools/thread_native.c',ROOT/'stage78/tools/guarded_helpers.c',ROOT/'stage78/tools/helper_frontend.c',ROOT/'stage77/tools/heap_native.c',ROOT/'stage81/tools/idiv.c']
 run('clang-driver',['/usr/bin/clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-logical-op-parentheses','-fsanitize=undefined',*inc,*sources,out/'driver.o','-o',out/'native-driver'])
 run('undefined',['/usr/bin/nm','-u',out/'native-driver']);put(out/'result.json',{'status':'DRIVER_BUILT_NOT_EXECUTED','kind':scope['kind'],'native_driver_sha256':sha(out/'native-driver'),'original_game_functions_invoked':0,'same_existing_function_set':True,'steps':steps});print(json.dumps({'kind':scope['kind'],'driver':str(out/'native-driver'),'built':True,'game_invocations':0}))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--authored',action='store_true');p.add_argument('--config',type=Path);a=p.parse_args();assert a.out.resolve().is_relative_to(S/'builds');build(a.out.resolve(),a.authored,a.config)
