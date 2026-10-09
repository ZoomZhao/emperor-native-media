#!/usr/bin/env python3
"""Actual original loading checkpoints, never a complete-chain pass claim."""
import json,struct,sys,hashlib,zlib
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from stage7_runtime import Runtime,ROOT,SCRATCH,GUEST_NAME,TEB
mode=sys.argv[1] if len(sys.argv)>1 else 'archive'
tag=sys.argv[2] if len(sys.argv)>2 else ''
assert not tag or tag.replace('-','').isalnum()
r=Runtime();steps=[]
def step(label,entry,args=(),this=0):
    item=r.call(entry,args,this);item['label']=label;steps.append(item)
    return item['status']=='returned'
r.uc.mem_write(SCRATCH,GUEST_NAME+b'\0')
if step('original_CRT_heap_initialization',0x76db30,(1,)) and (not tag or step('original_CRT_thread_and_lock_initialization',0x76c7fc)) and (not tag or 'io' not in tag or step('original_CRT_IO_initialization',0x76efc1)) and (not tag or step('original_CRT_initializer_table',0x769f80)):
    if mode=='archive':
        obj=SCRATCH+0x1000
        if step('original_archive_constructor',0x447b70,this=obj):
            if step('original_archive_open_and_first_compressed_block',0x447c90,(SCRATCH,0,0),obj):
                n=r.readword(obj+0x11a88)
                actual=bytes(r.uc.mem_read(obj+0x28,min(n,0x8d30)))
                b=(ROOT/'stage7-data/save-input.sav').read_bytes();compressed_size=struct.unpack_from('<I',b,8)[0]
                expected=zlib.decompress(b[16:16+compressed_size])
                steps[-1]['first_block_comparison']=dict(output_bytes=n,expected_bytes=len(expected),equal=actual==expected,actual_sha256=hashlib.sha256(actual).hexdigest(),expected_sha256=hashlib.sha256(expected).hexdigest())
                step('original_archive_destructor',0x447c10,this=obj)
    elif mode=='crt-city':
        step('original_city_load_root',0x534a30,(SCRATCH,1))
    else:raise ValueError(mode)
result=dict(mode=mode,steps=steps,original_instruction_execution=True,source_code_patched=False,
            platform_profile='bounded single-thread Windows XP ABI subset; not independently validated Windows',
            full_game_entry_executed=False,gate_or_phase_overridden=False,city_objects_fabricated=False,
            unimplemented_APIs_refuse=True,full_city_loaded=False,full_scheduler_cycle_executed=False,
            original_save_restored=False,arm64_chain_compared=False,complete_chain_passed=False,
            fs_base_expected=hex(TEB),unique_executed_addresses=len(r.visited),
            unique_original_addresses=sum(1 for address in r.visited if r.pe.section(address) and r.pe.section(address)['flags']&0x20000000),
            instruction_metric='instructions counts code-hook events, including imported API thunk entry',total_write_events=r.writes,
            platform_wall_clock_ns=r.wall_clock_ns,platform_monotonic_origin_ns=r.monotonic_origin_ns,
            original_input_sha256=hashlib.sha256((ROOT/'stage7-data/save-input.sav').read_bytes()).hexdigest())
path=ROOT/f'evidence/stage7-checkpoint-{mode}{"-"+tag if tag else ""}.json';path.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))
# A refusal is a checkpoint result, not a claim that the complete chain passed.
