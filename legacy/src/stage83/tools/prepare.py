"""Only this ABC write-boundary revision; old Stage82 stays immutable."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage83'
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
def main():
 for d in ('tools','configs','evidence','runs','builds','reports'):(S/d).mkdir(parents=True,exist_ok=True)
 prior=json.loads((ROOT/'stage82/evidence/protection-baseline.json').read_text())['hashes'];prior.update(json.loads((ROOT/'stage82/manifest.json').read_text())['hashes']);prior['stage82/manifest.json']=sha(ROOT/'stage82/manifest.json');(S/'evidence/protection-baseline.json').write_text(json.dumps({'hashes':prior,'count':len(prior)},indent=2)+'\n')
 names=('common.py','pipeline.py','engine.py','platform_model.py','activity.py','compare.py','runtime.c','runtime.h','platform_native.c','heap_connection.c','domains.c','build_drivers.py','authored-platform.ll','bounded.py','native_driver.c')
 for n in names:
  text=(ROOT/'stage82/tools'/n).read_text()
  if n=='common.py':text=text.replace("S=ROOT/'stage82'","S=ROOT/'stage83'")
  with(S/'tools'/n).open('x')as f:f.write(text)
 original=json.loads((ROOT/'stage82/configs/original.json').read_text());original.update(id='S83_write_bounded_natural_phase8_ABC_once',pins={},native_driver='stage83/builds/original-v1/native-driver',approved_output_root=str(S/'runs/original-ABC-once'),host_write_policy='stage83/configs/abc-write-policy.sb');(S/'configs/original.json').write_text(json.dumps(original,indent=2)+'\n')
 own=json.loads((ROOT/'stage82/configs/authored-platform.json').read_text());own.update(id='S83_authored_write_boundary_fixture',pins={},native_driver='stage83/builds/authored-v1/native-driver',authored_IR='stage83/tools/authored-platform.ll',approved_output_root=str(S/'runs/authored-safe-once'),host_write_policy='stage83/configs/abc-write-policy.sb');(S/'configs/authored.json').write_text(json.dumps(own,indent=2)+'\n')
 print(json.dumps({'old_hashes_preserved':len(prior),'new_stage':'stage83','original_execution':0}))
if __name__=='__main__':main()
