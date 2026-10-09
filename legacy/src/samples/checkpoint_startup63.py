"""Prepared only: closed batch refuses; same state/input, total child hard60 remains."""
import time
PROCESS_START=time.monotonic()
import argparse,copy,ctypes,hashlib,json,sys
from pathlib import Path
from stage54_batch import GateError,claim,finish
from stage54_watch import EmulationGate
from stage63_checkpoint import budget_seconds,save_checkpoint
ROOT=Path(__file__).resolve().parents[1]

def runtime_mixin(Base,x,Boundary,kernel_run,mark):
    class Runtime63(Base):
        def thread_run(self):
            if self.device61 is None:raise Boundary('Existing_controlled_device_required63')
            seconds=budget_seconds(time.monotonic()-PROCESS_START)
            if seconds<1:raise Boundary('Insufficient_whole_process_time_before_guest63',remaining_guest_seconds=seconds)
            self.attach61();mark('GUEST_BEGIN',seconds=seconds,reserve_seconds=25)
            try:return kernel_run(self,seconds,max_steps=200000)
            finally:
                mark('GUEST_END')
                (ROOT/f'evidence/stage63-{self.tag54}-input-exit.json').write_text(json.dumps(dict(
                    device=self.device61.snapshot(),meta=self.meta61,host_keyboard_read=False,
                    actual_original_segment_ceiling_seconds=seconds),indent=2)+'\n')

        def present_primary(self,o,pc,name):
            if not self.watch54.state['frame_candidate']:
                raise Boundary('Reviewed_prior_city_frame_required63')
            self.guard54();g=self.verify_native_geometry();n=g['client_w']*g['client_h']*4
            before=self.dd.pixels(o['id']);raw=ctypes.create_string_buffer(n);out=(ctypes.c_uint32*12)()
            mark('NATIVE_PRESENT_BEGIN')
            status=self.dd.fn['present'](self.dd.state,o['id'],out,raw,n,None)
            stats=dict(zip(['width','height','desktop_crop_x','desktop_crop_y','GPU_readback_pitch',
                'GPU_byte_mismatches','actual_drawable_presentations','visible','key_before','key_after','active_before','active_after'],out))
            if not status or stats['GPU_byte_mismatches'] or stats['actual_drawable_presentations']<1 or self.dd.pixels(o['id'])!=before:
                raise Boundary('Actual_primary_present_integrity_failed63',stats=stats)
            event=dict(original_call_return_pc=hex(pc),method=name,surface=o['id'],stats=stats,
                GPU_readback_sha256=hashlib.sha256(raw.raw).hexdigest(),source='only original successful primary Blt pixels',
                PNG_path=None,PNG_sha256=None,PNG_write_skipped=True,existing_city_frame_review_preserved=True,natural_state=self.values54())
            self.presentation_events.append(event);self.guard54();mark('NATIVE_PRESENT_END',stats=stats,PNG_written=False)
    return Runtime63

def main():
    p=argparse.ArgumentParser();p.add_argument('--mode',choices=['cycle'],required=True);p.add_argument('--resume',required=True);p.add_argument('--tag',required=True)
    a=p.parse_args();journal=[];journal_path=ROOT/f'evidence/stage63-{a.tag}-timing.json'
    if not a.tag.replace('-','').isalnum() or journal_path.exists():
        print('unique simple audit tag required',file=sys.stderr);return 2
    def mark(kind,**fields):
        journal.append(dict(kind=kind,elapsed_seconds=time.monotonic()-PROCESS_START,**fields))
        journal_path.write_text(json.dumps(journal,indent=2)+'\n')
    mark('PROCESS_BEGIN');mark('CLAIM_BEGIN')
    try:index,ledger=claim(a.mode,a.resume,a.tag,sys.argv)
    except (GateError,OSError,ValueError) as e:
        mark('GATE_REFUSED',reason=str(e),runtime_imported=False,original_instructions_executed=0)
        print(json.dumps(dict(status='approval_or_budget_gate_refused',reason=str(e))),file=sys.stderr,flush=True);return 2
    mark('CLAIM_END',index=index)
    r=None;gate=EmulationGate();engine_method=None;u=None;saved=None;hashes=None;watch=copy.deepcopy(ledger['watch_state'])
    result=dict(status='failed',boundary=dict(kind='Construction_or_restore_incomplete63'))
    try:
        for name in ['checkpoint_startup63.py','stage63_checkpoint.py']:
            path=str(ROOT/'samples'/name)
            if ledger['pinned_files'].get(path)!=hashlib.sha256(Path(path).read_bytes()).hexdigest():
                raise GateError('prepared continuation source is not bound: '+name)
        if not ledger['city_frame_review'] or not watch['frame_candidate']['owned_window_capture']:
            raise GateError('prior actual city frame and owned capture review required')
        from checkpoint_startup61 import controlled_factory
        from stage20_threads import ThreadKernel
        from stage12_state import restore_state,decode,field_names
        from checkpoint_stage12 import REGS
        Base,x,u,Boundary,BASE=controlled_factory(a.mode,watch,a.tag)
        Runtime=runtime_mixin(Base,x,Boundary,ThreadKernel.thread_run,mark);mark('RUNTIME_IMPORT_END')
        engine_method=u.Uc.emu_start;u.Uc.emu_start=gate.wrap(engine_method)
        prior=json.loads((ROOT/f'runs/stage12-state-{a.resume}/state.json').read_text());h=dict(prior['host']['dict'])
        if decode(h['watch_state54'])!=watch or 'keyboard_snapshot61' not in h:raise GateError('exact current observer and existing device required')
        r=Runtime('ST9BASE');r.start_threads(worker_capacity=3);mark('CONSTRUCTION_END')
        if any(q['low']==BASE for q in prior['regions']):r.map_dd()
        r.prepare_heap_restore(prior);r.prepare_resource_restore(prior);mark('RESTORE_BEGIN')
        m=restore_state(r,a.resume);mark('RESTORE_END')
        expected=decode(m['host'])['guest_threads20']
        for tid,t in r.threads.items():
            if t['context'].__getstate__()[0][40:]!=expected[tid]['context'][0][40:]:raise GateError('complete saved thread CPU mismatch')
        if gate.attempts:raise GateError('original execution attempted during restoration')
        for f in r.watch54.state['frames']:r.watch_city_return49(f['ret'])
        r.guard54();r.verify_native_geometry();r.begin_watch47();r.thread_quantum=100000
        r.primary54()  # retain original unique-primary identity check without rehashing restored pixels
        if r.distance_hook is not None:r.uc.hook_del(r.distance_hook);r.distance_hook=None
        if r.watch54.state['baseline_primary_sha256'] is None:raise GateError('existing baseline required; no reset')
        assert r.registry.start()==r.audio.start()==0
        r.configure_window_callback();r.configure_dd();r.guard54();mark('SETUP_END')
        gate.allowed=True
        try:result=r.thread_run()
        finally:gate.allowed=False
        r.guard54();mark('SAVE_BEGIN')
        m,hashes=save_checkpoint(r,a.tag,m['milestones'],ROOT,REGS,x,field_names,mark);saved=a.tag
        watch=r.watch_state54
        out=dict(source=a.resume,mode=a.mode,state=saved,result=result,watch_state=watch,natural_state=r.values54(),
            CPU_PC=hex(r.uc.reg_read(x.UC_X86_REG_EIP)),current_tid=r.current_tid,
            native_audio=r.audio.stats(),native_diagnostic=r.window.diagnostic(),emu_start_calls=gate.calls,
            original_code_patched=False,game_fields_injected=False,original_DLL_exports_executed=False,
            source_read_only=True,checkpoint_files=hashes,old_city_frame_review_preserved=True,
            new_PNG_or_window_screenshot=False,latest_original_city_events=r.city_observations49[-140:])
        (ROOT/f'evidence/stage63-{a.tag}.json').write_text(json.dumps(out,indent=2)+'\n');mark('RUNTIME_OUTPUT_END')
    except Exception as e:
        result=dict(status='failed',boundary=getattr(e,'details',dict(kind='Wrapper_or_restore_error63',exception=type(e).__name__,text=str(e))))
        if r is not None:watch=r.watch_state54
        (ROOT/f'evidence/stage63-{a.tag}-failure.json').write_text(json.dumps(dict(result=result,watch_state=watch,emu_start_calls=gate.calls),indent=2)+'\n')
        mark('FAILED',result=result)
    finally:
        gate.allowed=False;mark('CLEANUP_BEGIN')
        if r is not None:
            try:
                r.dispose();(ROOT/f'evidence/stage63-{a.tag}-after-dispose.json').write_text(json.dumps(r.lifecycle47(),indent=2)+'\n')
            except Exception as e:result=dict(status='failed',boundary=dict(kind='Native_cleanup_failed63',text=str(e)))
        mark('CLEANUP_END')
        if u is not None and engine_method is not None:u.Uc.emu_start=engine_method
        finish(index,result,watch,saved,hashes);mark('LEDGER_FINISH_END')
    print(json.dumps(dict(result=result,checkpoint=saved,elapsed_seconds=time.monotonic()-PROCESS_START)),flush=True)
    mark('BEFORE_NORMAL_INTERPRETER_EXIT')
    return 1 if result['status']=='failed' else 0
if __name__=='__main__':raise SystemExit(main())
