#!/usr/bin/env python3
"""Build only authored Stage37 code with the installed Apple clang/SDK."""
import hashlib,json,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[1]
clang='<LOCAL_MACHINE_PATH>'
sdk='<LOCAL_MACHINE_PATH>'
source=R/'samples/stage37_audio.c';library=R/'runs/stage37-native/libstage37_audio.dylib'
library.parent.mkdir(parents=True,exist_ok=True)
command=[clang,'-arch','arm64','-isysroot',sdk,'-std=c11','-O2','-Wall','-dynamiclib',str(source),'-framework','AudioToolbox','-framework','CoreAudio','-o',str(library)]
subprocess.run(command,cwd=R,check=True,timeout=20)
manifest=dict(library=str(library),sha256=hashlib.sha256(library.read_bytes()).hexdigest(),source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),command=command,architecture='arm64',author='self-authored test adapter; no original Miles implementation')
(R/'evidence/stage37-build.json').write_text(json.dumps(manifest,indent=2));print(json.dumps(manifest))
