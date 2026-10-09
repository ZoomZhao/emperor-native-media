"""One future-approved CPU evidence process; zero original game instructions.

--describe and module import never load Unicorn. --run needs a new user approval
bound to this exact manifest. Probe engines contain only authored instructions
and synthetic FS data, never original instruction/memory pages. No SR library
is loaded. Exclusive claim persists on every failure; no automatic retry.
"""
import argparse,hashlib,json,signal,sys,time
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage79';MANIFEST=S/'configs/cpu-evidence-manifest.json'
def sha(path):
 h=hashlib.sha256()
 with Path(path).open('rb')as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
def exclusive_json(path,value):
 with path.open('x')as f:json.dump(value,f,indent=2);f.write('\n')
def evidence(manifest):
 sys.path.insert(0,str(ROOT/'stage74/tools'))
 from cpu_adapter import load_fixed,restore,public_state
 u=load_fixed(ROOT,manifest);from unicorn import x86_const as x
 outputs=[];probes=[]
 for record in manifest['contexts']:
  raw=Path(record['path']).read_bytes();meta=record['metadata'];engine,ctx=restore(u,raw,meta);state=public_state(engine,u);del engine,ctx
  outputs.append(dict(tid=record['tid'],public_CPU=state,opaque_sha256=record['sha256'],source=record['path']))
  # Empty-engine probe clones: twelve DS/ES/SS endpoint accesses plus FS:[0]
  # read/write. They cover the selected memory-operation contract; no claim
  # about complete segmentation/SEH/interrupt/exception equivalence is made.
  for segment,prefix,targets in [('DS',0x3e,(0,0xfffffffc)),('ES',0x26,(0,0xfffffffc)),('SS',0x36,(0,0xfffffffc)),('FS',0x64,(0,))]:
   for target in targets:
    for opcode in (0xa1,0xa3):
     probe,ctx=restore(u,raw,meta);base=0x6f000000;body=bytes([prefix,opcode])+target.to_bytes(4,'little');probe.mem_map(base,4096,u.UC_PROT_ALL);probe.mem_write(base,body);probe.mem_protect(base,4096,u.UC_PROT_READ|u.UC_PROT_EXEC)
     if segment=='FS':
      fs=state['FS_BASE'];assert 0<=fs<=0xffffffff-4095 and fs%4096==0
      probe.mem_map(fs,4096,u.UC_PROT_READ|u.UC_PROT_WRITE);probe.mem_write(fs,(0x73537353).to_bytes(4,'little'))
     probe.reg_write(x.UC_X86_REG_EIP,base);probe.reg_write(x.UC_X86_REG_EAX,0x7a790001);events=[];code=[]
     def invalid(uc,access,address,size,value,data):events.append((access,address,size));return False
     def access(uc,kind,address,size,value,data):events.append((kind,address,size))
     def code_hook(uc,address,size,data):
      code.append((address,size))
      if address!=base or size!=6:uc.emu_stop()
     probe.hook_add(u.UC_HOOK_CODE,code_hook)
     if segment=='FS':probe.hook_add(u.UC_HOOK_MEM_READ|u.UC_HOOK_MEM_WRITE,access)
     else:probe.hook_add(u.UC_HOOK_MEM_INVALID,invalid)
     error=None
     try:probe.emu_start(base,base+len(body),timeout=100000,count=1)
     except u.UcError as e:error=str(e)
     assert code==[(base,6)],('unexpected authored probe fetch',record['tid'],segment,code,error)
     if segment=='FS':
      expected=u.UC_MEM_READ if opcode==0xa1 else u.UC_MEM_WRITE;assert error is None and events==[(expected,fs,4)],(segment,events,error)
      if opcode==0xa1:assert probe.reg_read(x.UC_X86_REG_EAX)==0x73537353
      else:assert bytes(probe.mem_read(fs,4))==(0x7a790001).to_bytes(4,'little')
     else:
      expected=u.UC_MEM_READ_UNMAPPED if opcode==0xa1 else u.UC_MEM_WRITE_UNMAPPED;assert error is not None and events==[(expected,target,4)],(segment,events,error)
     probes.append(dict(tid=record['tid'],segment=segment,target=target,access='read' if opcode==0xa1 else 'write',bytes=body.hex(),authored_instructions_maximum=1,events=events,expected_boundary_observed=True));del probe,ctx
 assert len(probes)==56
 return dict(status='PUBLIC_CPU_AND_SCOPED_CACHE_PROBES_COLLECTED',contexts=outputs,probes=probes,authored_instruction_maximum=56,original_game_instructions=0,game_or_SR_library_loaded=False,scope='DS/ES/SS 32bit dword endpoint linear accesses and FS:[0] read/write, each exact saved context; general segmentation/SEH equivalence unproved',snapshot_threads_executed_or_scheduled=False)
def main():
 p=argparse.ArgumentParser();mode=p.add_mutually_exclusive_group(required=True);mode.add_argument('--describe',action='store_true');mode.add_argument('--run',action='store_true');p.add_argument('--approval');args=p.parse_args();manifest=json.loads(MANIFEST.read_text());digest=sha(MANIFEST)
 if args.describe:print(json.dumps(dict(manifest_sha256=digest,execution_scope=manifest['execution_scope'],limits=manifest['limits'],contexts=manifest['contexts']),indent=2));return
 def timeout(a,b):raise RuntimeError('60-second hard process budget')
 signal.signal(signal.SIGALRM,timeout);signal.alarm(60)
 if not args.approval:raise RuntimeError('New explicit user approval required; no Unicorn loaded')
 approval_path=(ROOT/args.approval).resolve();assert approval_path.is_relative_to(S)
 approval=json.loads(approval_path.read_text())
 if approval.get('status')!='AUTHORIZED_BY_USER' or not approval.get('user_message_id')or approval.get('manifest_sha256')!=digest or approval.get('scope')!=manifest['execution_scope']or approval.get('maximum_processes')!=1:raise RuntimeError('New approval scope/digest/cap mismatch; no engine loaded')
 out=S/'runs/cpu-evidence-once';claim=S/'evidence/cpu-evidence-attempt.claim.json';exclusive_json(claim,dict(manifest_sha256=digest,approval_sha256=sha(approval_path),maximum_processes=1,charged_attempt=True,retry_allowed=False,old_ledger_reopened=False,start_UTC=time.time()));out.mkdir(parents=True,exist_ok=False)
 try:
  for row in manifest['pinned_files']:
   path=Path(row['path']);assert sha(path)==row['sha256'],str(path)
  result=evidence(manifest);result.update(manifest_sha256=digest,approval_sha256=sha(approval_path),old_ledger_modified=False)
  exclusive_json(out/'result.json',result);print(json.dumps({k:result[k]for k in ['status','authored_instruction_maximum','original_game_instructions','old_ledger_modified']}))
 except BaseException as e:
  exclusive_json(out/'failure.json',dict(status='CPU_EVIDENCE_ATTEMPT_FAILED',error=repr(e),retry_allowed=False,old_ledger_modified=False));raise
if __name__=='__main__':main()
