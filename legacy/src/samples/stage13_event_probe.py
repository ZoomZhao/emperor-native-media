#!/usr/bin/env python3
"""Task-authored PE event matrix; same already approved Wine, no game code."""
import argparse,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage10_probe_support import Data,build,run
NAME='EVENT13'

def make():
    d=Data();d.alloc(b'EV13v001');ops=[];cases=[];groups=[]
    def add(name,args,label,group,expect=None):
        result=d.alloc(bytes(8));op=dict(name=name,args=args,result=result);ops.append(op)
        cases.append(dict(name=name,args=args,label=label,group=group,result_offset=d.offset(result),expect=expect));return result
    for manual,initial in [(0,0),(0,1),(1,0),(1,1),(2,0),(0,0xffffffff)]:
        group=f'event-{manual}-{initial}';h=add('CreateEventA',[0,manual,initial,0],'create',group);handle={'load':h};groups.append(group)
        add('WaitForSingleObject',[handle,0],'initial-first',group,0 if initial else 258)
        add('WaitForSingleObject',[handle,0],'initial-second',group,0 if initial and manual else 258)
        add('SetEvent',[handle],'set-first',group,1);add('SetEvent',[handle],'set-again',group,1)
        add('WaitForSingleObject',[handle,0],'set-first-wait',group,0)
        add('WaitForSingleObject',[handle,0],'set-second-wait',group,0 if manual else 258)
        add('ResetEvent',[handle],'reset-first',group,1);add('ResetEvent',[handle],'reset-again',group,1)
        add('WaitForSingleObject',[handle,0],'reset-wait',group,258)
        add('CloseHandle',[handle],'close',group,1)
        for n,args,value in [('WaitForSingleObject',[handle,0],0xffffffff),('SetEvent',[handle],0),('ResetEvent',[handle],0),('CloseHandle',[handle],0)]:add(n,args,'closed-'+n,group,value)
    for invalid in [0,0x7f12345]:
        for n,args,value in [('WaitForSingleObject',[invalid,0],0xffffffff),('SetEvent',[invalid],0),('ResetEvent',[invalid],0),('CloseHandle',[invalid],0)]:add(n,args,f'invalid-{invalid:x}',f'invalid-{invalid:x}',value)
    # Wrong object type for event mutation. Named/security objects stay outside
    # the game adapter's accepted scope until an actual consumer requests them.
    mutex=add('CreateMutexA',[0,0,0],'mutex-create','wrong-type');mh={'load':mutex}
    add('SetEvent',[mh],'mutex-set','wrong-type',0);add('ResetEvent',[mh],'mutex-reset','wrong-type',0);add('CloseHandle',[mh],'mutex-close','wrong-type',1)
    path=d.alloc(b'NUL\0');file=add('CreateFileA',[path,0x80000000,3,0,3,0x80,0],'file-create','wrong-type');fh={'load':file}
    add('SetEvent',[fh],'file-set','wrong-type',0);add('ResetEvent',[fh],'file-reset','wrong-type',0);add('CloseHandle',[fh],'file-close','wrong-type',1)
    h=add('CreateEventA',[0,0,0,0],'timed-create','timed');hh={'load':h}
    add('GetTickCount',[],'before-ticks','timed');add('WaitForSingleObject',[hh,30],'wait-30ms','timed',258);add('GetTickCount',[],'after-ticks','timed');add('CloseHandle',[hh],'timed-close','timed',1)
    # Non-event pseudo process handle is a different valid waitable type.
    add('WaitForSingleObject',[0xffffffff,0],'current-process-wait','pseudo-process',258)
    add('SetEvent',[0xffffffff],'current-process-set','pseudo-process',0);add('ResetEvent',[0xffffffff],'current-process-reset','pseudo-process',0)
    meta=build(NAME,d,ops);(ROOT/'evidence/stage13-event-cases.json').write_text(json.dumps(dict(cases=cases,groups=groups),indent=2)+'\n');print(json.dumps(dict(sha256=meta['sha256'],case_count=len(cases))))

def capture():
    result=run(NAME);layout=json.loads((ROOT/'evidence/stage13-event-cases.json').read_text());raw=(ROOT/f'evidence/stage10-{NAME}-output.bin').read_bytes();rows=[]
    for case in layout['cases']:
        value,error=struct.unpack_from('<II',raw,case['result_offset']);row=dict(case,return_value=value,last_error=error)
        if case['expect'] is not None:assert value==case['expect'],row
        elif case['name'].startswith('Create'):assert value not in (0,0xffffffff),row
        rows.append(row)
    ticks={r['label']:r['return_value'] for r in rows if r['group']=='timed' and r['name']=='GetTickCount'}
    result.update(all_returns_match_expected=True,case_count=len(rows),rows=rows,wait30_tick_delta=(ticks['after-ticks']-ticks['before-ticks'])&0xffffffff,all_code_self_authored=True,original_game_not_loaded=True)
    (ROOT/'evidence/stage13-event-Wine-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='rows'}))

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('action',choices=['build','run']);a=p.parse_args();make() if a.action=='build' else capture()
