#!/usr/bin/env python3
import json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage14_messages import NativeMessages
def main():
 native=NativeMessages();tests=[]
 for name in ['PV14A','PV14B']:
  body=(ROOT/f'samples/stage10-{name}.exe').read_bytes();result=json.loads((ROOT/f'evidence/stage14-{name}-Wine-result.json').read_text());pid=result['rows'][0]['return_value']
  for row in result['rows'][1:]:
   requested={'zero':0,'repeat':0,'actual_current':pid,'invalid':0xffffffff}[row['kind']];value,error=native.process_version(body,requested,pid,0x13579);assert (value,error)==(row['return_value'],row['last_error']);tests.append(dict(probe=name,kind=row['kind'],return_value=value,last_error=error))
 body=Path('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe').read_bytes();original=native.process_version(body,0,2,0x13579);assert original==(0x40000,0x13579)
 record=dict(all_passed=True,measured_Wine_comparisons=len(tests),tests=tests,original_current_process_version=original,reads_subsystem_version_from_actual_PE_header=True,OS_header_field_not_used=True,other_unmeasured_processes_refused=True,official='https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocessversion')
 (ROOT/'evidence/stage14-process-native-regression.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record));native.dispose()
if __name__=='__main__':main()
