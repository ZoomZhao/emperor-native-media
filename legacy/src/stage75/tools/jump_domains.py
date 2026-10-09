"""Reviewed fixed-byte CRT dispatch metadata; no instruction interpretation.

Sixteen relative dispatch descriptions are reused for two byte-backed copies.
Bounds below are human-reviewed source contracts, explicitly not automated
abstract-interpretation proofs or a native implementation of either routine.
"""
import hashlib,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'samples'))
from auto_i386_analysis import PE
def main():
 pe=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe');assert hashlib.sha256(pe.data).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
 # offset, index register, table offset, signed index minimum/maximum, proof.
 specs=[
 (0x35,'edx',0x148,0,3,'AND edx,3 at+0x2b; REP MOVSD preserves edx'),
 (0x4d,'eax',0x60,1,3,'unaligned branch TEST edi,3/JNE at+0x20; eax=edi; AND eax,3 at+0x48'),
 (0x54,'ecx',0x158,-4,-1,'SUB ecx,4 at+0x43; JB at+0x46 means original unsigned count0..3; table address wraps modulo2^32'),
 (0x5c,'ecx',0xdc,0,7,'all incoming paths CMP ecx,8/JB at+0x2e,+0x8b,+0xb1,+0xcb'),
 (0x92,'edx',0x148,0,3,'edx=3 at+0x3e; AND edx,ecx at+0x70'),
 (0xb8,'edx',0x148,0,3,'edx=3 at+0x3e; AND edx,ecx at+0x9c'),
 (0xd2,'edx',0x148,0,3,'edx=3 at+0x3e; AND edx,ecx at+0xc0'),
 (0x13f,'edx',0x148,0,3,'edx from AND edx,3 or edx=3 AND ecx on every incoming forward-copy path'),
 (0x1b7,'edx',0x2e0,0,3,'AND edx,3 at+0x1ab; REP MOVSD preserves edx'),
 (0x1c2,'ecx',0x290,-7,0,'all incoming CMP ecx,8/JB; NEG ecx at+0x1c0 gives signed-7..0 and wrapped addressing'),
 (0x1dd,'eax',0x1e8,1,3,'unaligned TEST edi,3/JNE at+0x1a0; eax=edi; AND eax,3 at+0x1d8'),
 (0x1e4,'ecx',0x2e0,0,3,'CMP ecx,4/JB at+0x1d3; unsigned count0..3'),
 (0x20e,'edx',0x2e0,0,3,'edx=3 at+0x1ce; AND edx,ecx at+0x1fb'),
 (0x238,'edx',0x2e0,0,3,'edx=3 at+0x1ce; AND edx,ecx at+0x21b'),
 (0x26a,'edx',0x2e0,0,3,'edx=3 at+0x1ce; AND edx,ecx at+0x243'),
 (0x2d7,'edx',0x2e0,0,3,'edx from AND edx,3 or edx=3 AND ecx on every incoming reverse-copy path')]
 domains={};regbyte={'eax':0x85,'ecx':0x8d,'edx':0x95};bases=[0x765200,0x76ab30]
 for base in bases:
  for offset,reg,table,lo,hi,proof in specs:
   pc=base+offset;raw=pe.read(pc,7);assert raw[:3]==bytes([0xff,0x24,regbyte[reg]]),(hex(pc),raw.hex())
   address=struct.unpack('<I',raw[3:])[0];assert address==base+table
   rows=[]
   for i in range(lo,hi+1):
    slot=(address+4*i)&0xffffffff;target=struct.unpack('<I',pe.read(slot,4))[0];s=pe.section(target);assert s and s['flags']&0x20000000,(hex(pc),i,hex(target))
    rows.append(dict(signed_index=i,wrapped_index=hex(i&0xffffffff),slot=hex(slot),target=hex(target)))
   domains[hex(pc)]=dict(table=hex(address),index_register=reg,signed_index_min=lo,signed_index_max=hi,original_instruction_bytes=raw.hex(),rows=rows,reviewed_proof=proof,proof_scope='reviewed assembly dataflow, not machine checked',routine_base=hex(base))
 out=ROOT/'stage75/configs/crt-jump-domains.json';assert not out.exists();out.write_text(json.dumps(dict(domains=domains,manually_reviewed_relative_descriptions=len(specs),reused_copies=len(bases),generated_sites=len(domains),original_game_executed=False),indent=2)+'\n');print(json.dumps(dict(sites=len(domains),slots=sum(len(r['rows'])for r in domains.values()),manual_relative_descriptions=16,reused_copies=2)))
if __name__=='__main__':main()
