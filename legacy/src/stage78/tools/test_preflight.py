import sys,unittest
sys.dont_write_bytecode=True
from preflight import accept_required
class GateTests(unittest.TestCase):
 def test_unknown_denied(self):
  for x in [None,False,'true',1,[],{}]:self.assertFalse(accept_required({'gate':x}))
 def test_missing_denied(self):self.assertFalse(accept_required({}))
 def test_all_required(self):self.assertFalse(accept_required({'closed':True,'open':None}));self.assertTrue(accept_required({'a':True,'b':True}))
if __name__=='__main__':unittest.main()
