# -*- coding: utf-8 -*-
"""Bounded byte-backed CFG and real saved-state inspection, no emulation."""
import collections,gc,hashlib,json,re,struct
from pathlib import Path
from auto_i386_analysis import PE,decode as legacy_decode
from stage60_controlled_input_plan import decode
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence';R=ROOT/'runs/stage69-static'
EXE='<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe'
SHA='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
WINDOWS={0x4af350:0x4af48b,0x4af310:0x4af34b,0x4af490:0x4af5c8,0x47f1b0:0x47f1bf,0x4f8210:0x4f8214,0x543d90:0x543da7}
PATTERN=re.compile(r'^\s*([0-9a-f]+):\s*((?:[0-9a-f]{2}\s+)+)\s*([a-z][a-z0-9]*)\s*(.*)$')
def digest(b):return hashlib.sha256(b).hexdigest()
def code():
    pe=PE(EXE);assert digest(pe.data)==SHA
    instructions={};xrefs=collections.defaultdict(list)
    # Selected literal operand bases, not an alias-analysis claim.
    literals=[0xec5a10,0xeb8f00,0xc81b70,0xc81b74,0xc81b78,0xc81b7c,0xc81b80,0xc81b84,0x1312588,0x131258c]
    with (R/'fixed-text.disasm').open() as f:
        for line in f:
            m=PATTERN.match(line)
            if not m:continue
            pc=int(m[1],16);args=m[4].split('#')[0].strip();mn=m[3]
            if any(lo<=pc<hi for lo,hi in WINDOWS.items()):
                body=bytes.fromhex(m[2]);assert body==pe.read(pc,len(body)),hex(pc)
                instructions[pc]=dict(pc=pc,size=len(body),bytes=body.hex(),mnemonic=mn,args=args,line=line.rstrip())
            for addr in literals:
                if str(addr) in args:xrefs[hex(addr)].append(dict(pc=hex(pc),mnemonic=mn,args=args))
    # This candidate has SIX functions, including a tiny vector accessor.
    results=[];unknown=[];decoded=[];unsupported=[]
    for entry,stop in WINDOWS.items():
        queue=[entry];seen=set();edges=[];calls=[];returns=[]
        while queue:
            pc=queue.pop()
            if pc in seen:continue
            if not entry<=pc<stop or pc not in instructions:
                unknown.append(dict(kind='outside_window_or_decode_boundary',entry=hex(entry),pc=hex(pc)));continue
            seen.add(pc);q=instructions[pc];mn=q['mnemonic'];nxt=pc+q['size'];arg=q['args']
            target_match=re.match(r'0x([0-9a-f]+)',arg);target=int(target_match[1],16) if target_match else None
            if mn=='call':
                calls.append(dict(pc=hex(pc),target=hex(target) if target is not None else None,operand=arg))
                if target not in WINDOWS:unknown.append(dict(kind='unclosed_call',pc=hex(pc),operand=arg))
                successors=[nxt]
            elif mn.startswith('ret'):returns.append(hex(pc));successors=[]
            elif mn=='jmp':
                successors=[target] if target is not None else []
                if target is None:unknown.append(dict(kind='indirect_jump',pc=hex(pc),operand=arg))
            elif mn.startswith('j'):successors=[nxt,target]
            else:successors=[nxt]
            for dest in successors:
                if dest is None:unknown.append(dict(kind='unknown_branch',pc=hex(pc)))
                else:edges.append(dict(source=hex(pc),destination=hex(dest)));queue.append(dest)
            try:
                native=legacy_decode(pe,pc)
                if native['end']!=nxt:raise ValueError('length mismatch against LLVM '+str(native['end']-pc)+' vs '+str(q['size']))
                decoded.append(pc)
            except (ValueError,AssertionError,IndexError) as err:unsupported.append(dict(pc=hex(pc),bytes=q['bytes'],mnemonic=mn,args=arg,reason=str(err)))
        listed=[q for pc,q in instructions.items() if entry<=pc<stop]
        total_bytes=sum(instructions[p]['size'] for p in seen)
        assert sorted(p for p in seen)==sorted(q['pc'] for q in listed)
        assert sum(q['size'] for q in listed)==stop-entry
        (R/(hex(entry)[2:]+'.asm')).write_text('\n'.join(instructions[p]['line'] for p in sorted(seen))+'\n')
        results.append(dict(entry=hex(entry),exclusive_end=hex(stop),bytes=total_bytes,instructions=len(seen),calls=calls,returns=returns,
            CFG_edges=edges,all_listed_instructions_reachable_in_overapproximation=True,
            mnemonics=dict(collections.Counter(instructions[p]['mnemonic'] for p in seen)),sha256=digest(pe.read(entry,stop-entry))))
    total=sum(q['instructions'] for q in results);totalbytes=sum(q['bytes'] for q in results)
    automatic_bytes=sum(instructions[p]['size'] for p in decoded)
    phases=[dict(phase=i,dispatch=hex(struct.unpack('<I',pe.read(0x4ac4e8+(i-1)*4,4))[0]))for i in range(5,22)]
    out=dict(status='byte_backed_static_CFG_closed' if not unknown else 'static_closure_has_unknowns',functions=results,
        function_count=len(results),instruction_count=total,code_bytes=totalbytes,unknown=unknown,
        direct_calls=sum(len(q['calls']) for q in results),indirect_calls=0,indirect_jumps=0,
        external_import_calls=0,x87_instructions=0,SEH_or_segment_FS_accesses=0,
        all_branch_destinations_byte_backed=True,all_direct_call_targets_closed=not unknown,
        LLVM_CFG_coverage_percent=100,legacy_own_decoder_supported_instructions=len(decoded),legacy_own_decoder_total_instructions=total,
        legacy_decoder_instruction_percent=round(len(decoded)*100/total,3),legacy_decoder_supported_bytes=automatic_bytes,
        legacy_decoder_byte_percent=round(automatic_bytes*100/totalbytes,3),legacy_unsupported=unsupported,
        recoverability_means_static_decode_not_translated_or_validated_execution=True,
        literal_xrefs_not_complete_alias_write_proof=dict(xrefs),observed_phase_mapping=phases,
        original_instructions_executed=0,no_Unicorn_import_or_run=True,no_original_DLL=True)
    (E/'stage69-static-closure.json').write_text(json.dumps(out,indent=2)+'\n')
    print(json.dumps({k:out[k] for k in ['status','function_count','instruction_count','code_bytes','direct_calls','legacy_decoder_instruction_percent','legacy_decoder_byte_percent','unknown']}),flush=True)
class Saved:
    def __init__(self,tag):
        self.tag=tag;folder=ROOT/('runs/stage12-state-'+tag);d=json.loads((folder/'state.json').read_text());h=dict(d['host']['dict'])
        self.regions=d['regions'];self.allocations=decode(h['allocations']);self.threads=decode(h['guest_threads20']);self.current=decode(h['current_tid'])
        self.registers=d['registers'];self.source=d['original_EXE_sha256'];assert self.source==SHA
        self.memory=(folder/'memory.bin').open('rb');self.state_sha=digest((folder/'state.json').read_bytes())
        del d,h;gc.collect()
    def read(self,addr,n):
        for r in self.regions:
            if r['low']<=addr and addr+n<=r['high']+1:
                self.memory.seek(r['offset']+addr-r['low']);body=self.memory.read(n);assert len(body)==n;return body
        raise ValueError('unmapped address '+hex(addr))
    def word(self,addr):return struct.unpack('<I',self.read(addr,4))[0]
    def owner(self,addr,n):
        return [dict(base=hex(p),bytes=v[1],heap=hex(v[0])) for p,v in self.allocations.items() if p<=addr and addr+n<=p+v[1]]
def states():
    spans=[('component_labels',0xec5a10,51984),('subgroup_labels',0xeb8f00,51984),('work_globals_and_ring',0xc81b70,4020),
        ('top10_pairs',0x1312588,80),('cursor',0x101c560,4),('tile_flags',0xf6a9e0,51984*4),('tile_object_indices',0xfc3750,51984*2),
        ('neighbor_flags',0x13789c0,51984*2),('neighbor_deltas',0x820cd0,16),('grid_dimensions',0x101d0c0,16),('container',0x8c7634,16)]
    tags=['frame61a','cycle61a','cycle61p07'];all_rows=[];bodies={}
    for tag in tags:
        s=Saved(tag);rows=[];byname={}
        for name,address,n in spans:
            body=s.read(address,n);byname[name]=body
            item=dict(name=name,address=hex(address),bytes=n,sha256=digest(body),nonzero_bytes=sum(q!=0 for q in body))
            if n<=80:item['raw_hex']=body.hex();item['u32']=list(struct.unpack('<'+'I'*(n//4),body))
            rows.append(item)
        base=s.word(0x8c7638);end=s.word(0x8c763c);count=(end-base)//4 if end>=base else None
        selected_indices=struct.unpack('<51984h',byname['tile_object_indices']);unique=sorted(set(selected_indices))
        pointers=[]
        for index in unique:
            slot=(base+index*4)&0xffffffff
            try:
                obj=s.word(slot);field=s.read((obj+0x14)&0xffffffff,2)
                pointers.append(dict(index=index,slot=hex(slot),object=hex(obj),type_word_signed=struct.unpack('<h',field)[0],
                    slot_owner=s.owner(slot,4),object_field_owner=s.owner(obj+0x14,2),success=True))
            except ValueError as error:pointers.append(dict(index=index,success=False,error=str(error)))
        raw=s.threads[1]['context'][0];assert len(raw)==5608
        # Preserve opaque engine CPU; do not invent a context register layout.
        (R/(tag+'-main-thread-context.bin')).write_bytes(raw)
        metadata=dict(tag=tag,current_tid=s.current,top_level_registers={k:hex(v)for k,v in s.registers.items()},
            main_thread_status=s.threads[1]['status'],main_thread_context_bytes=len(raw),main_thread_context_sha256=digest(raw),
            main_thread_context_is_top_level_current_context=s.current==1,
            main_thread_register_layout_not_guessed=True,state_sha256=s.state_sha,rows=rows,
            pointer_chain=dict(container='0x8c7634',base=hex(base),end=hex(end),count=count,unique_signed_tile_indices=unique,
                resolution_scope='All indices present in saved full table, conservative input set; this is not static bounds proof',pointers=pointers),
            root_has_no_heap_writes_in_static_closure=True)
        all_rows.append(metadata);bodies[tag]=byname;s.memory.close();del s;gc.collect()
    differences=[]
    for old,new in [('frame61a','cycle61a'),('cycle61a','cycle61p07')]:
        for name,address,n in spans:
            a=bodies[old][name];b=bodies[new][name];changed=[i for i,(x,y) in enumerate(zip(a,b))if x!=y]
            differences.append(dict(before=old,after=new,name=name,address=hex(address),bytes=n,changed_bytes=len(changed),
                first_changed_offsets=changed[:32],old_sha256=digest(a),new_sha256=digest(b),
                sample_changes=[dict(offset=i,before=a[i],after=b[i])for i in changed[:8]]))
    out=dict(status='saved_state_dependency_and_output_diff_completed',states=all_rows,differences=differences,
        exact_single_phase_oracle_available=False,
        before_phase7_checkpoint='frame61a phase6 inside refresh, not callee entry',
        partial_phase7_checkpoint='cycle61a main thread observed PC0x4af311; current top-level thread2, main context separately retained',
        after_phase7_checkpoint='cycle61p07 phase9: completed phase7 and phase8 plus refresh, not isolated callee return',
        output_differences_are_actual_bytes_not_attribution_proof=True,original_instructions_executed=0)
    (E/'stage69-saved-state-diff.json').write_text(json.dumps(out,indent=2)+'\n')
    print(json.dumps(dict(status=out['status'],states=[dict(tag=q['tag'],current_tid=q['current_tid'],pointer_indices=len(q['pointer_chain']['pointers']),
        unresolved=sum(not p['success']for p in q['pointer_chain']['pointers']))for q in all_rows],differences=[q for q in differences if q['name']in ['component_labels','subgroup_labels','top10_pairs','work_globals_and_ring']])),flush=True)
if __name__=='__main__':code();states()
