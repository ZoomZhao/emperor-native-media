import copy,hashlib,json,sys,unittest
from pathlib import Path
sys.dont_write_bytecode=True
from thread_contract import ThreadRuntime,Boundary,checked_CPU,restore_metadata
ROOT=Path(__file__).resolve().parents[2]
def public():return json.loads((ROOT/'runs/stage73-reference-once/result.json').read_text())['initial_CPU']
def model():
 a=public();b=public();b['EAX']=0x1234;b['EIP']=0x401000
 records={1:dict(status='runnable',public_CPU=a,opaque_bytes=bytes([1])*5608,tls={0:0xffffffff},last_error=2,cached_segments_verified=True),2:dict(status='runnable',public_CPU=b,opaque_bytes=bytes([2])*5608,tls={0:0x80000000},last_error=8,cached_segments_verified=True)}
 return ThreadRuntime(records,1,a,bytes([1])*5608,{0:0xffffffff},2)
class Tests(unittest.TestCase):
 def test_public_FP80_exact(self):
  a=public();v=checked_CPU(a,True);self.assertEqual(v,a);a['FP80'][0]=[0xffffffffffffffff,0xffff];self.assertEqual(checked_CPU(a,True)['FP80'][0],a['FP80'][0])
 def test_switch_save_restore_all_fields(self):
  r=model();first=copy.deepcopy(r.current_CPU);r.kernel('TlsSetValue',0,7);r.kernel('SetLastError',9);r.switch(2);self.assertEqual(r.kernel('GetCurrentThreadId'),2);self.assertEqual(r.kernel('GetLastError'),8);self.assertEqual(r.kernel('TlsGetValue',0),0x80000000);self.assertEqual(r.last_error,0);self.assertEqual(r.opaque,bytes([2])*5608);r.switch(1);self.assertEqual(r.current_CPU,first);self.assertEqual(r.opaque,bytes([1])*5608);self.assertEqual(r.kernel('GetLastError'),9);self.assertEqual(r.tls[0],7)
 def test_failed_switch_is_atomic(self):
  for mutate in [lambda row:row.update(public_CPU=None),lambda row:row.update(cached_segments_verified=False),lambda row:row.update(status='waiting'),lambda row:row.update(opaque_bytes=b'')]:
   r=model();mutate(r.records[2]);before=copy.deepcopy(r.__dict__)
   with self.assertRaises(Boundary):r.switch(2)
   self.assertEqual(before,r.__dict__)
 def test_unknown_TLS_no_fabricated_failure(self):
  r=model();old=copy.deepcopy(r.__dict__)
  with self.assertRaises(Boundary):r.kernel('TlsGetValue',3)
  self.assertEqual(r.__dict__,old)
 def test_exception_stops_without_stack_cleanup_or_CPU_change(self):
  r=model();old=copy.deepcopy(r.current_CPU)
  with self.assertRaises(Boundary)as e:r.raise_exception(0xe06d7363,1,3,0x1000,lambda p,n:b'\x01\x00\x00\x00'*3)
  self.assertEqual(e.exception.kind,'UNIMPLEMENTED_SEH_DISPATCH_STOP');self.assertEqual(r.current_CPU,old);self.assertEqual(r.boundary['arguments'],[1,1,1]);self.assertFalse(r.boundary['guest_stack_cleanup_performed']);self.assertFalse(r.boundary['handler_or_unwind_executed'])
 def test_exception_NULL_ignores_count(self):
  r=model()
  with self.assertRaises(Boundary):r.raise_exception(1,0,0xffffffff,0,lambda p,n:self.fail())
  self.assertEqual(r.boundary['arguments'],[])
 def test_exception_invalid_pointer_and_count_stop(self):
  r=model()
  with self.assertRaises(Boundary):r.raise_exception(1,0,16,0x1000,lambda p,n:self.fail())
  self.assertEqual(r.boundary['argument_capture_boundary'],'TOO_MANY_EXCEPTION_PARAMETERS')
  def fail(p,n):raise Boundary('GUEST_ARGUMENT_GUARD')
  with self.assertRaises(Boundary):r.raise_exception(1,0,2,0x1000,fail)
  self.assertEqual(r.boundary['argument_capture_boundary'],'GUEST_ARGUMENT_GUARD')
 def test_real_metadata_overrides_stale_current(self):
  m=json.loads((ROOT/'stage79/evidence/checkpoint-state.json').read_text());r=restore_metadata(m,ROOT);self.assertEqual(len(r.records),4);self.assertEqual(r.current_CPU,m['root_current_CPU']);self.assertEqual(hashlib.sha256(r.opaque).hexdigest(),m['root_opaque_context']['sha256']);self.assertNotEqual(hashlib.sha256(r.opaque).hexdigest(),m['thread_records']['1']['opaque_context']['sha256']);self.assertEqual(r.tls,{int(k):v for k,v in m['root_host']['tls'].items()});self.assertEqual(r.last_error,m['root_host']['last_error'])
  # Worker snapshot is waiting and has no public CPU. No synthetic resume.
  with self.assertRaises(Boundary):r.switch(2)
if __name__=='__main__':unittest.main()
