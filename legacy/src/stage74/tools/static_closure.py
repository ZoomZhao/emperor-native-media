"""CFG closure from fixed Apple LLVM disassembly, checked against original bytes.

It does not interpret/run x86. Indirect targets remain explicit unresolved edges.
"""
import argparse,hashlib,json,re,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'samples'))
from stage70_analysis import PE,Analysis
PATTERN=re.compile(r'^\s*([0-9a-f]+):\s*((?:[0-9a-f]{2}\s+)+)\s*([a-z][a-z0-9]*)\s*(.*)$')
def closure(pe,path,root,cap=4000):
 instructions={}
 for line in Path(path).open():
  m=PATTERN.match(line)
  if m:
   pc=int(m[1],16);b=bytes.fromhex(m[2]);instructions[pc]=dict(pc=pc,end=pc+len(b),bytes=b.hex(),mnemonic=m[3],operand=m[4].split('#')[0].strip())
 funcs={};queue=[root];allseen=set();unknown=[]
 while queue:
  entry=queue.pop()
  if entry in funcs:continue
  seen=set();todo=[entry];calls=[];returns=[];indirect=[]
  while todo:
   pc=todo.pop()
   if pc in seen:continue
   if len(allseen)>=cap:unknown.append(dict(kind='instruction_cap',PC=hex(pc)));break
   if pc not in instructions:unknown.append(dict(kind='missing_decode',PC=hex(pc)));continue
   q=instructions[pc];section=pe.section(pc)
   if not section or not section['flags']&0x20000000:unknown.append(dict(kind='outside_original_text',PC=hex(pc)));continue
   assert pe.read(pc,q['end']-pc)==bytes.fromhex(q['bytes'])
   seen.add(pc);allseen.add(pc);mn=q['mnemonic'];match=re.match(r'0x([0-9a-f]+)',q['operand']);target=int(match[1],16)if match else None
   if mn.startswith('ret'):returns.append(pc);successors=[]
   elif mn=='call':
    calls.append(dict(pc=pc,continuation=q['end'],target=target,operand=q['operand']))
    if target is not None:queue.append(target)
    else:indirect.append(dict(pc=pc,kind='call',operand=q['operand']))
    successors=[q['end']]
   elif mn=='jmp':
    successors=[target]if target is not None else []
    if target is None:indirect.append(dict(pc=pc,kind='jump',operand=q['operand']))
   elif mn.startswith('j'):successors=[q['end'],target]
   else:successors=[q['end']]
   todo.extend(p for p in successors if p is not None)
  funcs[entry]=dict(entry=entry,instructions=sorted(seen),direct_calls=[c for c in calls if c['target']is not None],indirect_edges=indirect,returns=returns)
 selected=[instructions[p]for p in sorted(allseen)];intervals=[]
 for q in selected:
  if intervals and intervals[-1][1]==q['pc']:intervals[-1][1]=q['end']
  else:intervals.append([q['pc'],q['end']])
 analysis=Analysis(pe,dict(roots=[hex(root)],windows=[[hex(a),hex(b)]for a,b in intervals])).run()
 return dict(status='CLOSED_DIRECT_CFG'if not unknown and not any(f['indirect_edges']for f in funcs.values())else 'OPEN_INDIRECT_CFG',
   root=root,functions=[funcs[p]for p in sorted(funcs)],instructions=selected,windows=intervals,
   code_bytes=sum(q['end']-q['pc']for q in selected),instruction_count=len(selected),
   unknown=unknown,indirect_edges=[x for f in funcs.values()for x in f['indirect_edges']],
   direct_calls=[x for f in funcs.values()for x in f['direct_calls']],own_analysis=analysis,
   runtime_executed=False,indirect_targets_guessed=False)
def main():
 p=argparse.ArgumentParser();p.add_argument('--root',required=True);p.add_argument('--output',required=True);a=p.parse_args()
 exe=Path('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe');pe=PE(str(exe))
 assert hashlib.sha256(pe.data).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
 result=closure(pe,ROOT/'runs/stage69-static/fixed-text.disasm',int(a.root,0));out=ROOT/a.output
 assert out.resolve().is_relative_to(ROOT/'stage74');out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(dict(status=result['status'],functions=len(result['functions']),instructions=result['instruction_count'],bytes=result['code_bytes'],indirect=len(result['indirect_edges']),analysis_instructions=result['own_analysis']['instruction_count'],analysis_unknowns=len(result['own_analysis']['unknowns']))))
if __name__=='__main__':main()
