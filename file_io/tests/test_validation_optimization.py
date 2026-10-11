"""Startup-only tests: no compiler, codec or original input is run."""
import ast
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

REPO = Path(os.environ.get('PROFILE_AUDIT_REPO', Path(__file__).resolve().parents[2])).resolve()
class OptimizedValidation(unittest.TestCase):
    def test_all_assert_based_runners_reject_cli_and_environment_optimization(self):
        for name in ('run_acceptance.py','run_owned_review.py','run_independent_review.py','qualify_data_file.py'):
            for optimized in ('cli','environment'):
                with self.subTest(runner=name, mode=optimized), tempfile.TemporaryDirectory() as directory:
                    out = Path(directory)/'must-not-exist'
                    argv = [sys.executable,'-B']+(['-O'] if optimized=='cli' else [])+[str(REPO/'file_io'/name),'--output',str(out)]
                    if name=='qualify_data_file.py':
                        argv += ['--input',str(Path(directory)/'not-read.bin'),'--expected-bytes','16',
                                 '--max-input','8192','--max-output','1048576','--max-blocks','64']
                    environment=dict(os.environ);environment.pop('PYTHONOPTIMIZE',None)
                    if optimized=='environment':environment['PYTHONOPTIMIZE']='1'
                    result=subprocess.run(argv,env=environment,capture_output=True,text=True,timeout=10)
                    self.assertEqual(result.returncode,2,result.stderr)
                    self.assertIn('Python optimization',result.stderr)
                    self.assertFalse(out.exists())
    def test_real_qualification_validation_has_no_assert_nodes(self):
        tree=ast.parse((REPO/'file_io/qualify_data_file.py').read_text())
        self.assertFalse(any(isinstance(node,ast.Assert) for node in ast.walk(tree)))
    def test_real_qualification_source_hash_mismatch_stops_before_output_or_input(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory).resolve();(root/'file_io').mkdir();(root/'docs').mkdir()
            (root/'file_io/qualify_data_file.py').write_bytes((REPO/'file_io/qualify_data_file.py').read_bytes())
            (root/'fake_core.cpp').write_text('// self-authored dummy; never compiled\n')
            (root/'docs/original-profile-owned-readiness.json').write_text(json.dumps({
                'fixed_production_source_sha256':{'fake_core.cpp':'0'*64},'source_profile_sha256':'0'*64}))
            out=root/'must-not-exist';environment=dict(os.environ);environment.pop('PYTHONOPTIMIZE',None)
            result=subprocess.run([sys.executable,'-B',str(root/'file_io/qualify_data_file.py'),
                '--input',str(root/'not-read.bin'),'--expected-bytes','16','--max-input','8192',
                '--max-output','1048576','--max-blocks','64','--output',str(out)],
                env=environment,capture_output=True,text=True,timeout=10)
            self.assertNotEqual(result.returncode,0)
            self.assertIn('reviewed production source SHA mismatch',result.stderr)
            self.assertFalse(out.exists())
if __name__=='__main__': unittest.main()
