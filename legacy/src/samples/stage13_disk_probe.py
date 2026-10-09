#!/usr/bin/env python3
import argparse,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run
NAME='DISK13'
def make():
 d=Data();d.alloc(b'DSK13v01');ops=[];cases=[]
 for label,path in [('current',None),('drive','C:\\')]:
  name=d.alloc(path.encode()+b'\0') if path else 0;buf=d.alloc(bytes([0xa5])*24);out=d.alloc(bytes(8));ops.append(dict(name='GetDiskFreeSpaceA',args=[name,buf+4,buf+8,buf+12,buf+16],result=out));cases.append(dict(label=label,path=path,result_offset=d.offset(out),buffer_offset=d.offset(buf)))
 meta=build(NAME,d,ops);(ROOT/'evidence/stage13-disk-cases.json').write_text(json.dumps(cases,indent=2)+'\n');print(meta['sha256'])
def capture():
 result=run(NAME);raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();rows=[]
 for c in json.load(open(ROOT/'evidence/stage13-disk-cases.json')):
  p=c['buffer_offset'];value,error=struct.unpack_from('<II',raw,c['result_offset']);fields=struct.unpack_from('<4I',raw,p+4);assert raw[p:p+4]==raw[p+20:p+24]==bytes([0xa5])*4;rows.append(dict(c,return_value=value,last_error=error,sectors_per_cluster=fields[0],bytes_per_sector=fields[1],free_clusters=fields[2],total_clusters=fields[3],canaries_unchanged=True))
 result['rows']=rows;(ROOT/'evidence/stage13-disk-Wine-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(rows))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args();make() if a.action=='build' else capture()
