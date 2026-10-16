#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Headless authored callbacks only: no window/capture/input/permission calls."""
import argparse
import copy
import json
from pathlib import Path
import subprocess
import tempfile
import uuid
from final_dispatch import FinalClickAttempt,NativeDryRunAdapter,FakeClickSender,ExistingCLIAdapter
from reference_compare_tests import FinalClickBindingTests


class Clock:
    def __init__(self,value=1400000000,late=False,fail=False):self.value=value;self.late=late;self.fail=fail
    def __call__(self):
        if self.fail:raise ValueError('authored missing clock')
        return self.value+300000000 if self.late else self.value


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--mode',choices=['release','undefined'],default='release')
    parser.add_argument('--output',type=Path);args=parser.parse_args()
    out=args.output if args.output else Path(tempfile.mkdtemp(prefix='final-dispatch-'))
    if not out.is_absolute():parser.error('absolute owned output directory required')
    out.mkdir(parents=True,exist_ok=True);source=Path(__file__).resolve().parent;library=out/'dry-final.dylib'
    (out/'result.json').write_text(json.dumps({'passed':False,'status':'running','mode':args.mode})+'\n')
    flags=['-O2']if args.mode=='release'else['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all']
    command=['/usr/bin/clang++','-dynamiclib','-std=c++17','-fobjc-arc','-mmacosx-version-min=15.0','-Wall','-Wextra','-Wpedantic','-Werror',*flags,str(source/'final_dispatch_bridge.mm'),'-o',str(library),'-framework','AppKit','-framework','ApplicationServices','-framework','ScreenCaptureKit']
    if args.mode=='undefined':
        # System Python refuses sanitizer dylibs. Use an ordinary owned executable;
        # do not weaken platform policy or claim the FFI path ran under UBSan.
        binary=out/'native-final-tests'
        native=[part for part in command if part!='-dynamiclib']
        native[native.index(str(library))]=str(binary)
        native.insert(native.index('-o'),str(source/'final_dispatch_native_tests.mm'))
        subprocess.run(native,check=True,capture_output=True,text=True,timeout=60)
        completed=subprocess.run([str(binary)],check=True,capture_output=True,text=True,timeout=10)
        report=json.loads(completed.stdout);(out/'result.json').write_text(json.dumps(report,indent=2)+'\n')
        print(json.dumps({k:v for k,v in report.items()if k!='cases'}));return
    subprocess.run(command,check=True,capture_output=True,text=True,timeout=60)
    results=[]
    def setup():
        a=FinalClickBindingTests().fixture();images=a[:4];frozen,contexts,journal=a[4:];clock=Clock()
        frozen['id']=str(uuid.uuid4());journal['freeze_id']=frozen['id']
        for context in contexts:context['freeze_id']=frozen['id']
        def provider():
            context=copy.deepcopy(contexts[-1]);context['monotonic_ns']=clock.value;return context
        return images,frozen,contexts,journal,clock,provider
    images,frozen,contexts,journal,clock,provider=setup();attempt=FinalClickAttempt(images,frozen,contexts,journal);sender=FakeClickSender();adapter=NativeDryRunAdapter(library)
    result=attempt.run(adapter,provider,sender,clock)
    assert result['passed']and result['gate_calls']==2 and result['validation_calls']==2 and result['sender_callback_calls']==1
    assert len(sender.events)==2 and sender.calls==1 and result['native_event_posts']==0
    assert all(event['point']==frozen['click_global']for event in sender.events)
    reused=attempt.run(adapter,provider,sender,clock);assert reused['native_calls_this_run']==0 and adapter.calls==1 and sender.calls==1
    clone=FinalClickAttempt(images,frozen,contexts,journal);duplicate=clone.run(adapter,provider,sender,clock)
    assert duplicate['status']=='refused_reused_frozen_trial'and adapter.calls==1 and sender.calls==1
    results.append({'case':'accepted_exact_once','passed':True,'native_calls':1,'gate_calls':2,'sender_calls':1,'fake_events':2,'reuse_calls':0})
    failures=('wrong_point','wrong_cell','expired','cancel_before','cancel_after_validation','camera_after_validation','pointer_after_validation','missing_evidence','old_passed_json','cancelled_frame','final_frame','native_focus','native_frame','native_owner','native_deadline','native_wrong_point','expires_after_validation','missing_clock','metadata_tamper','pixels_tamper')
    for kind in failures:
        images,frozen,contexts,journal,clock,provider=setup();hook=None;fault='';point=None
        if kind=='wrong_point':contexts[-1]['cursor_global']=[131,140]
        elif kind=='wrong_cell':frozen['cell']=[1,0]
        elif kind=='expired':clock.value=1550000001
        elif kind=='missing_evidence':journal.pop('events')
        elif kind=='old_passed_json':journal.clear();journal.update(passed=True);frozen['passed']=True
        elif kind=='cancelled_frame':images[2]=images[2].copy();images[2][0,0]=1
        elif kind=='final_frame':images[3]=images[3].copy();images[3][0,0]=1
        elif kind.startswith('native_'):
            if kind=='native_wrong_point':point=[131,140]
            else:fault=kind.removeprefix('native_')
        elif kind=='expires_after_validation':clock.late=True
        elif kind=='missing_clock':clock.fail=True
        attempt=FinalClickAttempt(images,frozen,contexts,journal)
        if kind=='cancel_before':attempt.cancel()
        elif kind=='cancel_after_validation':hook=attempt.cancel
        elif kind=='camera_after_validation':hook=lambda:contexts[-1].update(camera_epoch=9)
        elif kind=='pointer_after_validation':hook=lambda:contexts[-1].update(pointer_epoch=9)
        elif kind=='metadata_tamper':hook=lambda:attempt.frozen.update(id='tampered')
        elif kind=='pixels_tamper':
            def tamper():
                image=attempt.images[0];image.setflags(write=True);image[0,0,0]^=1;image.setflags(write=False)
            hook=tamper
        adapter=NativeDryRunAdapter(library,fault,point);sender=FakeClickSender();frozen_before=copy.deepcopy(frozen)
        result=attempt.run(adapter,provider,sender,clock,hook)
        assert not result.get('passed',False)and sender.calls==0 and len(sender.events)==0,(kind,result)
        assert result.get('sender_callback_calls',0)==0,(kind,result)
        native_calls=adapter.calls;attempt.run(adapter,provider,sender,clock)
        assert adapter.calls==native_calls and sender.calls==0 and frozen==frozen_before
        results.append({'case':kind,'refused':True,'sender_calls':0,'fake_events':0,'native_calls':native_calls,'status':result['status']})
    images,frozen,contexts,journal,clock,provider=setup();attempt=FinalClickAttempt(images,frozen,contexts,journal);sender=FakeClickSender(True);adapter=NativeDryRunAdapter(library)
    result=attempt.run(adapter,provider,sender,clock)
    assert result['status']=='uncertain_after_sender_entry'and result['sender_callback_calls']==1 and len(sender.events)==1
    attempt.run(adapter,provider,sender,clock);assert adapter.calls==1 and sender.calls==1 and len(sender.events)==1
    results.append({'case':'partial_sender_failure','uncertain':True,'sender_calls':1,'known_fake_events':1,'automatic_retries':0})
    images,frozen,contexts,journal,clock,provider=setup();sender=FakeClickSender();attempt=FinalClickAttempt(images,frozen,contexts,journal)
    result=attempt.run(ExistingCLIAdapter(),provider,sender,clock)
    assert result['status']=='unsupported_live_adapter'and result['native_calls_this_run']==0 and sender.calls==0
    results.append({'case':'unsupported_live','refused':True,'native_calls':0,'sender_calls':0})
    output={'passed':True,'mode':args.mode,'cases':results,'authored_cases_passed':[len(results),len(results)],'pre_sender_refusals':[len(failures),len(failures)],
            'positive_exact_one_dispatch':[1,1],'partial_failure_no_retry':[1,1],'unsupported_live_refused':[1,1],'OS_event_posts':0,'original_game_launches':0,'captures':0,
            'live_evidence_producer':'unsupported; no live integration claimed','library':str(library)}
    (out/'result.json').write_text(json.dumps(output,indent=2)+'\n');print(json.dumps({k:v for k,v in output.items()if k!='cases'}))


if __name__=='__main__':main()
