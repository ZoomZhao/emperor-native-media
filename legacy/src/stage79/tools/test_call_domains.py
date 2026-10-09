import json,sys,unittest
from pathlib import Path
sys.dont_write_bytecode=True
from call_domains import resolve
from thread_contract import Boundary
ROOT=Path(__file__).resolve().parents[2]
class Tests(unittest.TestCase):
 def setUp(self):self.c=json.loads((ROOT/'stage79/evidence/call-domains.json').read_text())['conditional_singleton_site_certificates']
 def test_fixed_original_readonly_case(self):
  for c in self.c:
   for r in c['cases']:
    if r['target_in_full_function_entries']:self.assertEqual(resolve(self.c,c['site'],r['target'],{c['vptr_register']:r['vtable']},lambda a,n:bytes.fromhex(r['slot_bytes']),lambda a,n:1),r['target'])
 def test_unknown_not_skipped(self):
  with self.assertRaises(Boundary):resolve(self.c,0x4c1d26,1,{},lambda a,n:b'',lambda a,n:1)
 def test_table_integrity_and_permission(self):
  c=self.c[0];r=c['cases'][0]
  for read,p in [(lambda a,n:b'\0'*4,1),(lambda a,n:bytes.fromhex(r['slot_bytes']),3)]:
   with self.assertRaises(Boundary):resolve(self.c,c['site'],r['target'],{c['vptr_register']:r['vtable']},read,lambda a,n:p)
 def test_unseen_vptr_or_wrong_target(self):
  c=self.c[0];r=c['cases'][0]
  for vt,t in [(0x7ab59c,r['target']),(r['vtable'],r['target']+1)]:
   with self.assertRaises(Boundary):resolve(self.c,c['site'],t,{c['vptr_register']:vt},lambda a,n:bytes.fromhex(r['slot_bytes']),lambda a,n:1)
if __name__=='__main__':unittest.main()
