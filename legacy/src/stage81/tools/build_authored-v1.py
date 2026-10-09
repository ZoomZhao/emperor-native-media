import json,re,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage81';sys.path.insert(0,str(S/'tools'))
from separate_code_targets import transform
def main():
 out=S/'builds/authored-joint-v1';out.mkdir(exist_ok=False);steps=[]
 def run(label,args):
  with(out/(label+'.stdout.log')).open('xb')as a,(out/(label+'.stderr.log')).open('xb')as b:q=subprocess.run([str(x)for x in args],cwd=ROOT,stdout=a,stderr=b,timeout=30)
  steps.append(dict(command=[str(x)for x in args],returncode=q.returncode));assert not q.returncode,label
 run('llasm',[ROOT/'build/bin/llasm',S/'tools/authored.llasm','-I',ROOT/'vendor-src/SR/SR/llasm-support','-m64','-pic','-ptrofs','-O','-o',out/'raw.ll'])
 text,report=transform((out/'raw.ll').read_text(),expected_lookups=3)
 ptr={};lines=[];index=0
 for line in text.splitlines():
  if line.startswith('define '):ptr={}
  m=re.match(r'(%\w+) = inttoptr i64 %\w+ to i(8|16|32)\*$',line)
  if m:ptr[m[1]]=int(m[2])//8
  m=re.match(r'(?:%\w+ = )?(load|store) i(8|16|32)(?:, | )(.+)',line)
  if m:
   kind,bits,tail=m.groups();pointer=re.search(r'i\d+\* (%\w+)',tail)
   if pointer and pointer[1]in ptr:
    p=pointer[1];index+=1;cast='%guard_'+str(index);n=ptr[p];lines+=[cast+' = bitcast i'+bits+'* '+p+' to i8*','call void @stage72_guard(i8* '+cast+',i32 '+str(n)+',i32 '+str(2 if kind=='store'else 1)+')',line]
    if kind=='store':
     v=tail.split(',',1)[0];wide='%value_'+str(index);lines+=([wide+' = zext i'+bits+' '+v+' to i64']if v.startswith('%')else[]);lines+=['call void @stage72_note_write(i8* '+cast+',i32 '+str(n)+',i64 '+(wide if v.startswith('%')else str(int(v)&((1<<int(bits))-1)))+')']
    continue
  lines.append(line)
 text='\n'.join(lines)+'''\n
declare void @stage72_guard(i8*,i32,i32)
declare void @stage72_note_write(i8*,i32,i64)
define ccc void @stage81_authored_run(%_cpu* %cpu) {
call fastcc void @own_entry(%_cpu* %cpu)
ret void
}
@stage81_authored_bindings = global [3 x {i32,i8*}] [{i32,i8*} {i32 2684354821,i8* bitcast(void(%_cpu*)* @own_resume to i8*)},{i32,i8*} {i32 2684355072,i8* bitcast(void(%_cpu*)* @own_callback to i8*)},{i32,i8*} {i32 2684354564,i8* bitcast(void(%_cpu*)* @own_done to i8*)}],align 8
'''
 (out/'guarded-separated.ll').write_text(text);(out/'rewrite.json').write_text(json.dumps(report,indent=2)+'\n');run('clang-authored',['/usr/bin/clang','-Wno-override-module','-O2','-c',out/'guarded-separated.ll','-o',out/'authored.o'])
 run('clang-joint',['/usr/bin/clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-logical-op-parentheses','-fsanitize=undefined','-I'+str(S/'tools'),'-I'+str(ROOT/'stage80/tools'),'-I'+str(ROOT/'stage79/tools'),'-I'+str(ROOT/'stage78/tools'),'-I'+str(ROOT/'stage77/tools'),S/'tools/joint_selftest.c',S/'tools/runtime.c',S/'tools/domains.c',S/'tools/heap_connection.c',ROOT/'stage79/tools/guest_memory.c',ROOT/'stage79/tools/thread_native.c',ROOT/'stage79/tools/thread_frontend.c',ROOT/'stage78/tools/guarded_helpers.c',ROOT/'stage78/tools/helper_frontend.c',ROOT/'stage77/tools/heap_native.c',ROOT/'stage77/tools/heap_frontend.c',out/'authored.o','-o',out/'joint-selftest'])
 (out/'result.json').write_text(json.dumps(dict(status='AUTHORED_JOINT_ONLY_BUILT',steps=steps,original_game_object_linked=False,SR_original_game_instructions=0),indent=2)+'\n')
if __name__=='__main__':main()
