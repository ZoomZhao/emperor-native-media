#!/usr/bin/env python3
"""Separate whole-callgraph refusal from a manually guarded path contract."""
import json,hashlib,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from auto_i386_analysis import PE,Analysis,decode
ROOT=Path(__file__).resolve().parents[1]
SOURCE='<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe'
pe=PE(SOURCE);assert hashlib.sha256(pe.data).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
windows=[[0x5917e0,0x5918f9],[0x59a1b0,0x59a1c5],[0x43b860,0x43b887],[0x548340,0x5486d4]]
cfg=dict(roots=['0x5917e0'],windows=windows)
whole=Analysis(pe,cfg).run();(ROOT/'evidence/stage5-unclosed-analysis.json').write_text(json.dumps(whole,indent=2))
assert not whole['conversion_ready'],'Unresolved notification dependencies must not be silently accepted'
class Guarded(Analysis):
    def successors(self,ins):
        if ins['pc']==0x548347:
            # Reviewed incoming memory condition. No invented branch semantics.
            prev=decode(self.pe,0x548340)
            assert prev['end']==ins['pc'] and prev['name']=='cmp'
            m,v=prev['args'];assert m['kind']=='mem' and m['base'] is None and m['index'] is None and m['disp']==0x88ec38 and m['bits']==32
            assert v['value']==1 and ins['control']=='jcc' and ins['args'][0]['value']==4 and ins['target']==0x5486d3
            return [ins['target']]
        return super().successors(ins)
guarded=Guarded(pe,cfg).run();assert guarded['conversion_ready'],guarded['unknowns']
# Prove the contract cannot be changed within this guarded slice. Dynamic stores
# are accepted only for the dedicated ESP stack domain; no heap alias is assumed.
stores=[]
for ins in guarded['instructions']:
    if ins['name'] in ('mov','add','sub','inc','dec','or','and','xor','neg','setcc') and ins['args'] and ins['args'][0]['kind']=='mem':
        m=ins['args'][0]
        assert (m['base']==4 and m['index'] is None) or (m['base'] is None and m['index'] is None and m['disp']!=0x88ec38),'Unknown alias to guarded global'
        stores.append(dict(pc=hex(ins['pc']),operand=m))
guarded['manual_input_contract']=dict(address='0x88ec38',value=1,read_only_during_slice=True,branch='0x548347',required_guest_esp_domain='0x70000000..0x7000ffff',source_assignment='0x42e508',real_game_reachability_and_meaning='original assignment exists; dynamic reachability from a real city/scheduler state is unproved',negative_input_policy='reject before guest execution, never skip notification',store_alias_proof=stores)
guarded['whole_game_callgraph_closed']=False
out=ROOT/'runs/stage5-analysis';out.mkdir(parents=True,exist_ok=True)
(out/'analysis.json').write_text(json.dumps(guarded,indent=2))
(out/'instruction_flags.sci').write_text(''.join(f'loc_{int(a["site"],16):X},{a["sr_mask"]},{a["sr_input_mask"]}\n' for a in guarded['flags']['annotations']))
print(json.dumps(dict(whole_instructions=whole['instruction_count'],whole_unknowns=whole['unknowns'],guarded_instructions=guarded['instruction_count'],guarded_address_candidates=len(guarded['address_candidates']),guarded_flag_annotations=guarded['flags']['annotations'],manual_contracts=1,whole_callgraph_closed=False)))
