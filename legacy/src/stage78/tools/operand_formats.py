"""Strict finite i386 format decoder, not a pointer or semantic proof.

No disassembler/emulator is loaded. Each decoded extent is checked separately
against the pinned LLVM byte inventory and original PE file. Unknown encodings
raise; mapped-image immediate values remain ambiguous integers/pointers.
"""
import collections, hashlib, json, struct, sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'samples'))
from auto_i386_analysis import PE,decode as old_decode
REGS=['eax','ecx','edx','ebx','esp','ebp','esi','edi']
class Unsupported(ValueError):pass
def decode(body,pc=0):
 b=bytes(body);at=0;fields=[];prefix=[];operands=[]
 def take(n):
  nonlocal at
  if at+n>len(b):raise Unsupported('truncated field')
  v=int.from_bytes(b[at:at+n],'little');at+=n;return v
 def field(n,kind,signed=False):
  start=at;v=take(n)
  if signed and v&(1<<(n*8-1)):v-=1<<(n*8)
  f=dict(kind=kind,site=pc+start,width=n,value=v);fields.append(f);return f
 while at<len(b) and b[at] in (0x66,0x64,0x65,0x26,0x2e,0x36,0x3e,0xf2,0xf3,0x67,0xf0):
  p=take(1)
  if p in prefix:raise Unsupported('duplicate prefix')
  prefix.append(p)
 if 0x67 in prefix or 0xf0 in prefix:raise Unsupported('address-size/LOCK outside reviewed set')
 bits=16 if 0x66 in prefix else 32;n=bits//8;segment=next((x for x in prefix if x in (0x64,0x65,0x26,0x2e,0x36,0x3e)),None)
 if sum(x in (0x64,0x65,0x26,0x2e,0x36,0x3e) for x in prefix)>1:raise Unsupported('multiple segment overrides')
 def register(i,w):return dict(kind='reg',reg=i,bits=w)
 def modrm(w=bits):
  m=take(1);mode=m>>6;group=(m>>3)&7;base=m&7;idx=None;scale=1;disp=None
  if mode==3:return register(base,w),group
  if base==4:
   s=take(1);base=s&7;idx=(s>>3)&7;scale=1<<(s>>6)
   if idx==4:idx=None
  if mode==0 and base==5:base=None;disp=field(4,'absolute_displacement')
  elif mode==1:disp=field(1,'indexed_displacement',True)
  elif mode==2:disp=field(4,'indexed_displacement',True)
  o=dict(kind='mem',base=base,index=idx,scale=scale,disp=disp,bits=w,segment=segment);operands.append(o);return o,group
 def immediate(width=n):
  f=field(width,'immediate');operands.append(f);return f
 op=take(1);name=None;branch=None
 alu=['add','or','adc','sbb','and','sub','xor','cmp']
 if op<0x40 and op&7<=5:
  name=alu[op>>3];fmt=op&7
  if fmt<=3:
   w=8 if fmt%2==0 else bits;m,r=modrm(w);operands.append(register(r,w))
  else:operands.append(register(0,8 if fmt==4 else bits));immediate(1 if fmt==4 else n)
 elif 0x40<=op<=0x5f:
  name='inc' if op<0x48 else 'dec' if op<0x50 else 'push' if op<0x58 else 'pop';operands.append(register(op&7,bits))
 elif op in (0x68,0x6a):name='push';immediate(n if op==0x68 else 1)
 elif op in (0x69,0x6b):name='imul';m,r=modrm();operands.append(register(r,bits));immediate(n if op==0x69 else 1)
 elif 0x70<=op<=0x7f:name='jcc';branch=field(1,'relative',True)
 elif op in (0x80,0x81,0x83):m,g=modrm(8 if op==0x80 else bits);name=alu[g];immediate(n if op==0x81 else 1)
 elif op in (0x84,0x85):name='test';m,r=modrm(8 if op==0x84 else bits);operands.append(register(r,8 if op==0x84 else bits))
 elif op in (0x88,0x89,0x8a,0x8b,0x8d):name='lea' if op==0x8d else 'mov';m,r=modrm(8 if op in (0x88,0x8a) else bits);operands.append(register(r,8 if op in (0x88,0x8a) else bits))
 elif op==0x8f:
  m,g=modrm();name='pop'
  if g!=0:raise Unsupported('8f extension')
 elif op==0x90:name='nop'
 elif op==0x99:name='cwd' if bits==16 else 'cdq'
 elif 0xa0<=op<=0xa3:
  name='mov';f=field(4,'absolute_displacement');operands.append(dict(kind='mem',base=None,index=None,scale=1,disp=f,bits=8 if op%2==0 else bits,segment=segment))
 elif op in (0xa4,0xa5,0xaa,0xab,0xae,0xaf):name={0xa4:'movsb',0xa5:'movsw' if bits==16 else 'movsd',0xaa:'stosb',0xab:'stosw' if bits==16 else 'stosd',0xae:'scasb',0xaf:'scasw' if bits==16 else 'scasd'}[op]
 elif op in (0xa8,0xa9):name='test';operands.append(register(0,8 if op==0xa8 else bits));immediate(1 if op==0xa8 else n)
 elif 0xb0<=op<=0xbf:name='mov';operands.append(register(op&7,8 if op<0xb8 else bits));immediate(1 if op<0xb8 else n)
 elif op in (0xc0,0xc1,0xd0,0xd1,0xd2,0xd3):
  m,g=modrm(8 if op%2==0 else bits);name={0:'rol',1:'ror',2:'rcl',3:'rcr',4:'shl',5:'shr',7:'sar'}.get(g)
  if name is None:raise Unsupported('invalid shift extension')
  if op in (0xc0,0xc1):immediate(1)
 elif op in (0xc2,0xc3):name='ret';immediate(2) if op==0xc2 else None
 elif op in (0xc6,0xc7):
  name='mov';m,g=modrm(8 if op==0xc6 else bits)
  if g!=0:raise Unsupported('invalid MOV extension')
  immediate(1 if op==0xc6 else n)
 elif op==0xc9:name='leave'
 elif op in (0xe8,0xe9,0xeb):name='call' if op==0xe8 else 'jmp';branch=field(1 if op==0xeb else n,'relative',True)
 elif op in (0xf6,0xf7):
  m,g=modrm(8 if op==0xf6 else bits);name={0:'test',2:'not',3:'neg',4:'mul',5:'imul',6:'div',7:'idiv'}.get(g)
  if name is None:raise Unsupported('invalid F6/F7 extension')
  if g==0:immediate(1 if op==0xf6 else n)
 elif op in (0xfc,0xfd):name='cld' if op==0xfc else 'std'
 elif op in (0xfe,0xff):
  m,g=modrm(8 if op==0xfe else bits);name={0:'inc',1:'dec',2:'call',4:'jmp',6:'push'}.get(g)
  if name is None or op==0xfe and g>1:raise Unsupported('invalid FE/FF extension')
 elif op==0x0f:
  second=take(1)
  if 0x80<=second<=0x8f:name='jcc';branch=field(n,'relative',True)
  elif 0x90<=second<=0x9f:name='setcc';m,g=modrm(8)
  elif second in (0xb6,0xb7,0xbe,0xbf):name='movzx' if second<0xbe else 'movsx';m,g=modrm(8 if second%2==0 else 16);operands.append(register(g,bits))
  elif second==0xaf:name='imul';m,g=modrm();operands.append(register(g,bits))
  else:raise Unsupported('unreviewed 0f '+hex(second))
 else:raise Unsupported('unreviewed opcode '+hex(op))
 if any(x in prefix for x in (0xf2,0xf3)) and not name.startswith(('movs','stos','scas')):raise Unsupported('repeat prefix on non-string')
 if segment is not None and not any(o['kind']=='mem' for o in operands):raise Unsupported('segment on implicit/string access outside reviewed set')
 return dict(pc=pc,end=pc+at,name=name,prefixes=prefix,fields=fields,operands=operands,branch_target=pc+at+branch['value'] if branch else None)
def legacy_decode(pe,pc):
 d=decode(pe.read(pc,15),pc);args=[]
 for op in d['operands']:
  if op['kind']=='mem':
   f=op['disp'];args.append(dict(kind='mem',base=op['base'],index=op['index'],scale=op['scale'],disp=f['value'] if f else 0,site=f['site'] if f else None,width=f['width'] if f else 0,bits=op['bits'],segment=op['segment']))
  elif op['kind']=='immediate':args.append(dict(kind='imm',value=op['value'],site=op['site'],width=op['width']))
 return dict(pc=pc,end=d['end'],name=d['name'],args=args)
def category(q):
 b=bytes.fromhex(q['bytes']);m=q['mnemonic']
 if b[0]==0x64:return 'FS_segment_memory'
 if b[0] in (0xf2,0xf3):return 'REP_implicit_memory'
 if m in ('leave','std','cld'):return 'implicit_stack_or_DF'
 if b[0]==0x66:return 'operand_size_16'
 if m in ('movzx','movsx','imul','neg','not','inc','dec'):return 'typed_ModRM_or_extension'
 return 'arithmetic_ModRM_or_accumulator'
def audit(output):
 c=json.loads((ROOT/'stage76/evidence/full-dependency-closure-v1.json').read_text());pe=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe');assert hashlib.sha256(pe.data).hexdigest()==c['original_sha256']
 failures=[];resolved_old=[];candidates=[];fields=collections.Counter()
 for q in c['instructions']:
  pc=q['pc'];b=bytes.fromhex(q['bytes']);assert pe.read(pc,len(b))==b
  try:old_decode(pe,pc)
  except (ValueError,AssertionError,KeyError):resolved_old.append(dict(PC=hex(pc),category=category(q),mnemonic=q['mnemonic']))
  try:
   d=decode(b,pc);assert d['end']==q['end'],(hex(pc),d,q)
   expected=q['mnemonic'];actual=d['name']
   assert expected==actual or actual=='jcc' and expected.startswith('j') or actual=='setcc' and expected.startswith('set') or expected in ('rep','repne') and actual.startswith(('movs','stos','scas')),(q,actual)
   for f in d['fields']:fields[f['kind']]+=1
   for op in d['operands']:
    if op['kind']=='mem' and op['disp'] and op['disp']['width']==4:
     f=op['disp'];v=f['value']&0xffffffff;s=pe.section(v)
     if s:candidates.append(dict(PC=hex(pc),site=hex(f['site']),VA=hex(v),kind='segment_relative_requires_descriptor' if op['segment'] else 'absolute_memory' if op['base'] is None and op['index'] is None else 'indexed_displacement_not_pointer_proved'))
    elif op['kind']=='immediate' and op['width']==4 and pe.section(op['value']):candidates.append(dict(PC=hex(pc),site=hex(op['site']),VA=hex(op['value']),kind='mapped_immediate_not_pointer_proved'))
  except (Unsupported,AssertionError) as e:failures.append(dict(PC=hex(pc),bytes=q['bytes'],mnemonic=q['mnemonic'],reason=str(e)[:500]))
 r=dict(status='FORMAT_AUDIT_COMPLETE' if not failures else 'FORMAT_AUDIT_OPEN',original_instructions=len(c['instructions']),extent_and_mnemonic_verified=len(c['instructions'])-len(failures),old_unsupported_sites=len(resolved_old),old_unsupported_categories=dict(collections.Counter(x['category'] for x in resolved_old)),remaining_format_failures=failures,encoded_field_counts=dict(fields),image_value_candidates=candidates,candidate_counts=dict(collections.Counter(x['kind'] for x in candidates)),format_decode_is_pointer_or_semantic_proof=False,source_bytes_changed=0,game_or_Unicorn_executions=0,old_unsupported_inventory=resolved_old)
 Path(output).write_text(json.dumps(r,indent=2)+'\n');print(json.dumps({k:v for k,v in r.items() if k not in ('old_unsupported_inventory','image_value_candidates')}));return r
if __name__=='__main__':audit(sys.argv[1])
