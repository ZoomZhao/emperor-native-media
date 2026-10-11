#!/usr/bin/env python3
"""Fail-closed review gate regression; no compiler, codec or data file is run."""
import os
from pathlib import Path
import subprocess,sys,tempfile,unittest,json
REPO=Path(os.environ.get('PROFILE_AUDIT_REPO',Path(__file__).resolve().parents[2])).resolve()
RUNNER=REPO/'file_io/run_profile_integration.py'
class ReviewGate(unittest.TestCase):
    def invoke(self,flags=(),optimize=None):
        temp=tempfile.TemporaryDirectory();self.addCleanup(temp.cleanup);p=Path(temp.name)
        gate=p/'rejected-gate.json';gate.write_text(json.dumps({'schema':'INDEPENDENT_PROFILE_AND_OWNED_REVIEW_GATE_V1','approved_source_profile':False,'approved_owned_wrapper':False}))
        out=p/'must-not-be-created';env=dict(os.environ);env.pop('PYTHONOPTIMIZE',None)
        if optimize is not None:env['PYTHONOPTIMIZE']=optimize
        argv=[sys.executable,'-B',*flags,str(RUNNER),'--repo',str(REPO),'--profile',str(p/'missing-profile.json'),'--gate',str(gate),'--output',str(out)]
        result=subprocess.run(argv,capture_output=True,text=True,timeout=10,env=env)
        return result,out.exists()
    def test_normal_unapproved_gate_stops_before_profile_or_build(self):
        result,created=self.invoke();self.assertNotEqual(result.returncode,0);self.assertIn('both independent reviews required',result.stderr);self.assertFalse(created)
    def test_cli_optimization_rejected_before_any_file_gate(self):
        result,created=self.invoke(('-O',));self.assertEqual(result.returncode,2);self.assertIn('Python optimization',result.stderr);self.assertFalse(created)
    def test_environment_optimization_rejected_before_any_file_gate(self):
        result,created=self.invoke(optimize='1');self.assertEqual(result.returncode,2);self.assertIn('Python optimization',result.stderr);self.assertFalse(created)
if __name__=='__main__':unittest.main()
