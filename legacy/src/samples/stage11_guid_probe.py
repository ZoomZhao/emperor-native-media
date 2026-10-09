#!/usr/bin/env python3
"""Own CoCreateGuid/StringFromGUID2 repeated/canary/NULL capture; no game code."""
import argparse,json,struct,sys,uuid
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run
NAME='GUID11'

def make():
    data=Data();data.alloc(b'GUID11v1');ops=[];cases=[]
    for i in range(32):
        buf=data.alloc(bytes([0xa5])*24);result=data.alloc(bytes(8));text=data.alloc(bytes([0xa5])*84);text_result=data.alloc(bytes(8))
        ops.extend([dict(name='CoCreateGuid',args=[buf+4],result=result),dict(name='StringFromGUID2',args=[buf+4,text+4,39],result=text_result)])
        cases.append(dict(buffer_offset=data.offset(buf),result_offset=data.offset(result),text_offset=data.offset(text),text_result_offset=data.offset(text_result)))
    null=data.alloc(bytes(8));ops.append(dict(name='CoCreateGuid',args=[0],result=null))
    meta=build(NAME,data,ops,dll_for={'CoCreateGuid':'ole32.dll','StringFromGUID2':'ole32.dll'})
    (ROOT/'evidence/stage11-guid-cases.json').write_text(json.dumps(dict(cases=cases,null_result_offset=data.offset(null)),indent=2)+'\n')
    print(json.dumps(dict(sha256=meta['sha256'],size=meta['size'])))

def capture():
    result=run(NAME);layout=json.loads((ROOT/'evidence/stage11-guid-cases.json').read_text())
    if 'output_sha256' in result:
        raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();rows=[]
        for c in layout['cases']:
            at=c['buffer_offset'];body=raw[at:at+24];guid=body[4:20];ret,error=struct.unpack_from('<II',raw,c['result_offset'])
            t=c['text_offset'];string=raw[t+4:t+82].decode('utf-16le');text_ret,text_err=struct.unpack_from('<II',raw,c['text_result_offset'])
            parsed=uuid.UUID(bytes_le=guid);expected='{'+str(parsed).upper()+'}\0'
            assert body[:4]==body[20:]==bytes([0xa5])*4 and string==expected
            assert raw[t:t+4]==bytes([0xa5])*4 and raw[t+82:t+84]==bytes([0xa5])*2
            assert ret==0 and error==0x13579 and text_ret==39 and text_err==0x13579
            assert parsed.version==4 and parsed.variant==uuid.RFC_4122
            rows.append(dict(return_value=ret,last_error=error,guid_bytes_le_hex=guid.hex(),guid_string=string[:-1],version=parsed.version,variant=parsed.variant,canaries_unchanged=True))
        assert len({x['guid_bytes_le_hex'] for x in rows})==32
        ret,error=struct.unpack_from('<II',raw,layout['null_result_offset']);assert ret==0x80070057 and error==0x13579
        result.update(all_passed=True,rows=rows,null_pointer=dict(return_value=hex(ret),last_error=error),sample_unique_count=32,note='Finite collision observation is not mathematical proof of uniqueness; compare structure and branches, not random cross-run bytes')
    (ROOT/'evidence/stage11-guid-Wine-result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='rows'}))
    assert result.get('all_passed')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args();make() if a.action=='build' else capture()
