"""Generate authored x86 and equivalent own LLVM; no original bytes reused."""
import json,struct,sys
sys.dont_write_bytecode=True
from common import ROOT,S,API,put
def main():
 c=json.loads((S/'configs/authored.json').read_text());pc=c['entry'];instructions=[];functions=[];lines=[];bindings=[];counter=0;imports={}
 def ins(body,mnemonic):
  nonlocal pc
  old=pc;instructions.append({'pc':pc,'end':pc+len(body),'bytes':body.hex(),'mnemonic':mnemonic,'operand':''});pc+=len(body);lines.append('call void @stage72_step(i32 '+str(old)+')');return old
 def var():
  nonlocal counter
  counter+=1;return '%v'+str(counter)
 def loadcpu(i):
  p=var();v=var();lines.extend([p+' = getelementptr %_cpu,%_cpu* %cpu,i32 0,i32 0,i32 '+str(i),v+' = load i32,i32* '+p]);return p,v
 def ptr(a):
  q=var();o=var();aa=var();p=var();b=var();lines.extend([q+' = getelementptr %_cpu,%_cpu* %cpu,i32 0,i32 5',o+' = load i64,i64* '+q,aa+' = add i64 '+o+','+str(a),p+' = inttoptr i64 '+aa+' to i32*',b+' = bitcast i32* '+p+' to i8*']);return p,b
 def guestload(a):
  p,b=ptr(a);v=var();lines.extend(['call void @stage72_guard(i8* '+b+',i32 4,i32 1)',v+' = load i32,i32* '+p]);return v
 def gueststore(p,b,v):
  w=var();lines.extend(['call void @stage72_guard(i8* '+b+',i32 4,i32 2)','store i32 '+v+',i32* '+p,w+' = zext i32 '+v+' to i64','call void @stage72_note_write(i8* '+b+',i32 4,i64 '+w+')'])
 def push(value):
  sp,s=loadcpu(4);next=var();wide=var();offp=var();off=var();address=var();p=var();b=var();lines.extend([next+' = sub i32 '+s+',4',wide+' = zext i32 '+next+' to i64',offp+' = getelementptr %_cpu,%_cpu* %cpu,i32 0,i32 5',off+' = load i64,i64* '+offp,address+' = add i64 '+off+','+wide,p+' = inttoptr i64 '+address+' to i32*',b+' = bitcast i32* '+p+' to i8*']);gueststore(p,b,str(value));lines.append('store i32 '+next+',i32* '+sp)
 decl='''declare void @stage72_step(i32)
declare void @stage72_guard(i8*,i32,i32)
declare void @stage72_note_write(i8*,i32,i64)
declare i8* @stage72_lookup_return(i32)
'''
 current='own_phase';lines=['define fastcc void @'+current+'(%_cpu* %cpu){']
 ops=[(10,[0xa1000200],None),(11,[0xa1000200],None),(11,[0xa1000200],None),(12,[0xa1000200],None),(12,[0xa1000200],None),(1,[0x3001,0,13],0xa1000100),(15,[{'load':0xa1000100},0x3d,13],None),(4,[0x3001,0,{'load':0xa1000100}],0xa1000104),(2,[0x3001,8,{'load':0xa1000100},25],0xa1000100),(13,[0xa1000300,{'load':0xa1000100},13],None),(14,[0xa1000301,0xa1000300,12],None),(3,[0x3001,0,{'load':0xa1000100}],None),(7,[777],None),(6,[],None),(9,[0,0xf00],None),(8,[0],None),(5,[],None),(1,[0x3001,8,0],0xa1000100),(3,[0x3001,0,{'load':0xa1000100}],None),(1,[0x3001,0,0x10000001],None)]
 for id,args,result in ops:
  for a in reversed(args):
   if isinstance(a,dict):ins(b'\xff\x35'+struct.pack('<I',a['load']),'push');v=guestload(a['load'])
   else:ins(b'\x68'+struct.pack('<I',a),'push');v=str(a)
   push(v)
  name=API[id][0];target=0xa0000800+16*id;imports[target]=['authored.fixture',name];resume=pc+5;ins(b'\xe8'+struct.pack('<i',target-resume),'call');push(resume);symbol='Guest'+name if id>=13 else name;lines+=['musttail call fastcc void @'+symbol+'(%_cpu* %cpu)','ret void','}'];functions.append('\n'.join(lines));current='resume_'+str(resume);bindings.append((resume,current));lines=['define fastcc void @'+current+'(%_cpu* %cpu){']
  if id>=13:
   ins(b'\x8d\x64\x24\x0c','lea');p,v=loadcpu(4);n=var();lines.extend([n+' = add i32 '+v+',12','store i32 '+n+',i32* '+p])
  if result:
   ins(b'\xa3'+struct.pack('<I',result),'mov');p,v=loadcpu(0);gp,gb=ptr(result);gueststore(gp,gb,v)
 ins(b'\xc3','ret');sp,v=loadcpu(4);wide=var();op=var();off=var();a=var();st=var();stb=var();r=var();pop=var();t=var();fn=var();lines.extend([wide+' = zext i32 '+v+' to i64',op+' = getelementptr %_cpu,%_cpu* %cpu,i32 0,i32 5',off+' = load i64,i64* '+op,a+' = add i64 '+off+','+wide,st+' = inttoptr i64 '+a+' to i32*',stb+' = bitcast i32* '+st+' to i8*','call void @stage72_guard(i8* '+stb+',i32 4,i32 1)',r+' = load i32,i32* '+st,pop+' = add i32 '+v+',4','store i32 '+pop+',i32* '+sp,t+' = call i8* @stage72_lookup_return(i32 '+r+')',fn+' = bitcast i8* '+t+' to void(%_cpu*)*','musttail call fastcc void '+fn+'(%_cpu* %cpu)','ret void','}']);functions.append('\n'.join(lines))
 assert pc<0xa0000800
 decl+='@stage72_return_bindings_count = global i32 '+str(len(bindings))+'\n@stage72_return_bindings = global ['+str(len(bindings))+' x {i32,i8*}] ['+','.join('{i32,i8*} {i32 '+str(p)+',i8* bitcast(void(%_cpu*)* @'+n+' to i8*)}'for p,n in bindings)+']\n@stage77_structural_code_bindings = global [0 x {i32,i8*}] zeroinitializer\n'
 (S/'tools/authored-platform.ll').write_text(decl+'\n'+'\n'.join(functions)+'\n');c.update(id='S82_authored_platform_ABC_v1',source='stage82/runs/platform-fixture-input/snapshot.json',authored_IR='stage82/tools/authored-platform.ll',instructions=instructions,fixture_imports=imports,native_driver='stage82/builds/authored-platform-v1/native-driver');c['limits']={k:{'steps':1000,'writes':1000,'guest_seconds':1}for k in ('capture','reference','native')};put(S/'configs/authored-platform.json',c);put(S/'evidence/authored-platform-cases.json',{'API_calls':len(ops),'APIs':len(set(id for id,a,r in ops)),'operations':ops,'instructions':len(instructions),'original_instruction_bytes':0});print(json.dumps({'self_authored_instructions':len(instructions),'API_calls':len(ops),'all_15_APIs':True}))
if __name__=='__main__':main()
