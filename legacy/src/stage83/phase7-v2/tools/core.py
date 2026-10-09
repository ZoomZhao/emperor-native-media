"""Bounded phase7 closure development. Original execution has no entry point."""
import sys,json,hashlib,time,struct,copy
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[3];V=ROOT/'stage83/phase7-v2';OLD=ROOT/'stage83/phase7-v1'
sys.path.insert(0,str(ROOT/'stage83/repair-v2/tools'))
from common import sha,put,verify,decode
from binary_host import HostStore
SOURCE=ROOT/'runs/stage12-state-cycle61a'
PINS=json.loads((OLD/'configs/source.json').read_text())['original_source_pins']
class Boundary(RuntimeError):
 def __init__(self,kind,**details):self.kind=kind;self.details=details;super().__init__(kind+(' '+str(details)if details else''))
def host():return HostStore(OLD/'prepared-host/host.manifest.json')
def fixed():
 sys.path.insert(0,str(ROOT/'samples'))
 from stage73_cpu import load_fixed
 if sha(ROOT/'vendor-tools/unicorn-2.1.4/unicorn/lib/libunicorn.2.dylib')!='7207c8e3d7a63118fb0bca73e01816797fd51b1d8a39a4cbc7abfd562ee59c85':raise Boundary('ENGINE_PIN')
 return load_fixed(ROOT,{'unicorn_package':'vendor-tools/unicorn-2.1.4','unicorn_library':'vendor-tools/unicorn-2.1.4/unicorn/lib/libunicorn.2.dylib'})
def read_guest(a,n):
 s=json.loads((OLD/'configs/source.json').read_text())
 for r in s['regions']:
  if r['low']<=a and a+n<=r['high']+1:
   with(SOURCE/'memory.bin').open('rb')as f:f.seek(r['offset']+a-r['low']);return f.read(n)
 raise Boundary('STATIC_ADDRESS_UNMAPPED '+hex(a))
