import hashlib,json,re,struct,os,errno
from pathlib import Path
from auto_i386_analysis import PE
R=Path(__file__).resolve().parents[1];manifest=json.loads((R/'evidence/stage41-Bink-input.readonly.json').read_text());p=PE(manifest['copy_path']);assert hashlib.sha256(p.data).hexdigest()==manifest['sha256']
lines=(R/'evidence/stage41-Bink-Miles-binding.readonly.log').read_text().splitlines();strings=[]
for line in lines:
 if 'pushl'not in line:continue
 m=re.search(r'# imm = (0x3004[0-9a-fA-F]+)',line)
 if not m:continue
 addr=int(m[1],16)
 try:text=p.read(addr,128).split(b'\0')[0].decode('ascii')
 except (UnicodeDecodeError,ValueError):continue
 strings.append(dict(address=hex(addr),text=text))
d=json.loads((R/'runs/stage12-state-stream41/state.json').read_text());path=R/'runs/stage12-state-stream41/memory.bin'
def mem(addr,n):
 q=next(q for q in d['regions']if q['low']<=addr and addr+n<=q['high']+1)
 with path.open('rb')as f:f.seek(q['offset']+addr-q['low']);return f.read(n)
sp=d['registers']['esp'];args=list(struct.unpack('<2I',mem(sp+4,8)));assert args==[0x61001a70,0x5c000101]
filename_ptr=struct.unpack('<I',mem(sp+220,4))[0];filename=mem(filename_ptr,128).split(b'\0')[0].decode('ascii');assert filename in ['Binks\\emperor_intro.bik','Binks\\emperor_intro_hiq.bik']
files=[];absences=[]
for name in ['emperor_intro.bik','emperor_intro_hiq.bik','新建文件夹/emperor_intro_hiq.bik']:
 for root,label in [(R/'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]','copy'),(Path('<LOCAL_MACHINE_PATH>]'),'original')]:
  q=root/'Binks'/name
  try:fd=os.open(q,os.O_RDONLY|os.O_NOFOLLOW)
  except FileNotFoundError as e:
   assert e.errno==errno.ENOENT and q.parent.is_dir();absences.append(dict(name=name,which=label,path=str(q),actual_errno=e.errno,read_only=True));continue
  else:os.close(fd)
  b=q.read_bytes();head=struct.unpack('<4s9I',b[:40]);files.append(dict(name=name,which=label,path=str(q),bytes=len(b),sha256=hashlib.sha256(b).hexdigest(),header_signature=head[0].decode('ascii'),header_raw_DWORDs=list(head[1:]),read_only=True,decoded=False))
for name in ['新建文件夹/emperor_intro_hiq.bik']:
 a=[q for q in files if q['name']==name];assert len({q['sha256']for q in a})==1
out=dict(current_PC=hex(d['registers']['eip']),pending_API='BinkSetSoundSystem',callback_guest32=hex(args[0]),callback_import='_BinkOpenMiles@4',driver_guest32=hex(args[1]),filename_original_call_argument=filename,filename_guest32=hex(filename_ptr),filename_stack_derivation='original movie entry args: at SetSoundSystem thunk ESP + 220 after sub200/pushesi/pushdriver/pushcallback/call',setter_calls_callback_immediately_at='30003cdf call eax(driver)',setter_returns_success_only_if_callback_result_saved_nonzero=True,binding_symbols=strings,original_Miles_symbols_required=sorted({q['text']for q in strings if q['text'].startswith('_AIL_')}),movie_files=files,requested_path_absences=absences,original_Bink_Miles_code_executed=False,bindings_registered=False,decoder_or_audio_callback_equivalence_proven=False)
(R/'evidence/stage41-Bink-contract.readonly.json').write_text(json.dumps(out,ensure_ascii=False,indent=2));print(json.dumps({k:v for k,v in out.items()if k!='movie_files'},ensure_ascii=False,indent=2))
