#!/usr/bin/env python3
"""Read actual same-prefix Win32 OS version; no invented success or registry edits."""
import argparse,json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from stage10_probe_support import ROOT,Data,build,run
NAME='VERSION'

def make():
    data=Data();data.alloc(b'VERS0010');plain=data.alloc(bytes(8));version=data.alloc(struct.pack('<I',148)+bytes([0xa5])*144);extended=data.alloc(bytes(8))
    ops=[dict(name='GetVersion',args=[],result=plain),dict(name='GetVersionExA',args=[version],result=extended)]
    meta=build(NAME,data,ops);return dict(sha256=meta['sha256'],size=meta['size'])

def main():
    p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args()
    if a.action=='build':print(json.dumps(make(),indent=2));return
    result=run(NAME)
    if 'output_sha256' in result:
        raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();version=raw[16:164]
        result.update(GetVersion=hex(struct.unpack_from('<I',raw,8)[0]),GetVersionExA_success=struct.unpack_from('<I',raw,164)[0],OSVERSIONINFOA_hex=version.hex(),OSVERSIONINFOA_fields=struct.unpack_from('<5I',version),service_pack=version[20:].split(b'\0',1)[0].decode('ascii'))
    (ROOT/f'evidence/stage10-{NAME}-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':main()
