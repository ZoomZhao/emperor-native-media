#!/usr/bin/env python3
"""Re-run the approved fixed native graphics experiment, each process <=60s."""
import hashlib,json,subprocess,sys
from pathlib import Path
R=Path(__file__).resolve().parents[1];m=json.loads((R/'evidence/stage24-result.json').read_text())
for q in m['run_inputs']:
 p=Path(q['path']);p=p if p.is_absolute()else R/p
 if hashlib.sha256(p.read_bytes()).hexdigest()!=q['sha256']:raise SystemExit('Fixed approved input hash changed: '+str(p))
args=[str(R/'runs/stage24-sprites/emperor-sprites-arm64'),'runs/stage24-sprites/cases.bin']
if '--headless'in sys.argv:args.append('--headless')
raise SystemExit(subprocess.run(['/usr/bin/python3',str(R/'samples/run_bounded.py'),'stage24-delivery','.',*args],cwd=R,timeout=65).returncode)
