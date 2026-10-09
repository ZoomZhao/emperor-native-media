import ctypes,hashlib,json,struct,time
from pathlib import Path
R=Path(__file__).resolve().parents[1]
class StreamCloser:
 def __init__(self,stream):
  self.stream=stream;self.audio=stream.audio;m=json.loads((R/'evidence/stage50-stream-close-build.json').read_text());p=Path(m['library']);assert hashlib.sha256(p.read_bytes()).hexdigest()==m['sha256'];self.lib=ctypes.CDLL(str(p));self.diagnostic_c=self.lib.s50_stream_diag;self.diagnostic_c.restype=ctypes.c_int;self.diagnostic_c.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint64)];self.close_c=self.lib.s50_close_quiescent;self.close_c.restype=ctypes.c_int;self.close_c.argtypes=[ctypes.c_void_p,ctypes.c_uint32,ctypes.POINTER(ctypes.c_int32)]
 def diagnostic(self):
  v=(ctypes.c_uint64*14)();assert self.diagnostic_c(self.stream.state,v)==0;return dict(zip(['handle','generation','status','queued_frames','decoded_frames','consumed_frames','decoder_file_open','PCM_allocation_live','audio_phase','active_audio_callbacks','device_queued_buffers','device_pending_events','audio_callbacks','audio_render_frames'],v))
 def raw_close(self,h):
  status=ctypes.c_int32();rc=self.close_c(self.stream.state,h,ctypes.byref(status));return rc,status.value
 def close(self,h):
  before=self.diagnostic();out=dict(handle=h,before=before,source_PCM_sent_to_device=False,stream_service_thread='current owner only; no decoder/stream pointer reachable from AudioUnit render callback',original_MSS_callbacks_equivalence=False)
  if h==0:
   rc,status=self.raw_close(h);after=self.diagnostic();assert all(after[k]==before[k]for k in ['handle','generation','status','queued_frames','decoded_frames','consumed_frames','decoder_file_open','PCM_allocation_live']);out.update(result=rc,ExtAudioFileDispose_called=False,null_noop=True,after=after);return out
  if h!=before['handle']:out.update(result=-2,invalid_generation_or_handle=True,after=self.diagnostic());return out
  running=before['audio_phase']==1
  if running:
   rc=self.audio.stop();out['real_AudioUnit_stop_result']=rc
   if rc:out.update(result=rc,after=self.diagnostic());return out
  quiescent=self.audio.stats();out['audio_quiescent']=quiescent
  if quiescent['phase']not in[0,3]or quiescent['active_callbacks']or quiescent['queued_buffers']or quiescent['pending_events']:out.update(result=-20,after=self.diagnostic());return out
  frozen=self.audio.snapshot();time.sleep(0.003);assert self.audio.snapshot()==frozen;out['real_device_callbacks_and_queue_stable_while_stopped']=True
  rc,status=self.raw_close(h);out.update(result=rc,ExtAudioFileDispose_OSStatus=status,ExtAudioFileDispose_called=True,closed=self.diagnostic())
  if rc:return out
  after=out['closed'];assert after['handle']==after['status']==after['queued_frames']==after['decoded_frames']==after['decoder_file_open']==after['PCM_allocation_live']==0 and after['generation']==before['generation']+1;assert self.stream.fn['status'](self.stream.state,h)==-1;out['old_generation_handle_rejected']=True
  if running:
   restart=self.audio.start();out['real_silent_clock_restart_result']=restart
   if restart:out['result']=restart;return out
  out['after']=self.diagnostic();assert self.audio.stats()['source_frames']==0;return out
def generation_safe_open(stream,path,bytes=256000):
 # A retired generation is serializable but cannot be opened or wrapped.
 b=stream.snapshot();generation=struct.unpack_from('<I',b,8)[0]
 if not 1<=generation<=65535:return -50,0
 from stage44_stream import NativeStreams
 return NativeStreams.open(stream,path,bytes)
from stage44_stream import NativeStreams
class NativeStreams50(NativeStreams):
 def open(self,path,bytes=256000):return generation_safe_open(self,path,bytes)
 def restore(self,b):
  magic,version,generation,handle=struct.unpack_from('<4I',b)
  assert magic==0x5334344d and version==1 and 1<=generation<=65536 and(not handle or generation<=65535 and handle==(0x5f000000|(generation<<8)|1))
  # A semantic restore replaces host resources only after the device is stopped.
  # Its saved guest generation is intentionally preserved, not newly allocated.
  a=self.audio.stats();assert a['phase']in[0,3]and a['active_callbacks']==a['queued_buffers']==a['pending_events']==0
  closer=StreamCloser(self);h=closer.diagnostic()['handle']
  self.restore_release50=None
  if h:
   self.restore_release50=closer.close(h);assert self.restore_release50['result']==0,self.restore_release50
  return super().restore(b)
 def dispose(self):
  if self.state:
   closer=StreamCloser(self);h=closer.diagnostic()['handle']
   if h:result=closer.close(h);assert result['result']==0,result
  return super().dispose()
