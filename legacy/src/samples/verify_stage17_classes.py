#!/usr/bin/env python3
import json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage17_file import NativeSavedFile
p=NativeSavedFile(ROOT/'stage9-data/ST9BASE.sav');rows=json.loads((ROOT/'evidence/stage17-class-Wine-result.json').read_text())['rows'];nodes=json.loads((ROOT/'evidence/stage17-class-cases.json').read_text())['registry_nodes']
for row in rows:
 status,equal=p.identifier_equal(row['first'].encode(),row['second'].encode());assert status==1 and equal==(row['return_value']==0) and row['last_error']==0x13579
for a,b in [(b'a-b',b'ab'),(b'',b'a'),(b'\x80',b'a'),(b'A'*64,b'A'*64)]:assert p.identifier_equal(a,b)[0]==0
result=dict(all_passed=True,same_Wine_zero_result_comparisons=len(rows),actual_registry_nodes=len(nodes),case_sensitive_verified=True,unsupported_punctuation_nonASCII_empty_and_overlong_refused=True,consumer_scope='exact original MFC runtime-class lookup return78036c; only TEST EAX then JE; no locale ordering implementation',nonzero_result_is_consumer_projection=True)
(ROOT/'evidence/stage17-class-native-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));p.dispose()
