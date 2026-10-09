"""Hash legal model TXT inputs once; fixed paths and real native reads only."""
import hashlib,json
from pathlib import Path
R=Path(__file__).resolve().parents[1]
def main():
 old=json.loads((R/'evidence/stage31-next-JPEG-file.json').read_text());copy=Path(old['copy_path']).parent.parent/'Model';original=Path(old['original_path']).parent.parent/'Model';rows=[]
 for p in sorted(copy.iterdir()):
  if p.suffix.casefold()!='.txt':continue
  assert p.is_file() and not p.is_symlink() and p.resolve().is_relative_to(R);src=original/p.name;assert src.is_file() and not src.is_symlink();body=p.read_bytes();assert body==src.read_bytes();digest=hashlib.sha256(body).hexdigest()
  rows.append(dict(relative_path='Model/'+p.name,copy_path=str(p),original_path=str(src),copy_size=len(body),original_size=len(body),copy_sha256=digest,original_sha256=digest,source_copy_equal=True))
 out=dict(rows=rows,all_source_copy_equal=True,readonly=True,original_code_executed=False)
 (R/'evidence/stage35-model-catalog.readonly.json').write_text(json.dumps(out,indent=2));print(json.dumps(dict(count=len(rows),bytes=sum(r['copy_size']for r in rows))))
if __name__=='__main__':main()
