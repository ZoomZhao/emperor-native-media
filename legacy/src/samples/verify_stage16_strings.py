#!/usr/bin/env python3
import json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage16_paths import NativePaths
p=NativePaths();rows=[]
for row in json.loads((ROOT/'evidence/stage16-string-Wine-result.json').read_text())['rows']:
 status,n,body=p.copy(row['text'].encode(),row['count'],bytes([0xa5])*261)
 assert status==1 and row['returns_destination'] and row['last_error']==0x13579
 assert bytes([0xa5])*4+body+bytes([0xa5])*4==bytes.fromhex(row['buffer_hex'])
 rows.append(dict(text=row['text'],count=row['count'],bytes_written=n,output_guard_and_buffer_equal=True,return_is_destination=True,last_error_preserved=True))
assert p.copy(b'\x80',260,bytes(261))[0]==0
length_rows=[]
for row in json.loads((ROOT/'evidence/stage16-strlen-Wine-result.json').read_text())['rows']:
 assert p.length(row['text'].encode())==(1,row['return_value']) and row['last_error']==0x13579
 length_rows.append(row)
assert p.length(b'\x80')[0]==0
result=dict(all_passed=True,same_Wine_copy_comparisons=len(rows),same_Wine_length_comparisons=len(length_rows),rows=rows,length_rows=length_rows,non_ASCII_refused=True,scope='valid nonoverlapping ASCII buffers; no DBCS or exception semantics')
(ROOT/'evidence/stage16-strings-native-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));p.dispose()
