import os,errno
from pathlib import Path
from stage46_find import GAME
SOURCE=Path('<LOCAL_MACHINE_PATH>]')
def missing_name(path):
 relative=path
 if relative.lower().startswith(b'c:\\emperor\\'):relative=relative[len(b'c:\\emperor\\'):]
 return relative.split(b'\\')[-1] if relative.lower()in [b'campaigns\\audio.pak',b'campaigns\\untitled.pak']else None
def actual_absence(name):
 assert name.lower()in [b'audio.pak',b'untitled.pak'];observations=[]
 for game in [GAME,SOURCE]:
  directory=game/'Campaigns';fd=os.open(directory,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
  try:
   st=os.fstat(fd);matches=[q.name for q in os.scandir(fd)if q.name.casefold()==name.decode('ascii').casefold()];assert not matches
   try:opened=os.open(name.decode('ascii'),os.O_RDONLY|os.O_NOFOLLOW,dir_fd=fd)
   except FileNotFoundError as error:
    assert error.errno==errno.ENOENT;observations.append(dict(path=str(directory/name.decode('ascii')),native_errno=error.errno,casefold_matches=matches,parent_device=st.st_dev,parent_inode=st.st_ino,actual_openat_readonly=True))
   else:os.close(opened);raise AssertionError('missing campaign file now exists; fail closed')
  finally:os.close(fd)
 return observations
