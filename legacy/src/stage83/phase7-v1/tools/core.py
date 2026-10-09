"""Independent phase7 development; no execution authority."""
import sys,json,hashlib,time
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[3]
V=ROOT/'stage83/phase7-v1'
sys.path.insert(0,str(ROOT/'stage83/repair-v2/tools'))
from common import sha,put,decode,verify,Boundary
from binary_host import prepare,HostStore
class Boundary(RuntimeError):
 def __init__(self,kind,**details):self.kind=kind;self.details=details;super().__init__(kind+(' '+str(details)if details else''))
SOURCE=ROOT/'runs/stage12-state-cycle61a'
PINS={'state.json':'0f916a79bc15697d88f0b986edaa6dd3ef454c3962fe5466b88eba7fede5e1ae','context.bin':'776576d5abb17c7906e7ab9593db43980671008366b6c58ac98de9c0bf800e54','memory.bin':'d4fc811b000eb8923a8d085602e363d5b0c6b2a87e272688f25365d18f088214'}
LIB='vendor-tools/unicorn-2.1.4/unicorn/lib/libunicorn.2.dylib'
LIB_SHA='7207c8e3d7a63118fb0bca73e01816797fd51b1d8a39a4cbc7abfd562ee59c85'
def fixed():
 if sha(ROOT/LIB)!=LIB_SHA:raise Boundary('FIXED_ENGINE_HASH')
 sys.path.insert(0,str(ROOT/'samples'))
 from stage73_cpu import load_fixed
 return load_fixed(ROOT,{'unicorn_package':'vendor-tools/unicorn-2.1.4','unicorn_library':LIB})
def source():
 for n,h in PINS.items():
  if sha(SOURCE/n)!=h:raise Boundary('FIXED_PHASE7_INPUT_HASH '+n)
 return json.loads((SOURCE/'state.json').read_text())
