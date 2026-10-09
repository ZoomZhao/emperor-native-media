#!/usr/bin/env python3
import json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage13_events import NativeEvents
def main():
 n=NativeEvents();base=ROOT/'runs/stage13-directory-test';base.mkdir(exist_ok=True);(base/'Cities').mkdir(exist_ok=True);assert not (base/'ST13ProbeDir').exists();rows=[]
 for case in json.load(open(ROOT/'evidence/stage13-directory-Wine-result.json'))['rows']:
  p=base/Path(case['path'].replace('\\','/'));value,error=n.directory_create(p,0x13579);assert value==case['return_value'] and error==case['last_error'],(case,value,error);rows.append(dict(label=case['label'],return_equal=True,last_error_equal=True))
 result=dict(all_passed=True,compared_same_Wine_directory_cases=len(rows),rows=rows,all_paths_in_task_directory=True);(ROOT/'evidence/stage13-directory-native-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));n.dispose()
if __name__=='__main__':main()
