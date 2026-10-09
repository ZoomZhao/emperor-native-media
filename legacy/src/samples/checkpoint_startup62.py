"""Same controlled device and observer; use existing approved14-second ceiling."""
import json
import checkpoint_startup61 as previous

def factory62(mode,watch_state,tag):
    Base,x,u,Boundary,base=previous.controlled_factory(mode,watch_state,tag)
    class Runtime62(Base):
        def thread_run(self):
            if self.device61 is None:
                raise Boundary('Stage62_requires_existing_controlled_device_no_attach')
            self.attach61()  # existing-device path only; exact restoration audit
            try:
                from stage20_threads import ThreadKernel
                return ThreadKernel.thread_run(self,14,max_steps=200000)
            finally:
                from stage61_input import ROOT
                (ROOT/f'evidence/stage62-{self.tag54}-input-exit.json').write_text(json.dumps(dict(
                    device=self.device61.snapshot(),meta=self.meta61,host_keyboard_read=False,
                    original_DLL_executed=False,actual_original_segment_ceiling_seconds=14),indent=2)+'\n')
    return Runtime62,x,u,Boundary,base

if __name__=='__main__':
    previous.wrapper.runtime_class=factory62
    raise SystemExit(previous.wrapper.main())
