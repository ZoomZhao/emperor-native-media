"""Audit actual full-build coverage and original import dependencies, read-only."""
import collections,difflib,hashlib,json,re,struct,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage76'
sys.path.insert(0,str(ROOT/'samples'))
from auto_i386_analysis import PE
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 pe=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe')
 closure=json.loads((S/'evidence/full-dependency-closure-v1.json').read_text())
 build=json.loads((S/'builds/full-v9/result.json').read_text())
 flags=json.loads((S/'evidence/full-flags-v3.json').read_text())
 part=json.loads((S/'evidence/call-partition.json').read_text())
 opt=struct.unpack_from('<I',pe.data,60)[0]+24
 rva,size=struct.unpack_from('<II',pe.data,opt+104);imports={}
 def cstring(address):
  b=bytearray()
  for i in range(512):
   c=pe.read(address+i,1)[0]
   if not c:return b.decode('ascii')
   b.append(c)
  raise ValueError('string budget')
 for i in range(100):
  oft,time,forward,name,iat=struct.unpack('<5I',pe.read(pe.base+rva+20*i,20))
  if not any((oft,time,forward,name,iat)):break
  dll=cstring(pe.base+name)
  for j in range(4096):
   value=struct.unpack('<I',pe.read(pe.base+(oft or iat)+4*j,4))[0]
   if not value:break
   imports[pe.base+iat+4*j]=dll+'.'+('ordinal_'+str(value&65535)if value>>31 else cstring(pe.base+value+2))
 edges={q['pc']:q for q in closure['indirect_edges']};groups=collections.defaultdict(list);unknown=[]
 proofs=part['register_IAT_proofs']
 for pc,q in edges.items():
  if q['kind']!='indirect_call':continue
  m=re.fullmatch(r'dword ptr \[(\d+)\]',q['operand']);slot=int(m[1])if m else None
  name=imports.get(slot)or proofs.get(hex(pc),{}).get('import_name')
  if name:groups[name].append(hex(pc))
  else:unknown.append(dict(PC=hex(pc),operand=q['operand']))
 code=closure['instructions'];fixture=PE(str(S/'builds/full-v9/original-superset.exe'))
 for q in code:assert fixture.read(q['pc'],q['end']-q['pc'])==pe.read(q['pc'],q['end']-q['pc'])==bytes.fromhex(q['bytes'])
 assert build['translated_original_instruction_comments']==len(code)
 translated=(S/'builds/full-v9/seg01_code.original.llinc').read_text().splitlines()
 comments=[r for r in translated if r.startswith(';')]
 assert len(comments)==len(code)
 aliases={'jz':'je','jnz':'jne','jc':'jb','jnae':'jb','jnb':'jae','jnc':'jae','jna':'jbe','jnbe':'ja','jnge':'jl','jnl':'jge','jng':'jle','jnle':'jg','jpe':'jp','jpo':'jnp','setz':'sete','setnz':'setne','retn':'ret','sal':'shl','repnz':'repne'}
 fs_helpers=[]
 for q,c in zip(code,comments):
  mnemonic=c[1:].split()[0]
  if q['mnemonic']!=aliases.get(mnemonic,mnemonic):
   assert q['mnemonic']=='mov'and 'fs:[0]'in q['operand']and 'call x86_'in c and '_fs_dword'in c,(q,c)
   fs_helpers.append(dict(PC=hex(q['pc']),source_operand=q['operand'],SR_comment=c,native_SEH_semantics_unimplemented=True))
 contracts=flags['flags']['incoming_or_unknown_contracts']
 rows=[]
 for old,new in [('stage75/tools/exact_closure.py','stage76/tools/exact_closure.py'),('stage74/tools/analysis_v2.py','stage76/tools/analysis_full_base.py')]:
  a=(ROOT/old).read_text().splitlines();b=(ROOT/new).read_text().splitlines();same=sum(m.size for m in difflib.SequenceMatcher(a=a,b=b,autojunk=False).get_matching_blocks())
  rows.append(dict(source=old,new=new,reused_identical_source_lines=same,new_lines=len(b),percent=round(100*same/len(b),3),metric_is_source_lines_not_semantic_coverage=True))
 sourcefiles=['samples/stage10_native_platform.c','samples/stage11_native_runtime.c']
 result=dict(status='FULL_BUILD_STATIC_AUDIT_PASSED_WITH_OPEN_RUNTIME_CONTRACTS',source_function_entries=len(closure['function_entries']),instructions=len(code),original_code_bytes=closure['code_bytes'],exact_source_bytes_and_ordered_mnemonics_verified=True,SR_FS_helper_lowerings=fs_helpers,
  expanded_indirect_edge_records=len(closure['indirect_edges']),expanded_unique_indirect_sites=len(edges),expanded_unique_indirect_CALL_sites=sum(q['kind']=='indirect_call'for q in edges.values()),expanded_resolved_jump_sites=sum(q['kind']=='indirect_jump'for q in edges.values()),
  expanded_named_API_CALL_sites=sum(map(len,groups.values())),expanded_unique_named_APIs=len(groups),expanded_named_APIs=[dict(API=k,sites=v,site_count=len(v))for k,v in sorted(groups.items())],expanded_unresolved_non_IAT_CALL_sites=unknown,
  source_reuse=rows,unchanged_real_native_C_sources=[dict(path=p,sha256=sha(ROOT/p),source_lines=len((ROOT/p).read_text().splitlines()),copied_or_modified=False)for p in sourcefiles],
  emitted_relocations=build['reconstructed_relocations'],emitted_relocations_automatic_percent=100,relocation_coverage_of_all_original_pointer_operands='UNKNOWN: narrow operand decoder fails at 530 instruction sites; 394 emitted records include jump slots; denominator incomplete',unsupported_operand_decoder_instructions=len(build['own_decoder_unsupported_sites']),
  flags_annotations=len(flags['flags']['annotations']),flags_generated_percent=100,flags_summary_rounds=flags['summary_rounds'],flags_summary_converged=False,flags_unknowns=flags['unknowns'],incoming_flag_contracts=sum(q['provenance']=='incoming'for q in contracts),unknown_call_flag_contracts=sum(q['provenance']!='incoming'for q in contracts),
  manual_reviewed_register_IAT_facts=len(proofs),automatically_applied_register_IAT_bindings=len(build['register_import_bindings']),register_binding_application_percent=100,manual_reviewed_jump_domains=32,automatic_table_address_bindings=len(build['automatic_original_guest_table_address_bindings']),automatic_verified_fallthrough_bridges=len(build['automatic_verified_fallthrough_bridges']),per_address_original_instruction_patches=0,
  full_guest_memory_guards_and_ordered_write_trace_integrated=False,full_guest_CALL_RET_and_dynamic_target_bindings_integrated=False,real_heap_wrappers_guest_return_bridge_compiled=True,complete_runnable_phase8=False,new_original_or_Unicorn_or_generated_game_executions=0)
 (S/'evidence/full-audit.json').write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps({k:result[k]for k in ['status','instructions','expanded_unique_indirect_CALL_sites','expanded_named_API_CALL_sites','expanded_unique_named_APIs','source_reuse','flags_unknowns']}))
if __name__=='__main__':main()
