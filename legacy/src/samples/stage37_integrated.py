#!/usr/bin/env python3
"""One bounded own-i386 ABI -> arm64 PCM queue -> process restore experiment.

No Emperor image, original DLL, game audio or original Miles binding is loaded.
The callback contract here is an authored polling event queue, not Miles EOS.
"""
import argparse, hashlib, json, struct, subprocess, sys, threading, time
from pathlib import Path
from stage37_audio import NativeAudio, GuestABI, R
from stage7_runtime import u, x

CODE=0x41000000
THUNKS=0x42000000
STACK=0x43000000
DATA=0x44000000
DRIVER_OUT=DATA+0x100
HANDLE_OUT=DATA+0x200
EVENT_OUT=DATA+0x300
PCM=DATA+0x1000
SAVE=R/'runs/stage37-audio-snapshot.bin'
GUEST_SAVE=R/'runs/stage37-guest32-data.bin'
SPEC=R/'evidence/stage37-resume-spec.json'

def sha(b): return hashlib.sha256(b).hexdigest()
def fnv(b):
 h=14695981039346656037
 for v in b: h=((h^v)*1099511628211)&0xffffffffffffffff
 return h
def pcm(index,frames):
 # Authored +/-1 S16 values, no game assets or system volume changes.
 period=b''.join(struct.pack('<hh',1 if (j+index)%3 else -1,1 if (j+index)%5 else -1)for j in range(15))
 return (period*((frames+14)//15))[:frames*4]

class Guest32:
 def __init__(self,native,image=None):
  self.uc=u.Uc(u.UC_ARCH_X86,u.UC_MODE_32)
  self.uc.mem_map(CODE,4096,u.UC_PROT_READ|u.UC_PROT_EXEC)
  self.uc.mem_map(THUNKS,4096,u.UC_PROT_READ|u.UC_PROT_EXEC)
  self.uc.mem_map(STACK,65536,u.UC_PROT_READ|u.UC_PROT_WRITE)
  self.uc.mem_map(DATA,131072,u.UC_PROT_READ|u.UC_PROT_WRITE)
  if image:self.uc.mem_write(DATA,image)
  self.abi=GuestABI(native,self.uc,x)
  self.addresses={name:THUNKS+i*16 for i,name in enumerate(self.abi.argc)}
  self.reverse={v:k for k,v in self.addresses.items()}
  for address in self.reverse:self.uc.mem_write(address,b'\x0f\x0b')
  self.uc.hook_add(u.UC_HOOK_CODE,self.hook)
  self.end=None
 def hook(self,uc,address,size,userdata):
  if address in self.reverse:self.abi.api(self.reverse[address])
  elif address==self.end:uc.emu_stop()
 def call(self,name,*args):
  assert len(args)==self.abi.argc[name]
  # Actual own i386 instructions push DWORDs and indirectly call the ABI.
  code=b''.join(b'\x68'+struct.pack('<I',a)for a in reversed(args))
  code+=b'\xb8'+struct.pack('<I',self.addresses[name])+b'\xff\xd0'
  self.end=CODE+len(code)
  self.uc.mem_write(CODE,code+b'\xf4')
  self.uc.ctl_remove_cache(CODE,CODE+4096)
  initial_sp=STACK+65536-256
  self.uc.reg_write(x.UC_X86_REG_ESP,initial_sp)
  self.uc.emu_start(CODE,self.end+1,count=100)
  assert self.uc.reg_read(x.UC_X86_REG_EIP)==self.end
  assert self.uc.reg_read(x.UC_X86_REG_ESP)==initial_sp
  return self.uc.reg_read(x.UC_X86_REG_EAX)
 def word(self,p):return struct.unpack('<I',bytes(self.uc.mem_read(p,4)))[0]
 def poll(self,driver):
  status=self.call('poll',driver,EVENT_OUT)
  assert status in [0,1]
  return list(struct.unpack('<4I',bytes(self.uc.mem_read(EVENT_OUT,16))))if status else None
 def image(self):return bytes(self.uc.mem_read(DATA,131072))

def restore():
 spec=json.loads(SPEC.read_text());b=SAVE.read_bytes();guest_image=GUEST_SAVE.read_bytes()
 assert sha(b)==spec['snapshot_sha256'] and sha(guest_image)==spec['guest_image_sha256']
 native=NativeAudio()
 try:
  native.restore(b)
  guest=Guest32(native,guest_image)
  driver=guest.word(DRIVER_OUT)
  assert driver==native.driver
  assert [guest.word(HANDLE_OUT+i*4)for i in range(16)]==spec['all_handles']
  before=native.stats();assert before==spec['frozen_stats']
  assert guest.call('start',driver)==0
  deadline=time.monotonic()+4
  while native.stats()['queued_buffers'] and time.monotonic()<deadline:time.sleep(.005)
  assert native.stats()['queued_buffers']==0
  assert guest.call('stop',driver)==0
  after=native.stats();events=[]
  while True:
   event=guest.poll(driver)
   if event is None:break
   events.append(event)
  expected=spec['expected_events']
  assert events==expected,(events,expected)
  assert after['source_frames']==spec['expected_source_frames']
  assert after['source_hash']==spec['expected_source_fnv64']
  assert after['render_frames']==after['source_frames']+after['zero_frames']
  assert after['bad_buffers']==after['active_callbacks']==after['queued_buffers']==0
  assert after['callbacks']>before['callbacks']
  # One recycled slot verifies generation protection through real guest DWORD ABI.
  old=expected[0][0];assert guest.call('release',driver,old)==0
  assert guest.call('start',driver)==0
  body=pcm(17,128);guest.uc.mem_write(PCM,body)
  assert guest.call('prepare',driver,PCM,128,HANDLE_OUT)==0
  new=guest.word(HANDLE_OUT)
  assert new!=old and new&255==old&255 and (new>>8)&65535==((old>>8)&65535)+1
  old_status=guest.call('queue',driver,old)
  wrong_status=guest.call('queue',0x5d000101,new)
  assert old_status==wrong_status==1
  assert native.stats()['queued_buffers']==0
  assert guest.call('stop',driver)==0
  out=dict(ok=True,mode='independent_process_restore',before=before,after=after,events=events,expected_events=expected,exact_image_restored=True,guest_data_restored=True,guest_indirect_stdcall_count=len(guest.abi.calls),guest_abi_calls=guest.abi.calls,old_buffer_handle=hex(old),new_buffer_handle=hex(new),stale_handle_status=old_status,wrong_driver_kind_status=wrong_status,device_recreated=True,miles_contract_verified=False)
  (R/'evidence/stage37-independent-restore.json').write_text(json.dumps(out,indent=2))
  print(json.dumps({k:out[k]for k in ['ok','mode','after','old_buffer_handle','new_buffer_handle','stale_handle_status']}))
 finally:native.dispose()

def initial():
 native=NativeAudio();producer=None;errors=[];submissions=[]
 try:
  guest=Guest32(native)
  assert guest.call('open',DRIVER_OUT)==0
  driver=guest.word(DRIVER_OUT);assert driver==0x5c000101
  frame_counts=[1024,16384,4096]+[128]*13
  bodies=[pcm(i,n)for i,n in enumerate(frame_counts)]
  handles=[]
  for i,body in enumerate(bodies):
   guest.uc.mem_write(PCM,body)
   assert guest.call('prepare',driver,PCM,len(body)//4,HANDLE_OUT+i*4)==0
   h=guest.word(HANDLE_OUT+i*4);assert h>>24==0x5d;handles.append(h)
   guest.uc.mem_write(PCM,b'\x7f'*len(body))
  for h in handles[:3]:assert guest.call('queue',driver,h)==0
  def submit():
   try:
    other=Guest32(native)
    for h in handles[3:]:
     time.sleep(.020)
     status=other.call('queue',driver,h)
     submissions.append(dict(handle=h,status=status))
    submissions.append(dict(producer_abi_calls=len(other.abi.calls)))
   except BaseException as exc:errors.append(repr(exc))
  assert guest.call('start',driver)==0
  producer=threading.Thread(target=submit);producer.start()
  deadline=time.monotonic()+3
  while native.stats()['source_frames']<1800 and time.monotonic()<deadline:time.sleep(.002)
  assert native.stats()['source_frames']>=1800
  assert guest.call('stop',driver)==0
  producer.join(timeout=2);assert not producer.is_alive() and not errors,errors
  stopped=native.stats();b=native.snapshot();time.sleep(.030)
  assert native.snapshot()==b and native.stats()==stopped
  accepted=[r['handle']for r in submissions if r.get('status')==0]
  rejected=[r['handle']for r in submissions if r.get('status')==2]
  assert accepted and rejected,submissions
  statuses=[r['status']for r in submissions if 'status'in r]
  assert statuses==[0]*len(accepted)+[2]*len(rejected),statuses
  assert stopped['pending_events']>=1 and stopped['queued_buffers']>=2
  assert 1024<stopped['source_frames']<1024+16384
  assert stopped['bad_buffers']==stopped['active_callbacks']==0
  queued=handles[:3]+accepted
  indices=[handles.index(h)for h in queued]
  full_pcm=b''.join(bodies[i]for i in indices)
  header=struct.unpack_from('<12I5Q',b,0)
  # Buffer B is partially consumed; its snapshot cursor is the exact resume point.
  buffers_offset=88+32*4+32*16
  cursor=struct.unpack_from('<I',b,buffers_offset+(65536+32)*1+8)[0]
  assert 0<cursor<16384 and cursor==stopped['source_frames']-1024
  SAVE.write_bytes(b);guest_image=guest.image();GUEST_SAVE.write_bytes(guest_image)
  spec=dict(snapshot_sha256=sha(b),snapshot_bytes=len(b),guest_image_sha256=sha(guest_image),frozen_stats=stopped,all_handles=handles,expected_events=[[h,i+1,frame_counts[handles.index(h)],0]for i,h in enumerate(queued)],expected_source_frames=len(full_pcm)//4,expected_source_fnv64=fnv(full_pcm),partially_consumed_buffer_B_cursor=cursor,producer_submissions=submissions,authored_PCM_copied_before_guest_overwrite=True)
  SPEC.write_text(json.dumps(spec,indent=2));native.dispose();native=None
  with (R/'evidence/stage37-independent-restore.stdout.log').open('wb')as out,(R/'evidence/stage37-independent-restore.stderr.log').open('wb')as err:
   child=subprocess.run([sys.executable,str(Path(__file__).resolve()),'--restore'],stdout=out,stderr=err,cwd=R,timeout=10)
  assert child.returncode==0,child.returncode
  restored=json.loads((R/'evidence/stage37-independent-restore.json').read_text())
  assert restored['ok']
  out=dict(ok=True,scope='authored i386 indirect stdcall -> authored arm64 CoreAudio queue; no Miles/game execution',frozen=spec,guest_indirect_stdcall_count=len(guest.abi.calls),guest_abi_calls=guest.abi.calls,stop_froze_image_30ms=True,producer_accepted_before_stop=len(accepted),producer_rejected_after_stop=len(rejected),independent_restore=restored,original_game_checkpoint='runs/stage12-state-final35/',original_game_advanced=False,mss36_scope_approved=False,mss36_probe_executed=False)
  (R/'evidence/stage37-integrated.json').write_text(json.dumps(out,indent=2))
  print(json.dumps(dict(ok=True,frozen_stats=stopped,partial_B_cursor=cursor,accepted=len(accepted),rejected=len(rejected),final_source_frames=restored['after']['source_frames'],source_hash_equal=True,completion_events_equal=True,guest_indirect_stdcall_count=len(guest.abi.calls)+restored['guest_indirect_stdcall_count']+13)))
 finally:
  if producer and producer.is_alive():producer.join(timeout=2)
  if native:native.dispose()

if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('--restore',action='store_true');args=parser.parse_args()
 restore()if args.restore else initial()
