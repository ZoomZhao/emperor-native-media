"""Own 19-instruction mixed loop, four byte stores/pixel; never original bytes."""
from core import *
import argparse,subprocess,os,signal
def worker(out,pixels):
 u=fixed();from unicorn import x86_const as x
 uc=u.Uc(4,4);code=0x6f000000;src=0x40000000;dst=0x41000000;n=((pixels*4+4095)//4096)*4096;uc.mem_map(code,4096,7);uc.mem_map(src,n,1);uc.mem_map(dst,n,3)
 ins=[bytes.fromhex(q)for q in ('8b06','01d0','31d8','c1c005','c60711','c6470122','c6470233','c6470344','83c604','83c704','01c3','31da','f7d0','40','83f800','90','90','49')];body=b''.join(ins);body+=b'\x75'+struct.pack('<b',-len(body)-2);stores={code+sum(map(len,ins[:i]))for i in(4,5,6,7)};uc.mem_write(code,body);uc.mem_protect(code,4096,5);uc.reg_write(x.UC_X86_REG_ESI,src);uc.reg_write(x.UC_X86_REG_EDI,dst);uc.reg_write(x.UC_X86_REG_ECX,pixels)
 state={'steps':0,'writes':0,'planned':0,'blocks':0,'failure':None};t=time.monotonic();chunks=[];size=0;packer=struct.Struct('<6I');last_pc=0
 trace=(out/'writes.bin').open('xb')
 def stop(message):state['failure']=message;uc.emu_stop()
 def block(e,pc,z,data):
  if time.monotonic()-t>=45:stop('OWN_GUEST45_CAP');return
  if not(code<=pc and pc+z<=code+len(body)):stop('CODE_BLOCK_RANGE');return
  if bytes(e.mem_read(pc,z))!=body[pc-code:pc-code+z]:stop('IMMUTABLE_CODE_BLOCK_CHANGED');return
  state['blocks']+=1
 def instruction(e,pc,z,data):
  nonlocal last_pc
  if state['steps']>=20000000:stop('OWN_STEP20M_CAP');return
  if pc in stores:
   if state['writes']>=7000000:stop('OWN_WRITE7M_CAP_BEFORE_INSTRUCTION');return
   state['planned']+=1
  state['steps']+=1;last_pc=pc
 def write(e,access,a,z,v,data):
  nonlocal size
  if not dst<=a<dst+pixels*4 or z!=1 or state['planned']<1:stop('STORE_PLAN_ADDRESS_WIDTH');return
  state['planned']-=1;state['writes']+=1;row=packer.pack(0,last_pc,a,z,v&255,0);chunks.append(row);size+=len(row)
  if size>=262144:trace.write(b''.join(chunks));chunks.clear();size=0
 uc.hook_add(u.UC_HOOK_BLOCK,block);uc.hook_add(u.UC_HOOK_CODE,instruction);uc.hook_add(u.UC_HOOK_MEM_WRITE,write)
 uc.emu_start(code,code+len(body),timeout=45000000,count=20000000);guest=time.monotonic()-t
 if chunks:trace.write(b''.join(chunks))
 trace.close();s=time.monotonic();output=bytes(uc.mem_read(dst,pixels*4));(out/'output.bin').write_bytes(output);good=not state['failure']and state['steps']==19*pixels and state['writes']==4*pixels and output==bytes.fromhex('11223344')*pixels
 put(out/'result.json',{'status':'OWN_FULL_MIXED_LOOP_PASS'if good else'OWN_PERFORMANCE_BOUNDARY','pixels':pixels,'own_instructions':state['steps'],'own_write_records':state['writes'],'guest_seconds':guest,'instructions_per_second':state['steps']/guest,'own_output_save_seconds':time.monotonic()-s,'state':state,'own_body_hex':body.hex(),'code_original_or_SR_derived':False,'validation':'immutable block bytes plus each instruction count and pre-instruction single-store plans; packed ordered trace','steps_cap':20000000,'writes_cap':7000000,'guest_seconds_cap':45,'trace_bytes':(out/'writes.bin').stat().st_size,'original_instructions':0,'models_original_full_prefix_performance_exactly':False});print(json.dumps({'good':good,'steps':state['steps'],'writes':state['writes'],'guest_seconds':guest,'failure':state['failure']}));return 0 if good else 2
def main():
 p=argparse.ArgumentParser();p.add_argument('--worker',action='store_true');p.add_argument('--pixels',type=int,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if a.pixels not in (32768,786432)or not out.is_relative_to(V/'runs'):raise Boundary('OWN_PERFORMANCE_SCOPE')
 if a.worker:raise SystemExit(worker(out,a.pixels))
 out.mkdir(parents=True,exist_ok=False);put(out/'own.claim.json',{'kind':'AUTHORED_ONLY_PERFORMANCE','original_rounds_consumed':0,'hard_process_seconds':60,'pixels':a.pixels});t=time.monotonic()
 command=['/usr/bin/python3','-B',str(V/'tools/performance.py'),'--worker','--pixels',str(a.pixels),'--output',str(out)]
 with(out/'stdout.log').open('xb')as o,(out/'stderr.log').open('xb')as e:
  q=subprocess.Popen(command,stdout=o,stderr=e,start_new_session=True)
  try:rc=q.wait(timeout=60);timed=False
  except subprocess.TimeoutExpired:timed=True;os.killpg(q.pid,signal.SIGKILL);rc=q.wait()
 put(out/'process.json',{'command':command,'returncode':rc,'timed_out':timed,'total_seconds':time.monotonic()-t,'hard_cap_seconds':60,'original_execution':0});print(json.dumps({'returncode':rc,'process_seconds':time.monotonic()-t,'timeout':timed}))
if __name__=='__main__':main()
