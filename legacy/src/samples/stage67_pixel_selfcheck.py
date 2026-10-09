"""Detached platform pixels checked against saved self-authored fixture inputs."""
import ctypes,hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence'
class Surface(ctypes.Structure):
 _fields_=[(n,ctypes.c_int) for n in ['w','h','pitch','fmt','alive','locked','block']]+[('guest',ctypes.c_uint32),('pixels',ctypes.c_void_p)]
class Rect(ctypes.Structure):_fields_=[(n,ctypes.c_int)for n in ['x','y','w','h']]
def main():
 p=ROOT/'runs/stage67-native/libstage67_pixels.dylib';lib=ctypes.CDLL(str(p));ptr=ctypes.POINTER(Surface)
 create=lib.s23_create;create.argtypes=[ctypes.c_int]*4;create.restype=ptr
 free=lib.s23_free;free.argtypes=[ptr];free.restype=ctypes.c_int
 blt=lib.s23_blt;blt.argtypes=[ptr,ctypes.c_int,ctypes.c_int,ptr,Rect,ctypes.c_int,ctypes.c_uint32];blt.restype=ctypes.c_int
 fixture=ROOT/'runs/stage23-graphics/cases.bin';rows=[]
 with fixture.open('rb') as f:
  count=struct.unpack('<I',f.read(4))[0];assert count==8
  for index in range(count):
   a=struct.unpack('<12i',f.read(48));source=f.read(a[11]);n=a[6]*a[5];expected=f.read(n);after_prior_fixture_mutation=f.read(n)
   s=create(a[1],a[2],a[3],a[0]);d=create(a[4],a[5],a[6],32);assert s and d
   ctypes.memmove(s.contents.pixels,source,len(source));ctypes.memset(d.contents.pixels,0xa5,n)
   status=blt(d,a[7],a[8],s,Rect(0,0,a[1],a[2]),a[9],a[10]);actual=ctypes.string_at(d.contents.pixels,n)
   different=sum(x!=y for x,y in zip(actual,expected));rows.append(dict(case=index,format=a[0],status=status,
       saved_fixture_output_sha256=hashlib.sha256(expected).hexdigest(),own_output_sha256=hashlib.sha256(actual).hexdigest(),byte_mismatches=different))
   assert free(s)==free(d)==0
  assert not f.read(1)
 result=dict(status='detached_pixels_saved_fixture_equal' if all(q['status']==0 and q['byte_mismatches']==0 for q in rows) else 'detached_pixel_mismatch_STOP',
     rows=rows,library_sha256=hashlib.sha256(p.read_bytes()).hexdigest(),fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
     inputs_are_existing_self_authored_test_patterns_not_game_assets=True,original_reference_not_executed=True,no_EXE_no_Unicorn=True)
 (E/'stage67-pixel-selfcheck-v2.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)
 assert result['status']=='detached_pixels_saved_fixture_equal'
if __name__=='__main__':main()
