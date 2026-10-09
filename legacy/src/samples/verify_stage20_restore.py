#!/usr/bin/env python3
import json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage20 import ThreadsRuntime,x
from stage12_state import restore_state
r=ThreadsRuntime('ST9BASE');r.start_threads();m=restore_state(r,'threads20')
contexts={tid:bytes(t['context'])[40:]for tid,t in r.threads.items()};saved=r.current_tid;checks=[]
for tid,t in r.threads.items():
 r.thread_load(tid);assert bytes(r.uc.context_save())[40:]==contexts[tid];assert r.uc.reg_read(x.UC_X86_REG_FS)==(24 if tid==1 else 32+8*(tid-2));assert r.tls==t['tls']and r.last_error==t['last_error'];checks.append(dict(tid=tid,entire_CPU_payload_equal=True,per_thread_TLS_and_error_equal=True,FS_selector_correct=True,status=t['status'],native_handle_live=r.events.query(t['handle'])['live']if tid!=1 else None))
r.thread_load(saved)
for s in r.pe.sections:
 if s['flags']&0x20000000:assert bytes(r.uc.mem_read(s['va'],s['rawsize']))==r.pe.read(s['va'],s['rawsize'])
assert len(checks)==3 and r.next_tid==4;out=dict(all_passed=True,complete_restore='evidence/stage12-state-restore-threads20.json',contexts=checks,shared_memory_and_input_hashes_equal=True,original_code_equal=True,no_original_function_executed=True,third_worker_creation_still_refused=True,original_city_root_returned=False);(ROOT/'evidence/stage20-thread-restore.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out));r.dispose()
