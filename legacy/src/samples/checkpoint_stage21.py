#!/usr/bin/env python3
import argparse,json,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from checkpoint_stage20 import ThreadsRuntime,x
from stage12_state import restore_state,save_state
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',default='threads20');p.add_argument('--tag',default='threads21');a=p.parse_args();started=time.monotonic();m=json.loads((ROOT/f'runs/stage12-state-{a.resume}/state.json').read_text());capacity=next((v for k,v in m['host']['dict']if k=='worker_capacity'),2);r=ThreadsRuntime('ST9BASE');r.start_threads(worker_capacity=capacity);prior=restore_state(r,a.resume)
 if capacity==2:r.extend_to_reviewed_three_workers()
 if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
 s=r.thread_run(35);save_state(r,a.tag,'bounded original WinMain with reviewed fixed thread group',prior['milestones']);out=dict(source=a.resume,state=a.tag,elapsed_seconds=round(time.monotonic()-started,3),result=s,boot_events=r.boot_events,thread_log=r.thread_log,threads=r.thread_summary(),thread_switches=r.thread_switches,thread_waits=r.thread_waits,worker_capacity=r.worker_capacity,city_root_returned=False,native_city_passed=False,city_scheduler_cycles=0,original_code_patched=False,game_fields_filled=False,population=r.readword(0x130f988),treasury=r.readword(0x130f970));(ROOT/f'evidence/stage21-{a.tag}.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps({k:v for k,v in out.items()if k not in ['threads','thread_log']},ensure_ascii=False));r.dispose()
if __name__=='__main__':main()
