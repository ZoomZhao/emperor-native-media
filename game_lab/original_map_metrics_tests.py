"""Authored corpus role/completeness/crop acceptance regressions; no original inputs."""
from pathlib import Path
import json,os,struct,subprocess,sys,tempfile,zlib

binary=Path(sys.argv[1]).resolve()
def framed(body):
 out=bytearray(struct.pack('<I',0xfedcbaaa))
 for start in range(0,len(body),32768):
  chunk=body[start:start+32768];encoded=zlib.compress(chunk)
  out+=struct.pack('<III',0xabcdefff,len(encoded),len(chunk))+encoded
 return bytes(out)
body=bytearray(261455+51984*4)
body[:8]=bytes.fromhex('0500feca00000200');struct.pack_into('<I',body,84,84)
for i in range(51984):struct.pack_into('<I',body,261455+4*i,0x80000)
struct.pack_into('<I',body,261455+4*(114*228+114),0)
city=framed(body);other=framed(bytes([2])+body[1:]);invalid_crop=bytearray(body);struct.pack_into('<I',invalid_crop,261455,0)
checks=0
with tempfile.TemporaryDirectory(prefix='emperor-map-metric-own-') as temp:
 root=Path(temp)
 cases=[('positive',{'Cities/test.map':city,'Campaigns/TEST/TESTP.map':other},True),
        ('swapped_roles',{'Cities/test.map':other,'Campaigns/TEST/TESTP.map':city},False),
        ('incomplete_empire',{'Cities/test.map':city,'Campaigns/TEST/TESTP.map':struct.pack('<I',0xfedcbaaa)},False),
        ('late_city_checksum',{'Cities/test.map':city+framed(b'own suffix')[4:-1]+bytes([framed(b'own suffix')[-1]^1]),'Campaigns/TEST/TESTP.map':other},False),
        ('invalid_crop',{'Cities/test.map':framed(invalid_crop),'Campaigns/TEST/TESTP.map':other},False),
        ('unclassified',{'test.map':city},False),('empty',{},False)]
 for name,files,expected in cases:
  data=root/name;data.mkdir()
  for path,value in files.items():
   target=data/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(value)
  env=os.environ.copy();env['EMPEROR_GAMEDATA_ROOT']=str(data)
  result=subprocess.run([str(binary)],env=env,capture_output=True,text=True)
  report=json.loads(result.stdout)
  assert result.returncode==(0 if expected else 1),(name,result.stdout,result.stderr)
  assert report['acceptance_passed']==expected and not report['skipped'],name
  if name=='swapped_roles':assert report['terrain_grids_loaded']==0 and report['city_directory_maps']==1 and report['role_mismatches']==2
  if name=='incomplete_empire':assert report['complete_containers']==1 and report['map_files']==2
  if name=='positive':assert report['terrain_grids_loaded']==1 and report['empire_roles_rejected']==1 and report['complete_containers']==2 and report['spatial_MAP_views']==1 and report['spatial_native_images']==1
  checks+=1
 env=os.environ.copy();env.pop('EMPEROR_GAMEDATA_ROOT',None)
 result=subprocess.run([str(binary)],env=env,capture_output=True,text=True);report=json.loads(result.stdout)
 assert result.returncode==0 and report['skipped'] and 'acceptance_passed' not in report
 checks+=1
print(json.dumps({'passed':True,'cases':checks,'self_authored':True,'original_data':False}))
