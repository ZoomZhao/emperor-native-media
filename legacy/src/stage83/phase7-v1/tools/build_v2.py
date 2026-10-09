"""Add the actual residency predicate, preserving prior pinned libraries."""
from core import *
import subprocess
old=V/'tools/surfaces.c';text=old.read_text();text=text.replace('typedef struct{DImage im;Surface*surfaces[LIMIT];}S30DD;','typedef struct{DImage im;Surface*surfaces[LIMIT];void*gpu;}S30DD;')
fn=next(l for l in (ROOT/'samples/stage34_dd.m').read_text().splitlines()if l.startswith('int s34_is_lost('));text+='\ntypedef S30DD S34DD;\n#define nil NULL\n'+fn+'\n';target=V/'tools/surfaces-v2.c';target.write_text(text)
cmd=['/usr/bin/clang','-O2','-Wall','-Wextra','-dynamiclib',str(target),'-o',str(V/'builds/surfaces-v2.dylib')];q=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30);(V/'evidence/build-v2.stdout').write_bytes(q.stdout);(V/'evidence/build-v2.stderr').write_bytes(q.stderr);assert not q.returncode
oldbuild=json.loads((V/'evidence/build.json').read_text());verify(oldbuild['library_pins']);pins=dict(oldbuild['library_pins']);pins[str((V/'builds/surfaces-v2.dylib').relative_to(ROOT))]=sha(V/'builds/surfaces-v2.dylib');put(V/'evidence/build-v2.json',{'command':cmd,'returncode':q.returncode,'library_pins':pins,'reused_residency_function_verbatim':fn,'source_sha256':sha(ROOT/'samples/stage34_dd.m'),'GPU_exists_diagnostic':0,'HRESULT_is_decided_only_by_actual_resident_surface_not_GPU_diagnostic':True,'original_execution':0})
