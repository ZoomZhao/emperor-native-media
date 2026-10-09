#!/usr/bin/python3
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parent.parent
subprocess.run(['/usr/bin/python3',str(ROOT/'scripts/run.py'),'generate'],check=True,timeout=55)
