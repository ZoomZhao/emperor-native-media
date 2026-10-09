"""Actual existing own static scanner readiness; no decoder extension or CPU run."""
import json,struct
from pathlib import Path
from auto_i386_analysis import PE,Analysis
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence';R=ROOT/'runs/stage69-static'
EXE='<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe'
def main():
    proof=json.loads((E/'stage69-static-closure.json').read_text())
    config=dict(roots=['0x4af350'],windows=[[q['entry'],q['exclusive_end']]for q in proof['functions']])
    result=Analysis(PE(EXE),config).run()
    (E/'stage69-existing-decoder-pipeline.json').write_text(json.dumps(result,indent=2)+'\n')
    # The continuation stack locations are derived from the exact prologue and
    # verified saved bytes, not interpreted from opaque engine context offsets.
    d=json.loads((ROOT/'runs/stage12-state-cycle61a/state.json').read_text());regions=d['regions']
    with (ROOT/'runs/stage12-state-cycle61a/memory.bin').open('rb') as f:
        def read(a,n):
            for q in regions:
                if q['low']<=a and a+n<=q['high']+1:
                    f.seek(q['offset']+a-q['low']);b=f.read(n);assert len(b)==n;return b
            raise ValueError(hex(a))
        stack=struct.unpack('<10I',read(0x700ffd68,40));assert stack[1]==0x4af3cc and stack[8]==0x4ac333 and stack[9]==0x5371c4
        body=dict(pause_PC_observed='0x4af311',derived_pause_SP='0x700ffd68',
            stack_words=[dict(address=hex(0x700ffd68+i*4),value=hex(v))for i,v in enumerate(stack)],
            predicate_return='0x4af3cc',phase_callee_return='0x4ac333',scheduler_return='0x5371c4',
            stop_before='0x4ac333',derived_stop_SP='0x700ffd8c',pause_PC_has_already_executed_push_ESI=True,
            paused_tile_index=stack[2],saved_guest_stack_layout_matches_static_calling_sequence=True,
            CPU_registers_ES_base_DF_require_verified_engine_context_reader=True,opaque_context_layout_not_guessed=True)
    (E/'stage69-continuation-stack-contract.json').write_text(json.dumps(body,indent=2)+'\n')
    output=dict(existing_full_pipeline_conversion_ready=result['conversion_ready'],
        recovered_instruction_count=result['instruction_count'],complete_candidate_instruction_count=proof['instruction_count'],
        recovered_reachability_percent=round(result['instruction_count']*100/proof['instruction_count'],3),
        unknowns=result['unknowns'],constant_trace_stops=result['constant_trace_stops'],
        per_instruction_decoder_support_does_not_mean_whole_pipeline_ready=True,continuation=body,
        original_instructions_executed=0)
    (E/'stage69-existing-decoder-readiness.json').write_text(json.dumps(output,indent=2)+'\n');print(json.dumps(output),flush=True)
if __name__=='__main__':main()
