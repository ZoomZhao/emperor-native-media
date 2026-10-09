import json,wave,struct,math,time
from pathlib import Path
from stage37_audio import NativeAudio
from stage50_close_stream import NativeStreams50,StreamCloser
R=Path(__file__).resolve().parents[1];audio=NativeAudio();audio.open();s=NativeStreams50(audio);closer=StreamCloser(s);events=[]
try:
 fixture=R/'runs/stage50-native/own-close-tone.wav'
 with wave.open(str(fixture),'wb')as f:f.setnchannels(2);f.setsampwidth(2);f.setframerate(22050);f.writeframes(b''.join(struct.pack('<hh',int(math.sin(i*.1)*10000),int(math.cos(i*.1)*5000))for i in range(1000)))
 empty=closer.close(0);assert empty['result']==0 and empty['null_noop'];events.append(dict(test='empty NULL stream, no resource mutation',result=empty))
 rc,h=s.open(fixture);assert rc==0;assert s.command(h,1,0)==s.command(h,3)==0;assert audio.start()==0;time.sleep(.1);assert s.service()==0;before=closer.diagnostic();assert before['audio_phase']==1 and before['audio_callbacks']>0 and before['status']==4
 rc,status=closer.raw_close(h);after=closer.diagnostic();assert rc==-20 and after['handle']==h and after['decoder_file_open']==after['PCM_allocation_live']==1 and after['generation']==before['generation'];events.append(dict(test='close while real device running rejected before release',rc=rc,before=before,after=after))
 active_null=closer.close(0);assert active_null['result']==0 and active_null['after']['handle']==h and active_null['after']['decoder_file_open']==active_null['after']['PCM_allocation_live']==1;events.append(dict(test='NULL close with unrelated live stream retains all resources',result=active_null))
 stale=closer.close(h+0x100);assert stale['result']==-2 and closer.diagnostic()['handle']==h;events.append(dict(test='wrong generation while active rejected',result=stale))
 closed=closer.close(h);assert closed['result']==0 and closed['ExtAudioFileDispose_OSStatus']==0 and closed['real_AudioUnit_stop_result']==closed['real_silent_clock_restart_result']==0;events.append(dict(test='active stream actual stop, quiescent callbacks, checked file dispose/free, clock restart',result=closed))
 assert closer.raw_close(h)[0]==-2;rc,h2=s.open(fixture);assert rc==0 and h2==h+0x100;assert s.fn['status'](s.state,h)==-1;events.append(dict(test='reopen uses new generation and rejects old token',old=h,new=h2))
 assert s.command(h2,3)==0;assert s.command(h2,4,1)==0;assert audio.stop()==0;blob=s.snapshot();s.restore(blob);assert s.restore_release50['ExtAudioFileDispose_OSStatus']==0 and s.snapshot()==blob;closed2=closer.close(h2);assert closed2['result']==0;events.append(dict(test='paused semantic restore with checked old decoder release and real release',restore_release=s.restore_release50,result=closed2))
 assert audio.stats()['active_callbacks']==audio.stats()['queued_buffers']==audio.stats()['pending_events']==audio.stats()['source_frames']==0
 blob=bytearray(s.snapshot());struct.pack_into('<I',blob,8,65536);s.restore(bytes(blob));frozen=s.snapshot();assert s.open(fixture)==(-50,0)and s.snapshot()==frozen;events.append(dict(test='retired generation cannot wrap/reopen',generation=65536))
 out=dict(passed=True,events=events,real_device_callbacks_observed=True,actual_stopped_callback_queue_gate=True,original_MSS_exports_executed=False,source_PCM_sent_to_device=False,native_audio=audio.stats());(R/'evidence/stage50-close-probe.json').write_text(json.dumps(out,indent=2));print(json.dumps(dict(passed=True,cases=len(events),source_frames=audio.stats()['source_frames'])))
finally:s.dispose();audio.dispose()
