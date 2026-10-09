import argparse,hashlib,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage81'
p=argparse.ArgumentParser();p.add_argument('--audit',type=Path,default=S/'evidence/load-only-audit-v1.json');p.add_argument('--program',type=Path,default=S/'builds/load-preflight-v1');a=p.parse_args();audit=json.loads(a.audit.read_text());assert audit['guest_function_invocations_authorized']==0 and audit['initializer_or_terminator_sections']==0 and audit['LC_ROUTINES']==0
for p,h in [(audit['library_path'],audit['library_sha256']),(audit['expected_binding_file'],audit['expected_binding_sha256'])]:assert hashlib.sha256(Path(p).read_bytes()).hexdigest()==h
command=[a.program,audit['library_path'],audit['expected_binding_file'],ROOT/'stage80/evidence/measured-thread-transfer.bin',ROOT/'stage79/evidence/all-region-plan.bin',ROOT/'runs/stage12-state-cycle62p19/memory.bin',ROOT/'stage77/evidence/heap-checkpoint.bin']
q=subprocess.run([str(x)for x in command],cwd=ROOT,timeout=30);raise SystemExit(q.returncode)
