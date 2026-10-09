#!/usr/bin/env python3
import ctypes,hashlib,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage14_messages import NativeMessages
def main():
 native=NativeMessages();baseline=json.loads((ROOT/'evidence/stage14-message-Wine-result.json').read_text());rows=[];mapping={}
 clients=[]
 for letter in ['a','b']:
  lib=ctypes.CDLL(str(ROOT/f'runs/stage14-native/libclient_{letter}.dylib'));f=getattr(lib,'s14_client_'+letter);f.restype=ctypes.c_uint32;f.argtypes=[ctypes.c_void_p,ctypes.c_char_p,ctypes.POINTER(ctypes.c_uint32)];clients.append((lib,f))
 for row in baseline['rows']:
  text=row['text'].encode();function=row['module'];e=ctypes.c_uint32(0x13579)
  if function=='RegisterWindowMessageA':value,error=native.register(text,e.value)
  else:value=clients[0 if function=='RegisterFromA' else 1][1](native.state,text,ctypes.byref(e));error=e.value
  assert error==row['last_error']
  if text:
   assert 0xc000<=value<=0xffff
   known=mapping.setdefault(row['return_value'],value);assert known==value
   assert len(set(mapping.values()))==len(mapping)
  else:assert value==0
  rows.append(dict(module=function,text=row['text'],Wine_session_ID=row['return_value'],native_session_ID=value,last_error=error))
 # Snapshot continuation compares whole registry and future allocated IDs.
 blob=native.export();restored=NativeMessages();restored.restore(blob);tail=[b'ST14_SHARED',b'ST14_Future',b'st14_future',b'',b'commctrl_draglistmsg'];results=[]
 for text in tail:
  a=native.register(text,0x125);b=restored.register(text,0x125);assert a==b;results.append(dict(text=text.decode(),result=a))
 assert native.export()==restored.export()
 unchanged=restored.export();assert restored.register(b'nonASCII\xff',0x155)==(0xfffffffe,0x155);assert restored.export()==unchanged
 # Import validation must reject bad state without altering the live session.
 bad=bytearray(unchanged);bad[16:20]=b'\x01\x00\x00\x00';buf=ctypes.create_string_buffer(bytes(bad));assert restored.import_c(restored.state,buf,len(bad))==0;assert restored.export()==unchanged
 result=dict(all_passed=True,measured_Wine_comparisons=len(rows),two_distinct_native_modules_share_session=True,rows=rows,Wine_IDs_are_capture_only=True,native_IDs_allocated_in_own_isolated_session=True,snapshot_size=len(blob),snapshot_sha256=hashlib.sha256(blob).hexdigest(),snapshot_future_steps=results,exact_registry_equal_after_resume=True,unknown_input_refuses_without_state_change=True,invalid_snapshot_rejected=True,no_windows_or_message_pump=True,scope='ASCII letters/digits/underscore names, 1..255 bytes, 128 records; no external Win32 interoperability or full message dispatch')
 (ROOT/'evidence/stage14-message-native-regression.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['rows','snapshot_future_steps']}));native.dispose();restored.dispose()
if __name__=='__main__':main()
