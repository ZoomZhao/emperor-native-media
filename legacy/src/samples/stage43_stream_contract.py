import hashlib,json,struct
from pathlib import Path
R=Path(__file__).resolve().parents[1];d=R/'runs/stage12-state-final43';m=json.loads((d/'state.json').read_text());f=(d/'memory.bin').open('rb')
def body(a,n):
 for q in m['regions']:
  if q['low']<=a and a+n<=q['high']+1:f.seek(q['offset']+a-q['low']);b=f.read(n);assert len(b)==n;return b
 raise ValueError(hex(a))
def word(a):return struct.unpack('<I',body(a,4))[0]
def text(a):
 b=bytearray()
 for i in range(260):
  c=body(a+i,1)[0]
  if not c:return b.decode('ascii')
  b.append(c)
 raise ValueError('missing NUL')
r=m['registers'];sp=r['esp'];bp=r['ebp'];a=[word(sp+4+i*4)for i in range(3)];path=text(a[1]);assert path.lower()=='audio\\music\\menu1-san_xuan.mp3'and a[0]==0x5c000101 and a[2]==256000
frame=dict(EBP=hex(bp),caller_return=hex(word(bp+4)),saved_EBP=hex(word(bp)),path_pointer=hex(word(bp+8)),stream_slot_index=word(bp+12),loop_count_argument=word(bp+16),audio_owner=hex(word(bp-8)))
owner=word(bp-8);slot=frame['stream_slot_index'];volume_field=340 if slot==1 else 344 if slot==2 else 332;frame['effective_volume_before_open']=word(owner+volume_field)
names={}
for dll in json.loads((R/'evidence/pe-metadata.json').read_text())[0]['imports']:
 for q in dll['symbols']:
  va=int(q['iat_va'],16)
  if va in [8033900,8033904,8033916,8033908]:names[hex(va)]=dict(dll=dll['dll'],name=q.get('name'),IAT=hex(va))
rows=[]
for root in [R/'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]',Path('<LOCAL_MACHINE_PATH>]')]:
 p=root/'Audio/Music/MENU1-San_Xuan.mp3';b=p.read_bytes();digest=hashlib.sha256(b).hexdigest();assert len(b)==5050669 and digest=='27fecd0eaa705f65ab37ae2a849b57c2727c6bbc2ed7b33d14e308193374b1e8';rows.append(dict(path=str(p),bytes=len(b),sha256=digest,read_only=True))
out=dict(original_EXE_sha256=m['original_EXE_sha256'],main_PC=hex(r['eip']),actual_next_API='_AIL_open_stream@12',actual_API_return=hex(word(sp)),actual_args=a,path=path,actual_original_wrapper_frame=frame,original_following_imports=names,music_files=rows,Apple_afinfo_metadata=dict(channels=2,sample_rate=44100,codec='MP3',estimated_duration_seconds=157.829406,bitrate_bits_per_second=256000),current_native_PCM_backend=dict(sample_rate=22050,channels=2,format='signed16'),original_MSS_stream_open_or_play_executed=False,actual_nonempty_stream_handle_created=False,missing_file_failure_applicable=False,required_contracts=['actual compressed-file open/decode plus PCM conversion or resampling','real owned stream lifecycle and driver binding','loop-count interpretation, volume/mixing, actual start','buffer service, EOS/loop events, stop/close and persistent restore'],natural_screen_mode=body(17686597,1)[0],natural_gate=word(0x88ec38),natural_city_phase=word(0xc82ef8))
(R/'evidence/stage43-stream-contract.json').write_text(json.dumps(out,ensure_ascii=False,indent=2));print(json.dumps(out,ensure_ascii=False))
