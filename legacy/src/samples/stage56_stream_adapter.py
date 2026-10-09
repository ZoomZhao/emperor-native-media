"""Prepared original-wrapper integration; import creates/runs no emulator."""
from stage56_music import NativeStreams56, AudioContractError
from stage50_close_stream import StreamCloser

COUNTS = {'_AIL_open_stream@12': 3, '_AIL_set_stream_loop_count@8': 2,
          '_AIL_set_stream_volume@8': 2, '_AIL_start_stream@4': 1,
          '_AIL_stream_status@4': 1}
CALLERS = {'_AIL_open_stream@12': 0x762d46, '_AIL_set_stream_loop_count@8': 0x762daf,
           '_AIL_set_stream_volume@8': 0x762dc7, '_AIL_start_stream@4': 0x762ddb,
           '_AIL_stream_status@4': 0x762e83}


def with_generic_music(Base, x, Boundary):
    """Apply only after a separately approved runtime claim; currently tests use own fake base."""
    class GenericMusicRuntime56(Base):
        def __init__(self, *args, **kw):
            super().__init__(*args, **kw)
            self.streams44.dispose()
            self.streams44 = NativeStreams56(self.audio)
            self.stream_closer50 = StreamCloser(self.streams44)

        def api(self, dll, name):
            if dll.casefold() != 'mss32.dll' or name not in COUNTS:
                # Retain the already reviewed stage50 close/slot/quiescence contract.
                return super().api(dll, name)
            a = self.args(COUNTS[name])
            pc = self.readword(self.uc.reg_read(x.UC_X86_REG_ESP))
            if pc != CALLERS[name]:
                raise Boundary('Stream_caller_outside_reviewed_wrapper56', name=name, return_pc=hex(pc))
            before = self.streams44.stats()
            value = self.uc.reg_read(x.UC_X86_REG_EAX)
            try:
                if name == '_AIL_open_stream@12':
                    if a[0] != self.audio.driver or a[2] != 256000:
                        raise AudioContractError('original driver or buffer mismatch')
                    raw = self.cstring(a[1], 260)
                    status, value = self.streams44.open_guest(raw, a[2])
                else:
                    if not a[0] or a[0] != before['handle']:
                        raise AudioContractError('stale or unknown guest32 stream handle')
                    if name == '_AIL_stream_status@4':
                        value = self.streams44.fn['status'](self.streams44.state, a[0])
                        status = 0 if value >= 0 else value
                    else:
                        command = {'_AIL_set_stream_loop_count@8': 1,
                                   '_AIL_set_stream_volume@8': 2,
                                   '_AIL_start_stream@4': 3}[name]
                        status = self.streams44.command(a[0], command, a[1] if len(a) == 2 else 0)
                if status:
                    raise Boundary('Actual_offline_stream_backend_error56', name=name, OSStatus=status)
            except AudioContractError as error:
                raise Boundary('Frozen_stream_contract_refused56', name=name, args=a, reason=str(error))
            item = dict(name=name, args=a, return_pc=hex(pc), return_value=value,
                        before=before, after=self.streams44.stats(),
                        resource_contract=self.streams44.last_open_contract56,
                        backend='existing native ExtAudioFile offline decode and PCM ring',
                        loop_semantics='0 indefinite; positiveN total plays including first',
                        source_PCM_sent_to_device=False, original_MSS_export_executed=False,
                        exact_original_decoder_mixer_equivalence=False)
            self.stream_calls44.append(item)
            self.api_trace.append(item)
            self.ret(value, COUNTS[name])
    return GenericMusicRuntime56
