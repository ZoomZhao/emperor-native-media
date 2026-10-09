#!/usr/bin/env python3
"""Freeze one existing save and obtain original-only read-only provenance clues."""
import hashlib,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'samples'))
from auto_i386_analysis import PE
EN=Path('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe')
SAVE=Path('<LOCAL_ORIGINAL_PROJECT>/GameData/Save/zoom/历史战役自动保存_1.sav')
EXPECTED='e002894ce7dfa91d374715356708433afa70129bff673ce667f47b3520a12845'
pe=PE(str(EN)); original_hash=hashlib.sha256(pe.data).hexdigest()
assert original_hash=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
save=SAVE.read_bytes();assert hashlib.sha256(save).hexdigest()==EXPECTED
dest=ROOT/'stage7-data/save-input.sav';dest.parent.mkdir(exist_ok=True)
if dest.exists():assert dest.read_bytes()==save
else:dest.write_bytes(save);dest.chmod(0o444)
rsrc=next(s for s in pe.sections if s['name']=='.rsrc')
rb=pe.read(rsrc['va'],rsrc['rawsize']); versions=[];start=0
while True:
    at=rb.find(struct.pack('<I',0xfeef04bd),start)
    if at<0:break
    if at+52<=len(rb):
        v=struct.unpack_from('<13I',rb,at)
        versions.append(dict(va=hex(rsrc['va']+at),file_version=[v[2]>>16,v[2]&65535,v[3]>>16,v[3]&65535],product_version=[v[4]>>16,v[4]&65535,v[5]>>16,v[5]&65535],raw_fixed_info=list(v)))
    start=at+4
strings=[]
for s in pe.sections:
    if s['flags']&0x20000000:continue
    b=pe.read(s['va'],s['rawsize']);start=0
    while True:
        at=b.lower().find(b'.sav',start)
        if at<0:break
        lo=b.rfind(b'\0',max(0,at-96),at)+1;hi=b.find(b'\0',at)
        if hi<0:hi=min(len(b),at+96)
        strings.append(dict(va=hex(s['va']+lo),match_va=hex(s['va']+at),value=b[lo:hi].decode('latin1')))
        start=at+4
text=next(s for s in pe.sections if s['name']=='.text'); code=pe.read(text['va'],text['rawsize'])
for item in strings:
    target=int(item['va'],16);pattern=struct.pack('<I',target);at=0;refs=[]
    while True:
        at=code.find(pattern,at)
        if at<0:break
        refs.append(hex(text['va']+at));at+=1
    item['raw_operand_candidates']=refs
vtable=[hex(x) for x in struct.unpack('<28I',pe.read(0x7ac7d4,112))]
result=dict(source_exe=str(EN),exe_sha256=original_hash,version_resource_candidates=versions,
            selected_save=str(SAVE),save_sha256=EXPECTED,size=len(save),frozen_copy=str(dest),
            header_u32=[hex(x) for x in struct.unpack('<16I',save[:64])],
            original_producer_and_exe_compatibility='unknown until original loader validation',
            save_string_candidates=strings,archive_vtable_candidate=dict(va='0x7ac7d4',words=vtable,extent_unknown=True))
(ROOT/'evidence/stage7-input-inventory.json').write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n')
print(json.dumps(result,ensure_ascii=False))
