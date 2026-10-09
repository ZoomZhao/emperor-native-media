"""Only read fixed PE/resource bytes and our saved checkpoint; no runtime import."""
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EV = ROOT / 'evidence'
EXE = Path('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe')
EXPECTED = '8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'


def main():
    run = json.loads((EV / 'stage54-frame55a.json').read_text())
    boundary = run['result']['boundary']
    assert boundary['kind'] == 'Stream_open_parameters_unreviewed44'
    state_path = ROOT / 'runs/stage12-state-frame55a/state.json'
    state = json.loads(state_path.read_text())
    regs = state['registers']
    assert state['original_EXE_sha256'] == EXPECTED and regs['eip'] == 0x610017c0
    with (state_path.parent / 'memory.bin').open('rb') as memory:
        def body(addr, n):
            for q in state['regions']:
                if q['low'] <= addr and addr+n <= q['high']+1:
                    memory.seek(q['offset']+addr-q['low'])
                    b = memory.read(n)
                    assert len(b) == n
                    return b
            raise ValueError(hex(addr))

        def word(addr):
            return struct.unpack('<I', body(addr, 4))[0]

        def cstring(addr):
            b = body(addr, 260)
            assert b'\0' in b
            return b.split(b'\0', 1)[0].decode('ascii')

        sp, bp = regs['esp'], regs['ebp']
        args = [word(sp+4+i*4) for i in range(3)]
        assert args == boundary['args'] and word(sp) == 0x762d46
        path = cstring(args[1])
        assert path.lower() == 'audio\\music\\bg01-lonely_er_hu.mp3'
        owner = word(bp-8)
        slot = word(bp+12)
        volume_offset = 340 if slot == 1 else 344 if slot == 2 else 332
        wrapper = dict(EBP=hex(bp), caller_return=hex(word(bp+4)),
                       saved_EBP=hex(word(bp)), path_pointer=hex(word(bp+8)),
                       stream_slot_index=slot, loop_count_argument=word(bp+16),
                       audio_owner=hex(owner), driver=hex(word(owner+376)),
                       existing_slot_handle=hex(word(owner+476+slot*4)),
                       effective_volume_before_open=word(owner+volume_offset))
        pe = EXE.read_bytes()
        assert hashlib.sha256(pe).hexdigest() == EXPECTED
        head = struct.unpack_from('<I', pe, 0x3c)[0]
        nsec = struct.unpack_from('<H', pe, head+6)[0]
        optsize = struct.unpack_from('<H', pe, head+20)[0]
        base = struct.unpack_from('<I', pe, head+24+28)[0]
        matches = []
        for i in range(nsec):
            off = head+24+optsize+40*i
            _, rva, rawsize, rawoff = struct.unpack_from('<4I', pe, off+8)
            lo, hi = base+rva, base+rva+rawsize
            if lo <= 0x762ccb and 0x762de6 <= hi:
                original = pe[rawoff+0x762ccb-lo:rawoff+0x762de6-lo]
                saved = body(0x762ccb, len(original))
                assert saved == original
                matches.append(dict(start='0x762ccb', end_exclusive='0x762de6',
                                    sha256=hashlib.sha256(original).hexdigest(), saved_equal=True))
        assert len(matches) == 1
        assert args[0] == word(owner+376) == 0x5c000101 and args[2] == 256000
        assert word(bp+8) == args[1]

    roots = [ROOT / 'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]',
             Path('<LOCAL_MACHINE_PATH>]')]
    resources = []
    for root in roots:
        p = root / 'Audio/Music/BG01-Lonely_Er_Hu.mp3'
        b = p.read_bytes()
        resources.append(dict(path=str(p), bytes=len(b), sha256=hashlib.sha256(b).hexdigest(), read_only=True))
    assert resources[0]['sha256'] == resources[1]['sha256']
    afinfo = (EV / 'stage55-city-music-afinfo.stdout.log').read_text()
    assert json.loads((EV / 'stage55-city-music-afinfo.result.json').read_text())['returncode'] == 0
    result = dict(checkpoint='frame55a', stopped_PC=hex(regs['eip']),
                  exact_original_wrapper_return_PC=hex(word_sp := 0x762d46),
                  original_API=['mss32.dll', '_AIL_open_stream@12'],
                  actual_args=args, actual_arg_hex=[hex(a) for a in args], actual_path=path,
                  original_wrapper_frame=wrapper, original_wrapper_code_checks=matches,
                  driver_buffer_and_callsite_match_reviewed_menu_contract=True,
                  only_open_guard_difference_is_new_city_path=True,
                  native_stream_open_not_attempted_for_city=True,
                  city_stream_handle_not_fabricated=True,
                  music_files=resources, Apple_afinfo_readonly_metadata=afinfo,
                  duration_seconds_from_afinfo=145.811656,
                  native_decoder_static_MAXFRAMES_seconds=240,
                  duration_under_static_decoder_limit=True,
                  actual_city_MP3_PCM_decode_tested=False,
                  actual_city_MP3_lifecycle_or_persistence_tested=False,
                  original_MSS_codec_mixer_equivalence_proven=False,
                  loop_count_matches_existing_guard=wrapper['loop_count_argument'] == 0,
                  volume_matches_existing_guard=wrapper['effective_volume_before_open'] == 20,
                  original_instructions_executed_during_review=0,
                  emulator_or_native_runtime_imported=False,
                  pinned_adapter_or_ledger_changed=False,
                  followup='Review this exact file hash and saved wrapper arguments; test actual offline decoding/lifecycle/restore using own existing backend, without source PCM device output. Do not turn the menu-only file binding into a wildcard or fake stream success. Original continuation requires reopening approved scope after precise boundary review, preserving used3/remaining97/caps.')
    (EV / 'stage55-city-music-contract-review.json').write_text(
        json.dumps(result, ensure_ascii=False, indent=2) + '\n')
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
