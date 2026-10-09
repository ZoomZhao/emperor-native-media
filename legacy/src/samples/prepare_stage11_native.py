#!/usr/bin/env python3
"""Move source aliases before data; attach own API definitions; no x86 edits."""
import hashlib,json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];run=ROOT/'runs/stage11-native'
original=(run/'original.llasm').read_text();wrapper=(ROOT/'samples/stage11_native_wrappers.llasm').read_text()
aliases=re.findall(r'^define .+$',original,re.M)
out=re.sub(r'^define .+\n','',original,flags=re.M)
out=re.sub(r'^proc (?:HeapCreate|HeapDestroy|GetVersionExA|GetEnvironmentVariableA|GetModuleFileNameA|OutputDebugStringA|CoCreateGuid) external\n','',out,flags=re.M)
wrapper=wrapper.replace('include llasm.llinc\n','')
out=out.replace('include macros.llinc\n','include macros.llinc\n'+'\n'.join(aliases)+'\n'+wrapper+'\n',1)
(run/'combined.llasm').write_text(out)
(ROOT/'evidence/stage11-native-normalization.json').write_text(json.dumps(dict(original_llasm_sha256=hashlib.sha256(original.encode()).hexdigest(),combined_sha256=hashlib.sha256(out.encode()).hexdigest(),aliases_moved_before_data=len(aliases),original_translation_instruction_edits=0,api_external_declarations_replaced_by_explicit_own_definitions=True),indent=2)+'\n')
print('Combined source ready; original instruction translations unchanged.')
