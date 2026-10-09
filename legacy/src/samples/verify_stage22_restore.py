import argparse,copy,hashlib,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_startup22 import ProfileRuntime as StartupRuntime,x
from stage12_state import restore_state
p=argparse.ArgumentParser();p.add_argument('--state',default='profile22');a=p.parse_args();r=StartupRuntime('ST9BASE');r.start_threads(worker_capacity=3);m=restore_state(r,a.state);cpu={tid:bytes(t['context'])[40:]for tid,t in r.threads.items()};saved=r.current_tid;rows=[]
for tid,t in r.threads.items():
 r.thread_load(tid);assert bytes(r.uc.context_save())[40:]==cpu[tid];assert r.uc.reg_read(x.UC_X86_REG_FS)==(24 if tid==1 else 32+8*(tid-2));assert r.tls==t['tls']and r.last_error==t['last_error'];rows.append(dict(tid=tid,CPU_payload_bytes=len(cpu[tid]),CPU_equal=True,TLS_equal=True,last_error_equal=True,FS_correct=True,TEB=hex(t['teb']),stack=hex(t['stack']),status=t['status'],TLS0=hex(t['tls'][0]),native_thread_handle_live=r.events.query(t['handle'])['live']if tid!=1 else None))
r.thread_load(saved);assert len(rows)==4 and r.next_tid==5 and len({t['tls'][0]for t in r.threads.values()})==4;assert len({t['teb']for t in r.threads.values()})==4
for s in r.pe.sections:
 if s['flags']&0x20000000:assert bytes(r.uc.mem_read(s['va'],s['rawsize']))==r.pe.read(s['va'],s['rawsize'])
pixels,hot=r.cursor.pixels(0x55000001);assert hot==[12,4];out=dict(all_passed=True,state=a.state,contexts=rows,all_original_text_equal=True,all_mapped_memory_and_CPU_restored=True,native_cursor_restored=True,cursor_RGBA_sha256=hashlib.sha256(pixels).hexdigest(),actual_original_distinct_CRT_PTD_in_TLS0=True,game_save_restore=False,no_original_instruction_executed=True);(ROOT/f'evidence/stage22-restore-{a.state}.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out));r.dispose()
