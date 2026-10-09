"""Strict, fail-closed rewrite of SR pointer-result -> immediate tailcall.

Native pointers never become guest uint32 values. Original CALL/RET values,
guest data accesses and CPU ABI remain unchanged. Numeric SSA names become
named identifiers so deleting a proven dead conversion cannot break numbering.
"""
import hashlib,json,re
from pathlib import Path
LOOKUPS={'stage72_lookup_return','stage77_lookup_target','stage81_lookup_authored_call'}
def transform(text,*,expected_lookups=None):
 lines=text.splitlines();output=[];i=0;sites=[];static_addresses=[];removed_wrappers=0;removed_globals=[];body=False;function=''
 while i<len(lines):
  l=lines[i]
  if l.startswith('@')and re.match(r'@_(?:data|rdata|s\w+)\s*=',l):
   removed_globals.append(l.split(' = ',1)[0]);i+=1;continue
  if re.match(r'define .* @c_\w+\(',l):
   removed_wrappers+=1;i+=1
   while i<len(lines)and lines[i]!='}':i+=1
   assert i<len(lines);i+=1;continue
  if l.startswith('define '):body=True;function=l.split('@',1)[1].split('(',1)[0]
  address=re.fullmatch(r'(%\d+) = ptrtoint void\(%_cpu\*\)\* @(?:loc_|function_)([0-9A-Fa-f]+) to i64',l)
  if address:
   sub=re.fullmatch(r'(%\d+) = sub i64 (%\d+), (%\d+)',lines[i+1]);trunc=re.fullmatch(r'(%\d+) = trunc i64 (%\d+) to i32',lines[i+2]);assert sub and trunc and sub[2]==address[1]and trunc[2]==sub[1]
   value=int(address[2],16);assert value<=0xffffffff
   # Address-of-original-symbol remains the original guest VA, including
   # jump-table addresses that SR mistakenly models as native code labels.
   output.append(trunc[1]+' = add i32 0, '+str(value));static_addresses.append(dict(function=function,original_guest_VA=value,native_symbol=address[2]));i+=3;continue
  call=re.fullmatch(r'(%\d+) = call i8\* @(stage72_lookup_return|stage77_lookup_target|stage81_lookup_authored_call)\((.+)\) nounwind',l)
  if call:
   pointer=call[1];j=i+1
   while j<len(lines)and not lines[j].startswith('musttail '):
    assert lines[j]!='}'and j-i<24,(function,lines[j]);j+=1
   chunk=lines[i+1:j];plain=[];scratch={};aliases={};function_start=next(k for k in range(i,-1,-1)if lines[k].startswith('define '));prefix='\n'.join(lines[function_start:i])
   for x in chunk:
    if not x or x.startswith(';'):continue
    for name,value in aliases.items():x=re.sub(re.escape(name)+r'\b',lambda m:value,x)
    store=re.fullmatch(r'store i(32|64) (%\d+), i\1\* (%tmp0|%ptrofs), align (4|8)',x)
    if store:
     assert re.search(re.escape(store[3])+r' = alloca i'+store[1]+r', align '+store[4],prefix),'scratch is not private alloca'
     scratch[store[3]]=store[2];continue
    load=re.fullmatch(r'(%\d+) = load i(32|64), i\2\* (%tmp0|%ptrofs), align (4|8)',x)
    if load and load[3]in scratch:aliases[load[1]]=scratch[load[3]];continue
    plain.append(x)
   # Exact ten-instruction producer/consumer chain emitted by pinned llasm.
   assert len(plain)==10,(function,plain)
   pat=[r'(%\d+) = getelementptr %_cpu, %_cpu\* %cpu, i32 0, i32 13',r'(%\d+) = load i64, i64\* (%\d+), align 8',r'(%\d+) = ptrtoint i8\* (%\d+) to i64',r'(%\d+) = sub i64 (%\d+), (%\d+)',r'(%\d+) = trunc i64 (%\d+) to i32',r'(%\d+) = icmp eq i64 (%\d+), 0',r'(%\d+) = select i1 (%\d+), i32 0, i32 (%\d+)',r'(%\d+) = zext i32 (%\d+) to i64',r'(%\d+) = add i64 (%\d+), (%\d+)',r'(%\d+) = inttoptr i64 (%\d+) to void\(%_cpu\*\)\*']
   matches=[re.fullmatch(p,x)for p,x in zip(pat,plain)];assert all(matches),(function,plain);a,b,c,d,e,f,g,h,k,z=matches
   assert b[2]==a[1]and c[2]==pointer and(d[2],d[3])==(c[1],b[1])and e[2]==d[1]and f[2]==c[1]and(g[2],g[3])==(f[1],e[1])and h[2]==g[1]and(k[2],k[3])==(h[1],b[1])and z[2]==k[1]
   assert lines[j]=='musttail call fastcc void '+z[1]+'(%_cpu* %cpu) nounwind'
   # No deleted SSA producer may escape this immediate tail-transfer sequence.
   rest=[];q=j+1
   while q<len(lines)and lines[q]!='}':rest.append(lines[q]);q+=1
   assert rest==['ret void'],(function,rest)
   before='\n'.join(lines[i+1:j]);after=z[1]+' = bitcast i8* '+pointer+' to void(%_cpu*)*'
   output.extend([l,after,lines[j]]);sites.append(dict(function=function,resolver=call[2],removed_conversion_instructions=10,source_chain_sha256=hashlib.sha256(before.encode()).hexdigest()));i=j+1;continue
  output.append(l)
  if l=='}':body=False
  i+=1
 out='\n'.join(output)+'\n';function=False;renamed=[]
 for l in out.splitlines():
  if l.startswith('define '):function=True
  if function:l=re.sub(r'%(\d+)\b',r'%s81_\1',l)
  renamed.append(l)
  if l=='}':function=False
 out='\n'.join(renamed)+'\n'
 for name in removed_globals:assert not re.search(re.escape(name)+r'\b',out),('referenced native shadow data',name)
 assert not re.search(r'inttoptr i(?:32|64) .* to void\(%_cpu\*\)\*',out),'unresolved integer code target'
 assert not any('ptrtoint'in l for l in out.splitlines()if not l.startswith('@')),'remaining body pointer truncation'
 if expected_lookups is not None:assert len(sites)==expected_lookups,(len(sites),expected_lookups)
 return out,dict(native_pointer_tail_transfers=len(sites),original_static_symbol_VA_restorations=static_addresses,removed_generated_host_C_wrappers=removed_wrappers,removed_unreferenced_native_shadow_globals=removed_globals,native_pointer_to_guest_integer_conversions_remaining=0,integer_to_native_code_casts_remaining=0,original_guest_PC_or_data_address_edits=0,sites=sites)
def apply(source,destination,report,expected):
 text=Path(source).read_text();out,r=transform(text,expected_lookups=expected);Path(destination).write_text(out);r.update(source_sha256=hashlib.sha256(text.encode()).hexdigest(),output_sha256=hashlib.sha256(out.encode()).hexdigest());Path(report).write_text(json.dumps(r,indent=2)+'\n');return r
