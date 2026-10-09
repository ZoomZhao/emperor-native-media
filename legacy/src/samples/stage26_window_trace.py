"""Own WndProc records actual creation-message prefix in approved isolated Wine."""
import struct,json,hashlib,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run,BASE
from inspect_pe import inspect
d=Data();count=d.alloc(bytes(4));records=d.alloc(bytes(64*16));cls=d.alloc(b'Stage26TraceClass\0');title=d.alloc(b'Own Stage26 creation-message trace\0');wc=d.alloc(struct.pack('<10I',0,BASE+0xf000,0,0,BASE,0,0,0,0,cls));atom=d.alloc(bytes(8));hwnd=d.alloc(bytes(8));destroy=d.alloc(bytes(8));dummy=d.alloc(bytes(8))
ops=[dict(name='RegisterClassA',args=[wc],result=atom),dict(name='CreateWindowExA',args=[0x40000,cls,title,0x90080000,0,0,1440,900,0,0,BASE,0],result=hwnd),dict(name='DestroyWindow',args=[{'load':hwnd}],result=destroy),dict(name='DefWindowProcA',args=[0,0,0,0],result=dummy)]
meta=build('window26trace',d,ops,callback_stub=True,dll_for={name:'USER32.dll'for name in ['RegisterClassA','CreateWindowExA','DestroyWindow','DefWindowProcA']});exe=ROOT/'samples/stage10-window26trace.exe';p=inspect(exe);iat=next(int(s['iat_va'],16)for dll in p['imports']for s in dll['symbols']if s.get('name')=='DefWindowProcA')
# Callback preserves nonvolatile registers and tail-calls real DefWindowProcA.
code=bytearray(b'\x55\x89\xe5\xa1'+struct.pack('<I',count)+b'\x83\xf8\x40\x0f\x83');branch=len(code);code.extend(bytes(4));code.extend(b'\x89\xc2\xc1\xe2\x04')
for off in [0,4,8,12]:code.extend(b'\x8b\x4d'+bytes([8+off])+b'\x89\x8a'+struct.pack('<I',records+off))
code.extend(b'\xff\x05'+struct.pack('<I',count));tail=len(code);code.extend(b'\x5d\xff\x25'+struct.pack('<I',iat));struct.pack_into('<i',code,branch,tail-branch-4)
data=bytearray(exe.read_bytes());sec=next(s for s in p['sections']if s['name']=='.text');at=sec['raw_offset']+0xe000;data[at:at+len(code)]=code;pe=struct.unpack_from('<I',data,0x3c)[0];opt=pe+24;struct.pack_into('<I',data,opt+224+8,0xe000+len(code));exe.write_bytes(data);meta.update(sha256=hashlib.sha256(data).hexdigest(),size=len(data),callback_all_authored_here=True,callback_calls_real_DefWindowProcA=True);(ROOT/'evidence/stage10-window26trace-source.json').write_text(json.dumps(meta,indent=2))
r=run('window26trace');assert r['returncode']==0;out=(ROOT/'evidence/stage10-window26trace-output.bin').read_bytes();n=struct.unpack_from('<I',out,d.offset(count))[0];assert 0<n<64;messages=[dict(zip(['hwnd','message','wparam','lparam'],struct.unpack_from('<4I',out,d.offset(records)+16*i)))for i in range(n)];created=struct.unpack_from('<I',out,d.offset(hwnd))[0];assert created
r.update(messages=messages,first_creation_message=messages[0]['message'],create_args=ops[1]['args'],native_Win32_WndProc_called=True,original_game_code_executed=False,original_MSS_code_executed=False);(ROOT/'evidence/stage26-window-Wine-trace.json').write_text(json.dumps(r,indent=2));print(json.dumps(r))
