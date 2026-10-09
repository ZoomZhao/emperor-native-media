"""Owned read-only POSIX directory handles for the observed Win32 name consumer.
Full NTFS metadata/short-name/DOS wildcard equivalence is not asserted.
"""
import os,struct,hashlib,json
from pathlib import Path
from stage17_file import NativeSavedFile
R=Path(__file__).resolve().parents[1]
GAME=R/'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]'
def profile_rows():return json.loads((R/'evidence/stage46-profile-files.json').read_text())['rows']
def verify(row):
 p=Path(row['copy_path']);assert hashlib.sha256(p.read_bytes()).hexdigest()==row['copy_sha256']and p.stat().st_size==row['copy_size'];op=Path(row['original_path']);assert hashlib.sha256(op.read_bytes()).hexdigest()==row['original_sha256'];return p
def enumerate_profiles():
 directory=GAME/'Save';fd=os.open(directory,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW);records=[]
 try:
  with os.scandir(fd)as scan:
   for e in scan:
    if not e.name.lower().endswith('.dat'):continue
    row=next((r for r in profile_rows()if Path(r['copy_path']).name==e.name),None);assert row and e.is_file(follow_symlinks=False);p=verify(row);native=NativeSavedFile(p)
    try:size,birth,access,write,device,inode=native.info();attrs=native.attributes()
    finally:native.dispose()
    name=e.name.encode('gbk');assert len(name)<260 and name.decode('gbk')==e.name
    body=struct.pack('<I6I4I',attrs,birth&0xffffffff,birth>>32,access&0xffffffff,access>>32,write&0xffffffff,write>>32,size>>32,size&0xffffffff,0,0)+name+b'\0';body=body.ljust(320,b'\0');assert len(body)==320
    records.append(dict(name=e.name,name_ACP936=name,WIN32_FIND_DATAA=body,sha256=row['copy_sha256'],device=device,inode=inode))
  assert len(records)<=64 and len(records)==len(profile_rows());return fd,records
 except:os.close(fd);raise
