"""Own PE-only COM lifecycle litmus in the already approved isolated Wine baseline."""
import json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run
d=Data();ops=[]
sequence=[('CoInitialize',[0],0),('CoInitialize',[0],1),('CoInitializeEx',[0,0],0x80010106),('CoUninitialize',[],None),('CoInitialize',[0],1),('CoUninitialize',[],None),('CoUninitialize',[],None),('CoInitializeEx',[0,0],0),('CoInitialize',[0],0x80010106),('CoInitializeEx',[0,0],1),('CoUninitialize',[],None),('CoUninitialize',[],None),('CoInitialize',[0],0),('CoUninitialize',[],None)]
for name,args,expected in sequence:ops.append(dict(name=name,args=args,result=d.alloc(bytes(8)),expected_hresult=expected))
build('com26',d,ops,dll_for={n:'ole32.dll'for n in ['CoInitialize','CoInitializeEx','CoUninitialize']});r=run('com26');assert r['returncode']==0;body=(ROOT/'evidence/stage10-com26-output.bin').read_bytes()
for q in ops:
 q['return_value'],q['last_error']=struct.unpack_from('<II',body,d.offset(q['result']))
 if q['expected_hresult']is not None:assert q['return_value']==q['expected_hresult'],q
r.update(operations=ops,all_expected_HRESULTs_passed=True,original_game_or_MSS_code_executed=False);(ROOT/'evidence/stage26-com-Wine.json').write_text(json.dumps(r,indent=2));print(json.dumps(r))
