import json,hashlib,ctypes,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage26_com import NativeCOM
c=NativeCOM();oracle=json.loads((ROOT/'evidence/stage26-com-Wine.json').read_text());rows=[]
for q in oracle['operations']:
 before=c.query(1)
 if q['name']=='CoUninitialize':assert c.uninitialize(1)==1;value=None
 else:
  value=c.initialize(1,2 if q['name']=='CoInitialize'else q['args'][1]);assert value==q['return_value'],q
  if value&0x80000000:assert c.query(1)==before
 rows.append(dict(name=q['name'],native_hresult=value,baseline_hresult=q['return_value']if value is not None else None,state=c.query(1),baseline_last_error=q['last_error']))
assert c.query(1)['count']==0
assert c.initialize(1,2)==0 and c.initialize(2,0)==0 and c.initialize(2,2)==0x80010106 and c.query(1)==dict(mode=2,count=1)
blob=c.export();d=NativeCOM();d.restore(blob);assert d.query(1)==c.query(1)and d.query(2)==c.query(2)
bad=bytearray(blob);bad[16:20]=(0).to_bytes(4,'little');assert not d.fn['import'](d.state,ctypes.create_string_buffer(bytes(bad)),len(bad)) and d.export()==blob
assert c.initialize(3,1)==0x80070057 and c.query(3)['count']==0
record=dict(all_passed=True,baseline_operations=len(rows),rows=rows,independent_guest_threads=True,failed_mode_change_preserved_state=True,snapshot_exact=True,malformed_import_unchanged=True,state_sha256=hashlib.sha256(blob).hexdigest(),backend='authored native arm64 guest apartment state with real pthread mutex, no Windows DLL execution');(ROOT/'evidence/stage26-com-native.json').write_text(json.dumps(record,indent=2));print(json.dumps(record));c.dispose();d.dispose()
