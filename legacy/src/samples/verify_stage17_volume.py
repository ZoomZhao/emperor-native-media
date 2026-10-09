#!/usr/bin/env python3
import json,os,struct,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage17_volume import NativeVolume
from stage16_paths import GAME
p=NativeVolume();records=[]
for root in [b'C:\\',b'c:\\',None]:
 status,body=p.volume(root);assert status==1;maximum,flags,raw,valid,interfaces,valid_interfaces=struct.unpack('<6I',body);assert flags==6 and maximum==255 and raw&valid&0x200
 records.append(dict(root=root.decode() if root else None,maximum_ASCII_component=maximum,consumer_flags=hex(flags),native_format_caps=hex(raw),valid_format_caps=hex(valid),interfaces=hex(interfaces),valid_interfaces=hex(valid_interfaces)))
refusals=[]
for root,mask,size in [(b'D:\\',6,24),(b'C:\\Missing\\',6,24),(b'C:\\',8,24),(b'C:\\',0xffffffff,24),(b'C:\\',6,23),(b'C:\\',6,25)]:
 status,body=p.volume(root,mask,size);assert status==0 and body==bytes([0xa5])*size;refusals.append(dict(root=root.decode(),consumer_mask=hex(mask),output_bytes=size,untouched=True))
with tempfile.TemporaryDirectory(prefix='stage17-fs-',dir=ROOT/'runs') as d:
 at=Path(d);assert at.stat().st_dev==GAME.stat().st_dev
 name='ST17-é-皇帝.TXT';file=at/name;file.write_bytes(b'self-authored probe')
 assert [f.name for f in at.iterdir()]==[name];assert (at/name.lower()).read_bytes()==file.read_bytes()
 assert (at/('A'*255)).touch() is None
 try:(at/('A'*256)).touch()
 except OSError as e:assert e.errno==63
 else:raise AssertionError('ASCII NAME_MAX not enforced')
 unicode_roundtrip=dict(same_backing_device=True,exact_UTF8_name_preserved=True,case_preserved=True,case_insensitive_lookup=True,ASCII_255_accepted=True,ASCII_256_rejected=True,probe_removed=True)
blob=p.export();q=NativeVolume();q.restore(blob);assert q.volume(b'C:\\')==p.volume(b'C:\\');q.dispose();p.dispose()
result=dict(all_passed=True,records=records,refusals=refusals,roundtrip=unicode_roundtrip,snapshot_and_directory_identity_equal=True,Wine_reference_flags='0xA',production_flags='0x6 from APFS case-preservation and Unicode disk names',defined_flag_mask='0x6',other_Windows_flags_serial_and_name_unsupported=True,ASCII_component_length_scope=True,success_last_error_policy='preserve incoming; official only defines error on failure',not_full_GetVolumeInformationA=True)
(ROOT/'evidence/stage17-volume-native-regression.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n');print(json.dumps(result,ensure_ascii=False))
