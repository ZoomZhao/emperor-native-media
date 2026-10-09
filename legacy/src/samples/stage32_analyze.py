"""Read-only pixel and protected-input evidence; executes no original code."""
import hashlib,json,struct
from pathlib import Path
R=Path(__file__).resolve().parents[1]
class Memory:
 def __init__(self,tag):
  p=R/f'runs/stage12-state-{tag}';self.m=json.loads((p/'state.json').read_text());self.f=(p/'memory.bin').open('rb')
 def read(self,address,n):
  for s in self.m['regions']:
   if s['low']<=address and address+n<=s['high']+1:self.f.seek(s['offset']+address-s['low']);return self.f.read(n)
  raise ValueError(hex(address))
early=Memory('jpeg32');late=Memory('present32');a=json.loads((R/'evidence/stage32-jpeg32.json').read_text());b=json.loads((R/'evidence/stage32-present32.json').read_text());meta=a['jpeg_output_hashes']['data\\china_load1.jpg'];bgr=early.read(meta['guest_pointer'],meta['bytes']);assert hashlib.sha256(bgr).hexdigest()==meta['sha256'];surface=early.read(a['dd_objects']['805310560']['guest_pixels'],960000);expected=bytearray(960000)
for i in range(480000):
 blue,green,red=bgr[i*3:i*3+3];struct.pack_into('<H',expected,i*2,((red>>3)<<10)|((green>>3)<<5)|(blue>>3))
conversion=dict(pixels=480000,original_function='0x525910 executed unmodified in Unicorn',comparison='independent packed RGB555 formula over actual ImageIO BGR input before original free',byte_mismatches=sum(x!=y for x,y in zip(surface,expected)),actual_SHA256=hashlib.sha256(surface).hexdigest(),original_IJL_reference_pixels_available=False,source_BGR_sha256=hashlib.sha256(bgr).hexdigest());(R/'evidence/stage32-original-BGR-RGB555.json').write_text(json.dumps(conversion,indent=2));assert not conversion['byte_mismatches']
dst=late.read(b['dd_objects']['805310544']['guest_pixels'],1572864);expected=bytearray(len(dst))
for y in range(600):expected[(y+84)*2048+112*2:(y+84)*2048+912*2]=surface[y*1600:(y+1)*1600]
copy=dict(original_copy_return_pc='0x526005',source_rect=[0,0,800,600],destination_rect=[112,84,912,684],original_RGB555_work_surface_bytes=len(dst),full_surface_byte_mismatches=sum(x!=y for x,y in zip(dst,expected)),RGB555_surface_SHA256=hashlib.sha256(dst).hexdigest(),RGB555_discarded_component_bits_max=max(v&7 for v in bgr),RGB555_discarded_component_bits_mean=sum(v&7 for v in bgr)/len(bgr),original_IJL_decoder_difference='unmeasured',original_IJL_executed=False,next_text=late.read(272992425,28).decode('ascii'));(R/'evidence/stage32-original-surface-copy.json').write_text(json.dumps(copy,indent=2));assert not copy['full_surface_byte_mismatches']
extra=[]
for name in ['stage31-next-JPEG-file.json','stage31-IJL-readonly.json']:
 row=json.loads((R/'evidence'/name).read_text())
 for key in ['copy_path','original_path']:
  p=Path(row[key]);sha=hashlib.sha256(p.read_bytes()).hexdigest();extra.append(dict(path=str(p),sha256=sha,unchanged=sha==row['sha256']))
assert all(r['unchanged']for r in extra);(R/'evidence/stage32-extra-input-audit.json').write_text(json.dumps(dict(all_four_JPEG_IJL_source_copy_hashes_unchanged=True,checks=extra),indent=2));print(json.dumps(dict(original_conversion_byte_mismatches=conversion['byte_mismatches'],original_work_surface_byte_mismatches=copy['full_surface_byte_mismatches'],extra_readonly_inputs_unchanged=True,next_text=copy['next_text'])))
