import ctypes,hashlib,json,time,struct
from pathlib import Path
R=Path(__file__).resolve().parents[1]
def main():
 path=R/'runs/stage36-native/libstage36_audio.dylib';lib=ctypes.CDLL(str(path));fn={};v=ctypes.c_void_p;z=ctypes.c_size_t
 for name,(ret,args)in {'new':(v,[]),'open':(ctypes.c_int,[v]),'start':(ctypes.c_int,[v]),'stop':(ctypes.c_int,[v]),'status':(ctypes.c_int,[v,ctypes.POINTER(ctypes.c_uint32)]),'size':(z,[]),'export':(ctypes.c_int,[v,v,z]),'import':(ctypes.c_int,[v,v,z]),'delete':(None,[v])}.items():f=getattr(lib,'s36_'+name);f.restype=ret;f.argtypes=args;fn[name]=f
 states=[];events=[]
 def status(s):
  values=(ctypes.c_uint32*5)();assert fn['status'](s,values);return dict(zip(['OSStatus_unsigned','actual_default_output_device','initialized','started','generation'],values))
 def snap(s):
  b=ctypes.create_string_buffer(fn['size']());assert fn['export'](s,b,len(b));return b.raw
 def inspect(b):
  v=struct.unpack('<10I5Q',b);return dict(zip(['magic','version','rate','channels','bits','device','initialized','started','generation','bad_buffers','callbacks','frames','bytes','first_host','last_host'],v))
 try:
  s=fn['new']();states.append(s);ok=fn['open'](s);events.append(dict(step='actual AudioUnitInitialize',succeeded=bool(ok),status=status(s)))
  if not ok:raise RuntimeError('actual CoreAudio initialization failed')
  assert fn['start'](s);events.append(dict(step='actual AudioOutputUnitStart',status=status(s)));time.sleep(.35);assert fn['stop'](s);b=snap(s);first=inspect(b);time.sleep(.03);assert snap(s)==b;(R/'evidence/stage36-native-audio.state.bin').write_bytes(b);events.append(dict(step='stop and snapshot',state=first,no_callbacks_after_stop=True));assert first['callbacks']>0 and first['frames']>0 and first['bytes']==first['frames']*4 and first['bad_buffers']==0 and first['last_host']>first['first_host']
  assert not fn['stop'](s);fn['delete'](s);states.remove(s);s2=fn['new']();states.append(s2);assert fn['import'](s2,ctypes.create_string_buffer(b),len(b));assert snap(s2)==b;events.append(dict(step='recreate actual AudioUnit and restore quiescent state',byte_equal=True,status=status(s2)));assert fn['start'](s2);time.sleep(.25);assert fn['stop'](s2);last=inspect(snap(s2));assert last['callbacks']>first['callbacks'] and last['frames']>first['frames'] and last['bad_buffers']==0;events.append(dict(step='real callbacks continue after restore',state=last))
  out=dict(passed=True,native_arch='arm64',library_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),events=events,PCM='22050Hz stereo signed little-endian S16, actual silent render buffers',original_MSS_or_provider_code_executed=False,guest_game_state_advanced=False,scope='native host audio initialization/callback/clock/quiescent state only; Miles provider and guest callback equivalence unproven');(R/'evidence/stage36-native-audio.json').write_text(json.dumps(out,indent=2));print(json.dumps(dict(passed=True,device=last['device'],callbacks=last['callbacks'],frames=last['frames'],state_recreated_and_byte_equal=True)))
 except Exception as error:
  (R/'evidence/stage36-native-audio.json').write_text(json.dumps(dict(passed=False,error=str(error),events=events),indent=2));raise
 finally:
  for s in states:fn['delete'](s)
if __name__=='__main__':main()
