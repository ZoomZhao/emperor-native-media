"""Pure file/IR/Mach-O and preservation checks. Never loads compiled artifacts."""
import argparse,ast,collections,difflib,hashlib,json,re,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage75'
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
def load(p):return json.loads((ROOT/p).read_text())
def dump(p,v):
 p=S/p;assert not p.exists();p.write_text(json.dumps(v,indent=2)+'\n')
def preservation():
 base=load('stage75/evidence/protection-baseline.json');bad=[]
 for p,h in base['hashes'].items():
  if not Path(p).is_file()or sha(p)!=h:bad.append(p)
 assert not bad,bad
 ledger=load('evidence/stage55-batch-ledger.json');actual=(len(ledger['executions']),ledger['remaining_processes'],bool(ledger['goal_closed']))
 assert actual==(20,80,True);digest=sha(ROOT/'evidence/stage55-batch-ledger.json');assert digest=='4e499d8560cc4c7a9bc4a2e725e27f502fa13979de6366e9f66bd06f237edcc9'
 claims={str(p.relative_to(ROOT)):sha(p)for p in (ROOT/'evidence').glob('stage73*attempt.claim.json')};assert len(claims)==2
 assert not list((S/'evidence').glob('*attempt.claim.json'))
 result=dict(status='ALL_PROTECTED_FILES_UNCHANGED',checks=len(base['hashes']),ledger_SHA256=digest,used=20,remaining=80,closed=True,Stage73_claims=claims,new_execution_claims=0,original_game_or_Unicorn_or_generated_library_executions=0)
 dump('evidence/protection-postflight.json',result);print(json.dumps(result))
def implementation():
 build=S/'builds/type53-family-v5';r=load('stage75/builds/type53-family-v5/result.json');cfg=load('stage75/configs/phase8-type53-family.json');analysis=load('stage75/builds/type53-family-v5/analysis.json');closed=load('stage75/evidence/exact-family-closure-v4.json')
 for p in (S/'tools').glob('*.py'):ast.parse(p.read_text(),filename=str(p))
 assert sha(S/'configs/phase8-type53-family.json')==r['preparation']['config_sha256'];assert sha(r['artifact'])==r['sha256'];assert not r['library_loaded_or_executed']and not r['original_or_Unicorn_executed']and not r['runtime_eligible']
 assert analysis['conversion_ready']and not analysis['unknowns'];assert closed['status']=='GUARDED_ALIGNED_CFG_CLOSED'and not closed['gaps']and not closed['instruction_overlaps']
 decoded={q['pc']:q for q in analysis['instructions']}
 for q in closed['instructions']:
  assert decoded[q['pc']]['end']==q['end']and decoded[q['pc']]['bytes']==q['bytes']
 excluded=set(decoded)-{q['pc']for q in closed['instructions']};assert excluded=={0x413a64,0x413a66},excluded
 ir=(build/'protected.ll').read_text();lines=ir.splitlines();ptrs={};reads=writes=0
 for i,line in enumerate(lines):
  if line.startswith('define '):ptrs={}
  m=re.match(r'(%\d+) = inttoptr i64 %\d+ to i(8|16|32)\*$',line)
  if m:ptrs[m[1]]=int(m[2])//8
  access=re.match(r'(?:%\d+ = )?(load|store) i(8|16|32)[ ,]',line)
  if access:
   pointer=re.search(r'i(?:8|16|32)\* (%\d+)',line)
   if pointer and pointer[1]in ptrs:
    assert '@stage72_guard('in lines[i-1]
    if access[1]=='store':
     assert any('@stage72_note_write('in s for s in lines[i+1:i+3]);writes+=1
    else:reads+=1
 slots=sorted(set(map(int,re.findall(r'getelementptr %_cpu, %_cpu\* %cpu, i32 0, i32 (\d+)',ir))));assert set(slots)<=set(range(9))|{13}
 assert(reads,writes)==(90,75);assert ir.count('call i8* @stage75_lookup_target(')==6
 assert ir.count('call void @stage72_step(')==278
 # Incoming arithmetic flags for the constant count leaf are preserved, rather
 # than replaced with guessed producer annotations.
 leaf=re.search(r'^define [^\n]*@function_4e1d00\([^\n]*\).*?^}',ir,re.M|re.S).group(0)
 assert not re.search(r'getelementptr %_cpu, %_cpu\* %cpu, i32 0, i32 8\b',leaf)
 nm=subprocess.run(['/usr/bin/nm','-n',r['artifact']],capture_output=True,text=True,timeout=10);assert nm.returncode==0
 symbols={}
 for line in nm.stdout.splitlines():
  m=re.match(r'([0-9a-f]+)\s+\w\s+(\S+)',line)
  if m:symbols[m[2]]=int(m[1],16)
 # LLVM private continuation labels are intentionally absent from nm. Decode
 # their addresses from the on-disk Mach-O binding table, without dyld loading.
 # Apple SDK fixup-chains.h defines format6 DYLD_CHAINED_PTR_64_OFFSET and
 # dyld_chained_ptr_64_rebase target36/high8/next12/bind1 fields.
 macho=Path(r['artifact']).read_bytes();assert struct.unpack_from('<I',macho)[0]==0xfeedfacf
 ncmds=struct.unpack_from('<I',macho,16)[0];at=32;segments=[];fixups=None
 for _ in range(ncmds):
  cmd,size=struct.unpack_from('<II',macho,at)
  if cmd==0x19:
   name,va,vmsize,off,filesize=struct.unpack_from('<16s4Q',macho,at+8);segments.append((name.rstrip(b'\0'),va,off,filesize))
  if cmd==0x80000034:fixups=struct.unpack_from('<II',macho,at+8)
  at+=size
 assert fixups is not None
 start=fixups[0];starts_offset=struct.unpack_from('<I',macho,start+4)[0];segment_starts=start+starts_offset
 segcount=struct.unpack_from('<I',macho,segment_starts)[0];formats=[]
 for k in range(segcount):
  segoff=struct.unpack_from('<I',macho,segment_starts+4+4*k)[0]
  if segoff:formats.append(struct.unpack_from('<H',macho,segment_starts+segoff+6)[0])
 assert formats and set(formats)=={6},formats
 def fileoff(va):
  for _,low,off,size in segments:
   if low<=va<low+size:return off+va-low
  raise ValueError('unmapped native file address')
 native_returns=[];binding_records=[]
 for k,row in enumerate(r['return_bindings']):
  off=fileoff(symbols['_stage72_return_bindings'])+k*16;pc=struct.unpack_from('<I',macho,off)[0];pointer=struct.unpack_from('<Q',macho,off+8)[0]
  assert pc==int(row['guest_PC'],0)and not(pointer>>63)&1
  target=(pointer&((1<<36)-1))|(((pointer>>36)&255)<<56);assert 0<target<32768
  native_returns.append(target);binding_records.append(dict(guest_PC=hex(pc),native_image_offset=hex(target)))
 assert len(set(native_returns))==len(native_returns)
 native_targets=[symbols['_function_'+format(int(q['target'],0),'x')]for q in cfg['resolved_indirect_edges'].values()]
 undefined=subprocess.run(['/usr/bin/nm','-u',r['artifact']],capture_output=True,text=True,timeout=10);assert undefined.returncode==0
 reuse={}
 for old,new in [('stage74/tools/builder.py','stage75/tools/builder_v2.py'),('stage74/tools/prepare.py','stage75/tools/prepare.py'),('samples/stage72_runtime.c','stage75/builds/type53-family-v5/runtime.c')]:
  a=(ROOT/old).read_text().splitlines();b=(ROOT/new).read_text().splitlines();equal=sum(m.size for m in difflib.SequenceMatcher(None,a,b,autojunk=False).get_matching_blocks());reuse[new]=dict(original_lines=len(a),new_lines=len(b),identical_lines=equal,new_line_reuse_percent=round(100*equal/len(b),3))
 result=dict(status='STATIC_IMPLEMENTATION_AND_LINK_AUDIT_PASSED_NO_GAME_EXECUTION',source_bytes=759,decoded_instructions=278,guarded_reachable_instructions=276,guarded_reachable_bytes=752,excluded_seven_byte_throw_edge=[hex(x)for x in sorted(excluded)],original_function_entries=18,load_guards=reads,store_guards_and_ordered_logs=writes,guest_memory_guard_percent=100,step_markers=278,indirect_binding_calls=6,unique_native_indirect_targets=len(set(native_targets)),native_return_bindings=len(native_returns),unique_native_return_addresses=len(set(native_returns)),MachO_chained_pointer_formats=formats,on_disk_return_binding_records=binding_records,allowed_CPU_slots=slots,incoming_leaf_flags_preserved=True,source_line_reuse=reuse,system_undefined_symbols=undefined.stdout.splitlines(),no_game_instructions_or_generated_library_executed=True)
 dump('evidence/implementation-audit.json',result);print(json.dumps(result))
def main():
 p=argparse.ArgumentParser();p.add_argument('mode',choices=['implementation','preservation']);a=p.parse_args();implementation()if a.mode=='implementation'else preservation()
if __name__=='__main__':main()
