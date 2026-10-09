#!/usr/bin/env python3
import json,os,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage16_paths import NativePaths
def main():
 baseline=json.loads((ROOT/'evidence/stage16-cwd-Wine-result.json').read_text());row=baseline['rows'][0];guest=bytes.fromhex(row['buffer_hex'])[4:].split(b'\0')[0];native=NativePaths(guest);before=os.getcwd();records=[];outside=[]
 for row in baseline['rows']:
  if row['name']=='SetCurrentDirectoryA':
   status,value,error=native.set(row['path'].encode(),0x13579)
   if row['label']=='missing':assert status==0;outside.append(row['label']);continue
  else:
   expected=bytes.fromhex(row['buffer_hex']);status,value,error,body=native.get(row['size'],bytes([0xa5])*261,0x13579);assert bytes([0xa5])*4+body+bytes([0xa5])*4==expected
  assert status==1 and (value,error)==(row['return_value'],row['last_error']);records.append(dict(label=row['label'],return_value=value,last_error=error))
 blob=native.export();restored=NativePaths(guest);restored.restore(blob);assert restored.set(b'',0x124)==native.set(b'',0x124)==(1,0,123);assert native.export()==restored.export();assert os.getcwd()==before
 result=dict(all_passed=True,same_Wine_comparisons=len(records),records=records,outside_finite_scope=outside,snapshot_equal_after_failed_set_and_resume=True,host_process_cwd_unchanged=True,backing_directory_actual_POSIX_fd=True,test_namespace=guest.decode(),production_namespace='C:\\Emperor existing harness alias; not written to game path globals')
 (ROOT/'evidence/stage16-paths-native-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));native.dispose();restored.dispose()
if __name__=='__main__':main()
