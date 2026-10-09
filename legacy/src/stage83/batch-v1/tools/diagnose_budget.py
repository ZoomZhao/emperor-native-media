"""Read-only original PE/checkpoint analysis with installed Apple objdump."""
import json,re,struct,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
from common import ROOT,S,V,sha,put
def main():
 c=json.loads((V/'configs/round01.json').read_text());source=ROOT/c['source'];s=json.loads(source.read_text());r=json.loads((S/'runs/batch-v1-round01/capture/result.json').read_text());cpu=r['initial_CPU']
 assert all(cpu[k.upper()]==v for k,v in s['registers'].items())
 assert cpu['EIP']==0x419035 and cpu['EDI']==418 and cpu['EBP']==1024
 slot=cpu['ESP']+36;rows=None
 with(source.parent/'memory.bin').open('rb')as f:
  for reg in s['regions']:
   if reg['low']<=slot and slot+4<=reg['high']+1:f.seek(reg['offset']+slot-reg['low']);rows=struct.unpack('<I',f.read(4))[0];break
 assert rows==389
 cmd=['/usr/bin/objdump','--disassemble','--start-address=0x419027','--stop-address=0x419068',c['original_EXE']];q=subprocess.run(cmd,capture_output=True,timeout=15);assert q.returncode==0
 text=q.stdout.decode();pcs=[int(m.group(1),16)for line in text.splitlines()if(m:=re.match(r'\s*([0-9a-f]+):\s',line))]
 full=[p for p in pcs if 0x41902d<=p<=0x419054];partial=[p for p in pcs if 0x419035<=p<=0x419054];assert len(full)==19 and len(partial)==16
 remaining_pixels=cpu['EDI']+(rows-1)*cpu['EBP'];lower=len(partial)+(remaining_pixels-1)*len(full)
 assert lower==7556867 and lower>c['limits']['capture']['steps']
 (V/'evidence/original-loop.disassembly.txt').write_text(text)
 put(V/'evidence/capture-budget-analysis.json',{'status':'FIXED_INPUT_EXCEEDS_CAPTURE_INSTRUCTION_CAP','method':'Static lower bound from exact original PE loop and unchanged initial checkpoint; no new emulation','source_sha256':sha(source),'memory_sha256':sha(source.parent/'memory.bin'),'EXE_sha256':sha(Path(c['original_EXE'])),'initial_PC':cpu['EIP'],'initial_inner_remaining':cpu['EDI'],'initial_width':cpu['EBP'],'outer_rows_stack_slot':slot,'initial_outer_rows':rows,'remaining_pixels':remaining_pixels,'full_pixel_loop_instructions':len(full),'initial_partial_pixel_instructions':len(partial),'minimum_instructions_before_current_function_can_return':lower,'ignored_outer_loop_and_return_overhead':True,'capture_instruction_cap':c['limits']['capture']['steps'],'observed_attempt_steps':r['steps'],'observed_attempt_failure':r['failure'],'phase_entry_not_reached':True,'speed_only_repair_cannot_satisfy_instruction_cap':True,'input_or_budget_change_required':True,'new_original_instructions':0,'objdump_command':cmd})
 print(json.dumps({'lower_bound':lower,'capture_cap':6000000,'further_original_attempts_started':0}))
if __name__=='__main__':main()
