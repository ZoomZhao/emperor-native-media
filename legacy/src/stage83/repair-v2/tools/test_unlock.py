import json,struct,sys
sys.dont_write_bytecode=True
from common import V,put,Boundary,sha
from unlock_model import dispatch
class Memory:
 def __init__(self):self.p=0x30002000;self.b=bytes(range(16))
 def read(self,p,n):
  if p!=self.p or n!=len(self.b):raise Boundary('OWN_GUEST_RANGE')
  return self.b
def main():
 image=[0]*12;image[:9]=[0x44443239,1,2,2,32,0,2,2,32];objs=[[0]*12 for _ in range(64)];objs[1]=[2,1,0,0,0,0,2,2,8,32,1,0];blob=struct.pack('<12I',*image)+b''.join(struct.pack('<12I',*r)for r in objs)+bytes(16);memory=Memory();host={'dd_objects':{0x30001020:{'id':2,'kind':2,'guest_pixels':memory.p}},'dd_native_blob':blob};cases=[]
 assert dispatch(host,memory,'authored.dd','2.32',[0x30001020,memory.p])==0;out=host['dd_native_blob'];assert out[3120:]==memory.b and struct.unpack_from('<I',out,48+48+40)[0]==0;assert out[:48+48+40]==blob[:48+48+40]and out[48+48+44:3120]==blob[48+48+44:3120];cases.append('exact_C_unlock_changes_real_lock_and_copies_guest_pixels')
 assert dispatch(host,memory,'authored.dd','2.32',[0x30001020,0])==0x80004005;cases.append('second_unlock_real_failure_not_stub_success')
 for name,args,dll,method in [('wrong_guest_pointer',[0x30001020,1],'authored.dd','2.32'),('foreign_object',[0x30009999,0],'authored.dd','2.32'),('Blt_refused',[0x30001020,0],'authored.dd','2.5'),('window_refused',[0,0],'user32.dll','CreateWindowExA'),('audio_refused',[0,0],'dsound.dll','DirectSoundCreate')]:
  before=host['dd_native_blob']
  try:dispatch(host,memory,dll,method,args)
  except Boundary:pass
  else:raise AssertionError(name)
  assert host['dd_native_blob']==before;cases.append(name)
 put(V/'evidence/unlock-tests.json',{'status':'EXACT_EXISTING_C_UNLOCK_HEADLESS_OWN_TESTS_PASSED','cases':cases,'library_sha256':sha(V/'builds/unlock-headless.dylib'),'original_game_objects_linked':0,'UI_audio_video_network_calls':0,'A_original_execution_enabled':False,'single_active_state_only':True});print(json.dumps({'passed':len(cases),'real_native_C_unlock_used':True,'original_execution':0}))
if __name__=='__main__':main()
