import json,os,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
from common import ROOT,S,sha,put
from write_gate import sandbox_command
def main():
 out=S/'runs/write-gate-own-tests';out.mkdir();outside=S/'evidence/owned-outside-sentinel.txt';outside.write_text('PRESERVE_OWN_SENTINEL\n');(out/'escape').symlink_to(S/'evidence',target_is_directory=True);outside_sha=sha(outside)
 # Audit only: actual I/O outside the task is never performed.
 script='''import sys,os
from pathlib import Path
sys.dont_write_bytecode=True
sys.path.insert(0,sys.argv[1])
from write_gate import install
out=Path(sys.argv[2]);install(out)
with(out/'audit-inside.txt').open('x')as f:f.write('ok')
paths=[Path(sys.argv[3]),out/'escape'/'owned-outside-sentinel.txt',out/'..'/'parent.txt',Path('<LOCAL_MACHINE_PATH>')]
for p in paths:
 try:p.open('w')
 except PermissionError:pass
 else:raise AssertionError('audit permitted outside/symlink/traversal')
try:os.link(sys.argv[3],out/'hardlink.txt')
except PermissionError:pass
else:raise AssertionError('hardlink permitted')
print('5 audit negative cases passed; no external task write attempted')
'''
 (out/'audit-test.py').write_text(script);q=subprocess.run(['/usr/bin/python3','-B',str(out/'audit-test.py'),str(S/'tools'),str(out),str(outside)],capture_output=True,timeout=10);put(out/'audit-result.json',{'returncode':q.returncode,'stdout':q.stdout.decode(),'stderr':q.stderr.decode()});assert q.returncode==0
 exe=S/'builds/write-probe';cmd=['/usr/bin/clang','-std=c11','-O1','-Wall','-Wextra','-Werror',str(S/'tools/write_probe.c'),'-o',str(exe)];q=subprocess.run(cmd,capture_output=True,timeout=30);put(out/'compile-result.json',{'returncode':q.returncode,'stderr':q.stderr.decode()});assert not q.returncode
 targets=[str(out/'kernel-inside.txt'),str(outside),str(out/'escape/owned-outside-sentinel.txt'),str(out/'../kernel-parent.txt')];q=subprocess.run(sandbox_command(out,[str(exe),*targets]),capture_output=True,timeout=10);put(out/'kernel-result.json',{'returncode':q.returncode,'stdout':q.stdout.decode(),'stderr':q.stderr.decode()});assert not q.returncode and sha(outside)==outside_sha and not(S/'runs/kernel-parent.txt').exists()
 # Python direct C filesystem APIs are also covered by the kernel policy.
 code='import pathlib,sys; p=pathlib.Path(sys.argv[1]);\ntry:p.write_text("ESCAPE")\nexcept PermissionError:print("python kernel denied")\nelse:raise AssertionError("kernel escape")'
 q=subprocess.run(sandbox_command(out,['/usr/bin/python3','-B','-c',code,str(outside)]),capture_output=True,timeout=10);assert not q.returncode and sha(outside)==outside_sha
 put(S/'evidence/write-gate-tests.json',{'status':'AUDIT_AND_MACOS_KERNEL_WRITE_DENIAL_TESTS_PASSED','audit_negative_cases':5,'C_kernel_negative_cases':3,'Python_kernel_negative_cases':1,'inside_writes_passed':True,'outside_sentinel_unchanged':True,'task_external_IO_actual_attempted':False,'game_or_original_context_or_code_executed':0,'sandbox_exec_sha256':sha('/usr/bin/sandbox-exec')});print('9 authored write-boundary denial cases passed, owned sentinel unchanged')
if __name__=='__main__':main()
