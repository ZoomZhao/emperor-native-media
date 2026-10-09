import json,time,wave,struct,math,hashlib
from pathlib import Path
from stage37_audio import NativeAudio
from stage44_stream import NativeStreams
R=Path(__file__).resolve().parents[1];audio=NativeAudio();audio.open();s=NativeStreams(audio);events=[]
try:
 wav=R/'runs/stage44-native/own-tone.wav'
 with wave.open(str(wav),'wb')as f:f.setnchannels(2);f.setsampwidth(2);f.setframerate(22050);f.writeframes(b''.join(struct.pack('<hh',int(math.sin(i*0.1)*10000),int(math.cos(i*0.1)*5000))for i in range(1000)))
 for loops in [1,2,0]:
  e,h=s.open(wav);assert e==0 and h;assert s.command(h,1,loops)==s.command(h,2,20)==s.command(h,3)==0
  assert s.fn['pull'](s.state,2300)==0;q=s.stats();events.append(dict(test='own WAV explicit offline pull',loops=loops,stats=q));assert q['consumed_frames']==(1000*loops if loops else 2300)and q['status']==(2 if loops else 4);assert q['queued_passes']==loops if loops else q['queued_passes']>=2
  assert s.command(h,4,1)==0;paused=s.stats();assert s.fn['pull'](s.state,100)==0 and s.stats()==paused
  blob=s.snapshot();s.restore(blob);assert s.snapshot()==blob;assert s.command(h,5)==0 and s.fn['status'](s.state,h)==-1
 music=Path(json.loads((R/'evidence/stage43-stream-contract.json').read_text())['music_files'][0]['path']);e,h=s.open(music);assert e==0 and h;opened=s.stats();assert opened['decoded_frames']>3000000 and opened['decoder_file_open']==1
 assert s.command(h,1,0)==s.command(h,2,20)==s.command(h,3)==0;assert audio.start()==0;time.sleep(0.25);assert s.service()==0;assert audio.stop()==0;q=s.stats();assert q['consumed_frames']>0 and q['nonzero_mixed_samples']>0 and q['queued_frames']>0
 blob=s.snapshot();s.restore(blob);assert s.snapshot()==blob;assert s.command(h,5)==0;closed=s.stats();assert closed['decoder_file_open']==0 and audio.stats()['source_frames']==0
 result=dict(own_fixture_tests=events,music_open=opened,music_offline_consumption=q,checkpoint_bytes=len(blob),checkpoint_sha256=hashlib.sha256(blob).hexdigest(),independent_redecode_restore=True,music_closed=closed,native_audio=audio.stats(),source_PCM_sent_to_device=False,original_DLL_executed=False)
 (R/'evidence/stage44-stream-probe.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
finally:s.dispose();audio.dispose()
