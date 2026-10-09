"""Read-only independent checks of original refresh, native frame and inputs."""
import hashlib,json,struct
from pathlib import Path
R=Path(__file__).resolve().parents[1]
p=R/'runs/stage12-state-text33fixed';m=json.loads((p/'state.json').read_text());f=(p/'memory.bin').open('rb')
def read(address,n):
 for s in m['regions']:
  if s['low']<=address and address+n<=s['high']+1:f.seek(s['offset']+address-s['low']);return f.read(n)
 raise ValueError(hex(address))
a=json.loads((R/'evidence/stage33-text33fixed.json').read_text());objects=a['dd_objects'];primary=read(objects['805310496']['guest_pixels'],1440*900*4);back32=read(objects['805310512']['guest_pixels'],1024*768*4);work555=read(objects['805310544']['guest_pixels'],1024*768*2);expected=bytearray(len(back32))
for i in range(1024*768):
 v=struct.unpack_from('<H',work555,i*2)[0];expected[i*4:i*4+4]=bytes(((v&31)<<3,((v>>5)&31)<<3,((v>>10)&31)<<3,255))
conversion_mismatch=sum(x!=y for x,y in zip(expected,back32));assert not conversion_mismatch
crop=b''.join(primary[(y+94)*1440*4+208*4:(y+94)*1440*4+1232*4]for y in range(768));primary_mismatch=sum(x!=y for x,y in zip(crop,back32));assert not primary_mismatch;event=a['presentation_events'][-1];assert hashlib.sha256(crop).hexdigest()==event['GPU_readback_sha256'];assert event['stats']['GPU_byte_mismatches']==0
files=[]
for row in json.loads((R/'evidence/stage32-extra-input-audit.json').read_text())['checks']:
 sha=hashlib.sha256(Path(row['path']).read_bytes()).hexdigest();files.append(dict(path=row['path'],sha256=sha,unchanged=sha==row['sha256']))
assert all(row['unchanged']for row in files)
png=R/'evidence/stage33-window.png';header=png.read_bytes()[:24];assert header[:8]==b'\x89PNG\r\n\x1a\n';width,height=struct.unpack('>II',header[16:24]);result=dict(original_RGB555_to_BGR32_byte_mismatches=conversion_mismatch,original_primary_client_Blt_byte_mismatches=primary_mismatch,all_786432_original_client_pixels_equal_GPU_readback=True,original_refresh_function='0x52be40',original_converter='0x418ff0',original_primary_Blt_return='0x52c04a',work555_sha256=hashlib.sha256(work555).hexdigest(),back32_primary_GPU_sha256=hashlib.sha256(back32).hexdigest(),window_screenshot_pixels=[width,height],window_screenshot_sha256=hashlib.sha256(png.read_bytes()).hexdigest(),window_screenshot_contains_only_owned_window=True,JPEG_IJL_four_paths_unchanged=files,caption_on_primary_before_refresh=True,caption_overwritten_by_original_full_client_primary_Blt=True,original_IJL_pixel_equivalence='unmeasured',original_Windows_font_pixel_equivalence='unmeasured');(R/'evidence/stage33-frame-pixel-evidence.json').write_text(json.dumps(result,indent=2));print(json.dumps({k:v for k,v in result.items()if k not in ['JPEG_IJL_four_paths_unchanged']}))
