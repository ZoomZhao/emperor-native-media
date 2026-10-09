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
 p=Path(row['copy_path']);assert hashlib.sha256(p.read_bytes()).hexdigest()==row['sha256']and p.stat().st_size==row['copy_size'];op=Path(row['original_path']);assert hashlib.sha256(op.read_bytes()).hexdigest()==row['original_sha256'];return p
def enumerate_saves():
 directory=GAME/'Save/zoom';fd=os.open(directory,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW);records=[]
 try:
  with os.scandir(fd)as scan:
   for e in scan:
    if not e.name.lower().endswith('.sav'):continue
    row=next((r for r in json.loads((R/'evidence/stage47-existing-save-paths.json').read_text())if Path(r['path'])==directory.resolve()/e.name),None);assert row and e.is_file(follow_symlinks=False);p=Path(row['path']);assert p.stat().st_size==row['size']and hashlib.sha256(p.read_bytes()).hexdigest()==row['sha256'];native=NativeSavedFile(p)
    try:size,birth,access,write,device,inode=native.info();attrs=native.attributes()
    finally:native.dispose()
    name=e.name.encode('gbk');assert len(name)<260 and name.decode('gbk')==e.name
    body=struct.pack('<I6I4I',attrs,birth&0xffffffff,birth>>32,access&0xffffffff,access>>32,write&0xffffffff,write>>32,size>>32,size&0xffffffff,0,0)+name+b'\0';body=body.ljust(320,b'\0');assert len(body)==320
    records.append(dict(name=e.name,name_ACP936=name,WIN32_FIND_DATAA=body,sha256=row['sha256'],device=device,inode=inode))
  assert len(records)<=64 and len(records)==len([q for q in json.loads((R/'evidence/stage47-existing-save-paths.json').read_text())if Path(q['path']).parent==directory.resolve()and q['path'].lower().endswith('.sav')]);return fd,records
 except:os.close(fd);raise

def campaign_metadata(row):
 import ctypes
 p=Path(row['path']);st=p.stat();assert st.st_dev==row['device']and st.st_ino==row['inode']and st.st_size==row['size']
 if row['is_directory']:
  m=json.loads((R/'evidence/stage47-directory-build.json').read_text());lib=Path(m['library']);assert hashlib.sha256(lib.read_bytes()).hexdigest()==m['sha256'];native=ctypes.CDLL(str(lib));f=native.s47_directory_info;f.argtypes=[ctypes.c_char_p,ctypes.POINTER(ctypes.c_uint64),ctypes.POINTER(ctypes.c_uint32)];f.restype=ctypes.c_int;v=(ctypes.c_uint64*6)();a=ctypes.c_uint32();assert f(os.fsencode(p),v,ctypes.byref(a));return list(v),a.value
 assert hashlib.sha256(p.read_bytes()).hexdigest()==row['sha256'];native=NativeSavedFile(p)
 try:return native.info(),native.attributes()
 finally:native.dispose()
def enumerate_campaigns():
 directory=GAME/'Campaigns';fd=os.open(directory,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW);records=[];rows=json.loads((R/'evidence/stage47-campaign-files.json').read_text())
 try:
  with os.scandir(fd)as scan:
   for e in scan:
    row=next((r for r in rows if r['name']==e.name),None);assert row and not e.is_symlink();v,attrs=campaign_metadata(row);size,birth,access,write,device,inode=v;name=e.name.encode('gbk');assert name.decode('gbk')==e.name and len(name)<260;body=struct.pack('<I6I4I',attrs,birth&0xffffffff,birth>>32,access&0xffffffff,access>>32,write&0xffffffff,write>>32,size>>32,size&0xffffffff,0,0)+name+b'\0';records.append(dict(name=e.name,name_ACP936=name,WIN32_FIND_DATAA=body.ljust(320,b'\0'),sha256=row.get('sha256'),device=device,inode=inode))
  assert len(records)==len(rows);return fd,records
 except:os.close(fd);raise
