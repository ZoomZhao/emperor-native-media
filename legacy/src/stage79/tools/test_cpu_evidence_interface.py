import contextlib,importlib.util,io,sys,unittest
from pathlib import Path
sys.dont_write_bytecode=True
spec=importlib.util.spec_from_file_location('own_cpu_once',Path(__file__).with_name('cpu_evidence_once.py'));module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
class Tests(unittest.TestCase):
 def test_describe_does_not_load_engine(self):
  old=sys.argv;sys.argv=['own','--describe']
  try:
   with contextlib.redirect_stdout(io.StringIO()):module.main()
  finally:sys.argv=old
  self.assertNotIn('unicorn',sys.modules)
 def test_missing_approval_refuses_before_claim_or_engine(self):
  old=sys.argv;sys.argv=['own','--run']
  try:
   with self.assertRaisesRegex(RuntimeError,'approval'):module.main()
  finally:sys.argv=old
  self.assertNotIn('unicorn',sys.modules);self.assertFalse((module.S/'evidence/cpu-evidence-attempt.claim.json').exists())
if __name__=='__main__':unittest.main()
