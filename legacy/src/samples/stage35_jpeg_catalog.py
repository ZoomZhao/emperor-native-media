"""Read/hash only: fixed legal JPEG inputs, no original DLL execution."""
import hashlib,json,struct
from pathlib import Path
R=Path(__file__).resolve().parents[1]
def sof(body):
 if body[:2]!=b'\xff\xd8':return None
 p=2
 while p+4<=len(body):
  if body[p]!=255:raise ValueError('invalid JPEG marker')
  while body[p]==255:p+=1
  marker=body[p];p+=1
  if marker in (0xd9,0xda):break
  n=struct.unpack_from('>H',body,p)[0]
  if n<2 or p+n>len(body):raise ValueError('truncated JPEG segment')
  if marker in (0xc0,0xc1,0xc2,0xc3,0xc5,0xc6,0xc7,0xc9,0xca,0xcb,0xcd,0xce,0xcf):
   bits,h,w,c=struct.unpack_from('>BHHB',body,p+2)
   return dict(marker=hex(marker),bits=bits,height=h,width=w,components=c)
  p+=n
 return None
def main():
 old=json.loads((R/'evidence/stage31-next-JPEG-file.json').read_text());copy=Path(old['copy_path']).parent;original=Path(old['original_path']).parent;rows=[]
 for p in sorted(copy.iterdir()):
  if p.suffix.casefold()!='.jpg':continue
  assert p.is_file() and not p.is_symlink() and p.resolve().is_relative_to(R)
  src=original/p.name;assert src.is_file() and not src.is_symlink();body=p.read_bytes();orig=src.read_bytes();assert body==orig;header=sof(body)
  rows.append(dict(guest_path='data\\'+p.name,canonical_guest_key=('data\\'+p.name).casefold(),copy_path=str(p),original_path=str(src),size=len(body),sha256=hashlib.sha256(body).hexdigest(),JPEG_SOF=header,source_copy_equal=True,supported_header=bool(header and header['marker']=='0xc0' and header['bits']==8 and header['components']==3 and 0<header['width']<=2048 and 0<header['height']<=2048)))
 assert len({row['canonical_guest_key']for row in rows})==len(rows)==26
 out=dict(rows=rows,readonly=True,original_DLL_executed=False,path_scope='fixed legal baseline data directory; case-insensitive exact relative JPEG names only',all_source_copy_equal=True)
 (R/'evidence/stage35-JPEG-catalog.readonly.json').write_text(json.dumps(out,indent=2));print(json.dumps(dict(count=len(rows),supported_header_count=sum(r['supported_header']for r in rows),dimensions=sorted({(r['JPEG_SOF']['width'],r['JPEG_SOF']['height'])for r in rows}))))
if __name__=='__main__':main()
