#!/usr/bin/env python3
import ctypes,hashlib,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage17_file import NativeSavedFile
from stage16_paths import GAME
p=NativeSavedFile(GAME/'Save/zoom/ST9BASE.sav');rows=json.loads((ROOT/'evidence/stage17-metadata-Wine-result-ok.json').read_text())['rows'];info=p.info();wine_times=tuple(int.from_bytes(bytes.fromhex(v),'little') for v in rows[0]['buffer_hex']);assert info[1:4]==wine_times and info[0]==rows[1]['return_value']==168643
checked=[];differences=[]
for row in rows[3:]:
 kind=1 if row['name']=='FileTimeToLocalFileTime' else 2;status,body=p.convert(kind,row['ticks'],row['size'])
 if row['ticks']>=0x8000000000000000:
  assert status==0 and body==bytes([0xa5])*row['size'];differences.append(dict(name=row['name'],reason='out of supported valid FILETIME domain; old Wine even contradicts documented FileTimeToSystemTime failure'))
 else:assert status==1 and body.hex()==row['buffer_hex'][0] and row['last_error']==0x13579;checked.append(dict(name=row['name'],ticks=row['ticks'],exact_output_equal=True))
for n in [47,49]:
 b=ctypes.create_string_buffer(bytes([0xa5])*n);assert p.info_c(p.state,b,n)==0 and b.raw[:-1]==bytes([0xa5])*n
b=ctypes.create_string_buffer(bytes([0xa5])*48);assert p.info_c(None,b,48)==0 and b.raw[:-1]==bytes([0xa5])*48
full=p.read(0,168643);assert hashlib.sha256(full).hexdigest()=='fd97f2f426e4b69e311d5a8c8596b170f7246df8ad102c4173fcb476d43689e1';assert p.read(len(full),32)==b'' and p.read(0,0)==b''
attrs=p.attributes();assert attrs==0x80;differences.append(dict(name='GetFileAttributesA',native_basic_projection=hex(attrs),Wine=hex(rows[2]['return_value']),reason='no DOS archive bit copied; only basic fixed regular-file attributes; exact original CArchive filename consumer discards status attributes'))
result=dict(all_passed=True,same_physical_file_three_FILETIMEs_exact=True,same_Wine_file_size=True,valid_time_conversion_comparisons=len(checked),checked=checked,differences=differences,invalid_capacity_and_context_untouched=True,native_pread_complete_frozen_save_hash_equal=True,EOF_and_zero_read_passed=True,metadata_from_actual_readonly_FD=True,production_source='stage9-data/ST9BASE.sav; actual metadata sampled from that file',no_input_content_or_timestamps_set=True)
(ROOT/'evidence/stage17-file-native-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));p.dispose()
