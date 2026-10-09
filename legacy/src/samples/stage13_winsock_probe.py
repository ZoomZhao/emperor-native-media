#!/usr/bin/env python3
"""Own same-Wine startup/cleanup measurement. No socket/DNS/network requests."""
import argparse,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run
NAME='WSA13'
def make():
    d=Data();d.alloc(b'WSA13v01');ops=[];cases=[]
    def op(name,args,label):
        out=d.alloc(bytes(8));ops.append(dict(name=name,args=args,result=out));cases.append(dict(name=name,args=args,label=label,offset=d.offset(out)))
    op('WSACleanup',[],'cleanup-before-start')
    for version,fill in [(0x101,0xa5),(0x101,0x5a),(0,0xa5),(0x202,0xa5)]:
        buf=d.alloc(bytes([0xcd])*4+bytes([fill])*400+bytes([0xcd])*4)
        op('WSAStartup',[version,buf+4],f'start-{version:x}-{fill:x}');cases[-1].update(buffer_offset=d.offset(buf),fill=fill)
        if version:op('WSACleanup',[],f'cleanup-{version:x}-{fill:x}')
    a=d.alloc(bytes([0xa5])*400);b=d.alloc(bytes([0xa5])*400)
    op('WSAStartup',[0x101,a],'nested-first');op('WSAStartup',[0x101,b],'nested-second');op('WSACleanup',[],'nested-cleanup-first');op('WSACleanup',[],'nested-cleanup-second');op('WSACleanup',[],'nested-cleanup-exhausted')
    meta=build(NAME,d,ops,dll_for={'WSAStartup':'WSOCK32.dll','WSACleanup':'WSOCK32.dll'});(ROOT/'evidence/stage13-wsa-cases.json').write_text(json.dumps(cases,indent=2)+'\n');print(json.dumps(dict(sha256=meta['sha256'],case_count=len(cases))))
def capture():
    result=run(NAME);raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();rows=[]
    for case in json.loads((ROOT/'evidence/stage13-wsa-cases.json').read_text()):
        value,error=struct.unpack_from('<II',raw,case['offset']);row=dict(case,return_value=value,last_error=error)
        if 'buffer_offset' in case:
            p=case['buffer_offset'];body=raw[p+4:p+404];row.update(buffer_hex=body.hex(),canaries_unchanged=raw[p:p+4]==raw[p+404:p+408]==bytes([0xcd])*4,wVersion=struct.unpack_from('<H',body)[0],wHighVersion=struct.unpack_from('<H',body,2)[0],description=body[4:261].split(b'\0')[0].decode('latin1'),system_status=body[261:390].split(b'\0')[0].decode('latin1'));assert row['canaries_unchanged']
        rows.append(row)
    result.update(rows=rows,network_operations_performed=False,only_initialization_and_cleanup=True);(ROOT/'evidence/stage13-wsa-Wine-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='rows'}))
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args();make() if a.action=='build' else capture()
