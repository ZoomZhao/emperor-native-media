#!/usr/bin/env python3
import json,os,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage13_events import NativeEvents
def main():
 n=NativeEvents();path=ROOT/'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]';before=os.statvfs(path);fields,error=n.disk_space(path,0x13579);after=os.statvfs(path);oracle=json.load(open(ROOT/'evidence/stage13-disk-Wine-result.json'))['rows'][0]
 assert fields[0]==oracle['sectors_per_cluster']==8 and fields[1]==oracle['bytes_per_sector']==512 and fields[3]==oracle['total_clusters']==120699413
 assert min(before.f_bavail,after.f_bavail)<=fields[2]<=max(before.f_bavail,after.f_bavail) and error==oracle['last_error']==0x13579
 result=dict(all_passed=True,native_statvfs_fields=fields,geometry_and_total_match_same_Wine=True,free_clusters_equal_live_native_volume_interval=True,last_error_equal=True,frozen_Wine_free_clusters_not_replayed=True)
 (ROOT/'evidence/stage13-disk-native-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));n.dispose()
if __name__=='__main__':main()
