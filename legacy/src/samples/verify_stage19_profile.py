#!/usr/bin/env python3
import json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage19_profile import missing_profile,version_file_missing
rows=json.loads((ROOT/'evidence/stage19-profile-Wine.json').read_text())['results'];passed=[]
for row,default in zip(rows,[b'ServerPlayer',b'ServerPlayer',None,b'abc   ']):
 n,e,b,full=missing_profile(default,row['size']);assert n==row['return_value'] and e==row['last_error'] and full[:min(32,row['size'])].hex()==row['output'];assert full[n+1:]==bytes([0xa5])*(511-n);passed.append(dict(size=row['size'],output_and_return_and_error_equal=True,untouched_tail=True))
version_file_missing();out=dict(all_passed=True,same_Wine_rows=passed,actual_native_readonly_missing_version_file=True,no_original_game_logic_executed=True,native_helper_sha256=json.loads((ROOT/'evidence/stage19-profile-build.json').read_text())['sha256']);(ROOT/'evidence/stage19-profile-native-regression.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out))
