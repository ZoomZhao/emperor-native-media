"""Reject initializer-bearing Mach-O before any dlopen of game-derived code."""
import argparse,hashlib,json,re,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('library',type=Path);p.add_argument('output',type=Path);a=p.parse_args();b=a.library.read_bytes();magic,cpu,sub,kind,ncmd,size,flags,res=struct.unpack_from('<8I',b);assert(magic,cpu,kind)==(0xfeedfacf,0x100000c,6);at=32;sections=[];deps=[];text=[]
 for i in range(ncmd):
  cmd,n=struct.unpack_from('<II',b,at);assert n>=8 and at+n<=32+size;assert cmd not in {0x11,0x1a},'LC_ROUTINES initializer'
  if cmd==0x19:
   nc=struct.unpack_from('<I',b,at+64)[0]
   for j in range(nc):
    s=struct.unpack_from('<16s16sQQ8I',b,at+72+80*j);name=s[0].split(b'\0')[0].decode();seg=s[1].split(b'\0')[0].decode();stype=s[8]&255
    assert stype not in {9,10,0x15,0x16}and name not in {'__mod_init_func','__mod_term_func','__init_offsets'}and not name.startswith('__objc'),('initializer section',name,stype)
    sections.append(dict(section=name,segment=seg,size=s[3],flags=s[8]))
    if seg=='__TEXT'and name=='__text':text.append((s[2],s[3]))
  if cmd in {0xc,0x80000018,0x8000001f,0x80000023}:
   offset=struct.unpack_from('<I',b,at+8)[0];name=b[at+offset:at+n].split(b'\0')[0].decode();assert name=='/usr/lib/libSystem.B.dylib',('unapproved dependency',name);deps.append(name)
  at+=n
 assert at==32+size and len(text)==1
 source=ROOT/'stage81/builds/full-bound-guarded-open/guarded.ll';rows={}
 for l in source.open():
  for name in ['stage72_return_bindings','stage77_structural_code_bindings']:
   if l.startswith('@'+name+' ='):rows[name]=[int(v)for v in re.findall(r'\{i32,i8\*\} \{i32 (\d+),',l)]
 returns=rows['stage72_return_bindings'];targets=rows['stage77_structural_code_bindings'];assert(len(returns),len(targets))==(432,2020)
 expected=a.output.parent/(a.output.stem+'-expected-bindings.bin');expected.write_bytes(b'S81B'+struct.pack('<IIQQ',len(returns),len(targets),*text[0])+struct.pack('<%dI'%(len(returns)+len(targets)),*(returns+targets)))
 result=dict(status='LOAD_ONLY_MACHO_NO_INITIALIZERS_AUDITED',library_path=str(a.library.resolve()),library_sha256=hashlib.sha256(b).hexdigest(),initializer_or_terminator_sections=0,LC_ROUTINES=0,dependencies=deps,sections=sections,text_vmaddr=text[0][0],text_size=text[0][1],expected_binding_file=str(expected.resolve()),expected_binding_sha256=hashlib.sha256(expected.read_bytes()).hexdigest(),guest_function_invocations_authorized=0,self_authored_C_setup_provider_symbols=['s80_activate','stage79_active_threads','stage77_active_heap_context','stage78_current_context'],self_authored_C_setup_provider_calls_maximum=5,SR_or_original_code_binding_invocations_authorized=0,RTLD_LAZY_does_not_prove_missing_platform_APIs_implemented=True)
 a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items()if k!='sections'}))
if __name__=='__main__':main()
