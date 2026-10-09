# -*- coding: utf-8 -*-
"""Decode fixed original DLL result, without another DLL invocation."""
import hashlib,json,struct
from pathlib import Path
R=Path(__file__).resolve().parents[1]
m=json.loads((R/'evidence/stage36-MSS-pending-probe.json').read_text())
execution=json.loads((R/'evidence/stage36-MSS-execution.json').read_text())
b=(R/'evidence/stage36-MSS-result.bin').read_bytes()
assert len(b)==m['output_size'] and hashlib.sha256(b).hexdigest()==execution['output_sha256']
def word(p):return struct.unpack_from('<I',b,p-m['data_base'])[0]
rows=[]
for i,q in enumerate(m['operations']):
 row=dict(index=i,name=q.get('original_export',q['name']),args=q['args'],EAX_u32=word(q['result']),GetLastError_u32=word(q['result']+4))
 row['EAX_s32']=row['EAX_u32']if row['EAX_u32']<0x80000000 else row['EAX_u32']-0x100000000
 rows.append(row)
assert execution['returncode']==0
assert all(r['EAX_u32']for r in rows if r['name']in ['LoadLibraryA','GetProcAddress'])
starts=[r['EAX_u32']for r in rows if r['name']=='_AIL_startup@0']
prefs=[r['EAX_u32']for r in rows if r['name']=='_AIL_get_preference@4']
wave=[r for r in rows if r['name']=='_AIL_waveOutOpen@16'][0]
driver=word(m['driver_slot'])
assert starts==[1,0,1],starts
assert prefs[1:]==[32,8,8],prefs
log=(R/'evidence/stage36-MSS-execution.log').read_text(errors='replace')
providers={name:dict(load_count=sum('Loaded L"C:\\\\MSS36StartupProbe\\\\'+name+'"'in line for line in log.splitlines()),unload_count=sum('Unloaded module L"C:\\\\MSS36StartupProbe\\\\'+name+'"'in line for line in log.splitlines()))for name in ['Mp3dec.asi','Mssds3dh.m3d','Mssrsx.m3d']}
assert all(v['load_count']==v['unload_count']==2 for v in providers.values()),providers
out=dict(scope='MSS36_STARTUP_DEVICE_V1',probe_completed=True,cold_repeated_restart_startup=starts,preference42_before_set_after_set_after_cold_after_restart=prefs,waveOutOpen_result=wave['EAX_u32'],driver_handle_u32=driver,device_open_success=wave['EAX_u32']==0 and driver!=0,waveOutClose_called=wave['EAX_u32']==0 and driver!=0,provider_load_unload=providers,operations=rows,GetLastError_note='函数未保证清除 LastError，结果只记录，不解释成 Miles 错误码。shutdown/close/set_directory 未由返回值独立验证副作用。',probe_output_sha256=execution['output_sha256'],no_sample_stream_3D_contract_tested=True,no_extra_original_calls=True)
(R/'evidence/stage38-MSS-decoded.json').write_text(json.dumps(out,indent=2,ensure_ascii=False));print(json.dumps({k:out[k]for k in ['probe_completed','cold_repeated_restart_startup','preference42_before_set_after_set_after_cold_after_restart','waveOutOpen_result','driver_handle_u32','device_open_success','provider_load_unload']},ensure_ascii=False))
