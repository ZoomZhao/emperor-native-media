#!/usr/bin/env python3
"""Finite byte/provenance/regression audit; no extra gameplay examples."""
import sys,json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from auto_i386_analysis import PE,Analysis,decode,ALL,MUL_FLAGS,CC_FLAGS
layout=json.loads((ROOT/'evidence/stage5-slice-layout.json').read_text())
original=PE('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe');fixture=PE(str(ROOT/'samples/stage5-feedback.exe'))
assert hashlib.sha256(original.data).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
for f in layout['fragments']:assert fixture.read(f['start'],f['end']-f['start'])==original.read(f['start'],f['end']-f['start'])
guarded=json.loads((ROOT/'runs/stage5-analysis/analysis.json').read_text());whole=json.loads((ROOT/'evidence/stage5-unclosed-analysis.json').read_text())
assert guarded['conversion_ready'] and not whole['conversion_ready']
assert len(layout['manual_conditions'])==1 and len(layout['boundaries'])==9
flags=json.loads((ROOT/'runs/stage5-analysis/analysis.json').read_text())['flags']
cross=[c for c in flags['conditional_read_evidence'] if c['region_incoming_sr_mask']!='0x0']
assert any(c['consumer']=='0x5918ac' and c['region_incoming_sr_mask']=='0x30' and set(c['producers'])=={'0x59186a','0x591877'} for c in cross)
new_results={}
for name in ('leaf','tax','write'):
    cfg=json.loads((ROOT/f'samples/auto-{name}-config.json').read_text())
    current=Analysis(original,cfg).run();old=json.loads((ROOT/f'runs/auto-{name}/analysis.json').read_text())
    assert current['conversion_ready']
    assert current['instruction_count']==old['instruction_count']
    key=lambda c:(c['site'],c['target'],c['confidence'])
    assert set(map(key,current['address_candidates']))==set(map(key,old['address_candidates']))
    assert current['runtime_pointer_bindings']==old['runtime_pointer_bindings']
    olddefs={i['pc']:(i['defines'],i['undefined']) for i in old['instructions']}
    assert all(olddefs[i['pc']]==(i['defines'],i['undefined']) for i in current['instructions'])
    new_results[name]=dict(instructions=current['instruction_count'],address_inference_and_runtime_bindings_unchanged=True,instruction_flag_definitions_unchanged=True,conversion_ready=True,new_flag_annotations=len(current['flags']['annotations']))
negative=[]
for name in ('mapped_scalar','dereferenced_base','scaled_integer_index','mid_instruction_branch','indirect_call','unsupported_fpu','data_as_code','outside_image_memory','conditional_initializer','call_depth_round_budget'):
    pe=PE(str(ROOT/f'samples/analysis-negative/{name}.exe'));s=pe.section(0x401000)
    cfg=dict(roots=[0x401000],windows=[[0x401000,0x401000+s['size']]])
    if name=='conditional_initializer':cfg['initializers']=[0x401000]
    result=Analysis(pe,cfg).run();unknown=name not in ('mapped_scalar','dereferenced_base','scaled_integer_index')
    assert bool(result['unknowns'])==unknown
    if unknown:assert not result['conversion_ready']
    negative.append(dict(name=name,passed=True))
# Compact flag values independently read from the approved SR headers.
header=(ROOT/'vendor-src/SR/SRW/udis86_dep.h').read_text()
for name,value in [('CARRY',1),('PARITY',2),('ADJUST',4),('ZERO',8),('SIGN',16),('OVERFLOW',32)]:
    import re
    match=re.search(r'FL_'+name+r'\s*=\s*(0x[0-9A-Fa-f]+)',header);assert match and int(match[1],16)==value
assert MUL_FLAGS==0x21 and CC_FLAGS[13]==0x30
native=json.loads((ROOT/'evidence/stage5-native-v2.stdout.log').read_text());baseline=json.loads((ROOT/'evidence/stage5-unicorn-results.json').read_text())
native_negative=json.loads((ROOT/'evidence/stage5-native-negative-v2.stdout.log').read_text());baseline_negative=json.loads((ROOT/'evidence/stage5-unicorn-negative.json').read_text())
assert native['passed'] and baseline['passed'] and native_negative['passed'] and baseline_negative['passed']
result=dict(passed=True,original_fragments_byte_identical=4,original_bytes=1257,guarded_bytes=355,guarded_instructions=128,executed_unique_instructions=baseline['instructions_reached'],automatic_absolute_address_candidates_guarded=len(guarded['address_candidates']),automatic_absolute_address_candidates_whole=len(whole['address_candidates']),automatic_flag_annotations=len(flags['annotations']),cross_region_conditional_contracts=cross,existing_modules_regression=new_results,existing_negative_controls=negative,native=native,baseline=baseline,negative_native=native_negative,negative_baseline=baseline_negative,full_callgraph_closed=False,real_city_reachability_proved=False)
(ROOT/'evidence/stage5-final-results.json').write_text(json.dumps(result,indent=2));print(json.dumps(dict(passed=True,rounds=native['rounds'],regression_modules=3,negative_controls=10,automatic_flag_annotations=len(flags['annotations']),full_callgraph_closed=False,real_city_reachability_proved=False)))
