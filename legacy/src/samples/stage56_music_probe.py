"""Own offline tests only: no Unicorn, original EXE execution or device start."""
import argparse
import hashlib
import json
import struct
import sys
import wave
from pathlib import Path

from stage37_audio import NativeAudio
from stage50_close_stream import NativeStreams50, StreamCloser
from stage56_music import NativeStreams56, FrozenAudioCatalog, AudioContractError
from stage56_stream_adapter import with_generic_music, CALLERS

ROOT = Path(__file__).resolve().parents[1]
EV = ROOT / 'evidence'
CITY = b'audio\\music\\BG01-Lonely_Er_Hu.MP3'
MENU = b'AUDIO\\MUSIC\\menu1-san_xuan.MP3'
SNAPSHOT = ROOT / 'runs/stage56-city-stream-mid.bin'
CASES = []


def passed(name, **details):
    CASES.append(dict(name=name, passed=True, **details))


def reject(name, action, stream=None):
    before = stream.snapshot() if stream else None
    try:
        action()
    except (AudioContractError, FileNotFoundError) as e:
        if stream:
            assert stream.snapshot() == before
        passed(name, reason=str(e), native_state_unchanged=stream is not None)
        return
    raise AssertionError('expected contract rejection: ' + name)


def fnv(body):
    value = 14695981039346656037
    for byte in body:
        value = ((value ^ byte) * 1099511628211) & 0xffffffffffffffff
    return value


def stop_and_close(stream, h):
    close = StreamCloser(stream).close(h)
    assert close['result'] == 0
    assert stream.fn['status'](stream.state, h) == -1
    assert stream.stats()['decoder_file_open'] == 0
    return close


def create():
    catalog = FrozenAudioCatalog()
    for path in [b'../Audio/Music/BG01-Lonely_Er_Hu.mp3',
                 b'audio/music/../../mss32.dll', b'C:\\audio\\music\\song.mp3',
                 b'/audio/music/song.mp3', CITY+b'\0', b'audio//music/song.mp3',
                 b'audio/music/NOT_EXIST.mp3']:
        reject('reject unsafe/missing guest path ' + repr(path), lambda: catalog.resolve_guest(path))
    reject('reject host file outside frozen Audio', lambda: catalog.inspect(Path('/tmp/outside.mp3')))
    city_proof = catalog.inspect(catalog.resolve_guest(CITY))
    menu_proof = catalog.inspect(catalog.resolve_guest(MENU))
    passed('different songs and case resolve through one frozen inventory',
           city=city_proof, menu=menu_proof, inventory_MP3_count=len(catalog.expected))
    audio = NativeAudio()
    stream = NativeStreams56(audio, catalog)
    try:
        rc, h = stream.open_guest(CITY)
        print(json.dumps(dict(event='actual_city_open', OSStatus=rc, handle=h)), flush=True)
        assert rc == 0 and h
        opened = stream.stats()
        n = opened['decoded_frames']
        assert n > 22050*100 and opened['decoder_file_open'] == 1
        passed('actual city MP3 decoded by existing Apple backend', stats=opened)
        reference = ROOT / 'runs/stage56-city-reference.wav'
        independent = dict(available=False, comparison=None)
        if reference.exists():
            with wave.open(str(reference), 'rb') as f:
                if f.getnframes():
                    assert (f.getnchannels(), f.getsampwidth(), f.getframerate()) == (2,2,22050)
                    pcm = f.readframes(f.getnframes())
                    reference_hash=fnv(pcm)
                    independent = dict(available=True, frames=f.getnframes(),
                                       decoded_fnv64=reference_hash,
                                       native_decoded_fnv64=opened['decoded_pcm_fnv64'],
                                       frame_count_equal=f.getnframes()==n,
                                       comparison=f.getnframes()==n and reference_hash==opened['decoded_pcm_fnv64'])
                    CASES.append(dict(name='independent afconvert PCM byte identity',
                                      passed=independent['comparison'], **independent,
                                      failure_is_retained=True,
                                      difference_cause_not_proven=True))
        assert stream.command(h, 1, 1) == stream.command(h, 2, 20) == stream.command(h, 3) == 0
        assert stream.fn['pull'](stream.state, 12345) == 0
        mid = stream.snapshot()
        SNAPSHOT.write_bytes(mid)
        mid_stats = stream.stats()
        assert mid_stats['consumed_frames'] == 12345 and mid_stats['status'] == 4
        rejected_blob = bytearray(mid)
        fake_path = b'/tmp/outside.mp3\0'
        rejected_blob[96:1120] = fake_path + bytes(1024-len(fake_path))
        reject('restore validates frozen path before replacing live resources',
               lambda: stream.restore(bytes(rejected_blob)), stream)
        for bad in [-1, 0x80000000, 1.25]:
            reject('reject unsupported loop value ' + repr(bad), lambda: stream.command(h,1,bad), stream)
        reject('reject unsupported volume', lambda: stream.command(h,2,128), stream)
        assert stream.fn['pull'](stream.state, n-12345+123) == 0
        ended = stream.stats()
        assert ended['consumed_frames'] == n and ended['queued_frames'] == 0 and ended['status'] == 2
        assert stream.fn['pull'](stream.state, 1000) == 0 and stream.stats() == ended
        passed('loop1 ends after exactly one complete play with no extra frames', stats=ended)
        closed = stop_and_close(stream, h)
        passed('actual file/PCM release and stale handle rejection', close=closed)
        for loops, requested in [(2, 2*n+123), (0, n+123)]:
            rc, h = stream.open_guest(CITY)
            assert rc == 0 and h
            assert stream.command(h,1,loops) == stream.command(h,2,20) == stream.command(h,3) == 0
            assert stream.fn['pull'](stream.state,requested) == 0
            q = stream.stats()
            assert q['consumed_frames'] == (2*n if loops == 2 else requested)
            assert q['status'] == (2 if loops == 2 else 4)
            assert q['nonzero_mixed_samples'] > 0
            passed('positive2 total plays / zero indefinite', loop_count=loops, stats=q)
            blob = stream.snapshot()
            stream.restore(blob)
            assert stream.snapshot() == blob
            passed('native queue/cursor/loop state roundtrips exactly', loop_count=loops,
                   bytes=len(blob), sha256=hashlib.sha256(blob).hexdigest())
            stop_and_close(stream, h)
        rc, h = stream.open_guest(MENU)
        assert rc == 0 and h
        assert stream.command(h,1,0) == stream.command(h,2,20) == stream.command(h,3) == 0
        assert stream.fn['pull'](stream.state,1000) == 0
        menu_blob = stream.snapshot()
        legacy = NativeStreams50(audio)
        try:
            legacy.restore(menu_blob)
            assert legacy.snapshot() == menu_blob
            passed('new contract snapshots remain byte-compatible with existing backend', bytes=len(menu_blob))
        finally:
            legacy.dispose()
        stop_and_close(stream,h)
        native = audio.stats()
        assert native['phase'] == 0 and native['callbacks'] == native['source_frames'] == native['render_frames'] == 0
        assert 'unicorn' not in sys.modules
        return dict(city_open=opened, city_mid=mid_stats, city_end_control=ended,
                    snapshot=str(SNAPSHOT), snapshot_sha256=hashlib.sha256(mid).hexdigest(),
                    remaining_pull_frames=n-12345+123, independent_PCM=independent,
                    native_audio=native, device_opened=False, source_PCM_sent_to_device=False)
    finally:
        stream.dispose()
        audio.dispose()


def restore():
    control = json.loads((EV/'stage56-music-create.json').read_text())['result']
    blob = SNAPSHOT.read_bytes()
    assert hashlib.sha256(blob).hexdigest() == control['snapshot_sha256']
    audio = NativeAudio()
    stream = NativeStreams56(audio)
    try:
        stream.restore(blob)
        assert stream.snapshot() == blob and stream.stats() == control['city_mid']
        passed('separate process restores original guest handle and all stream bytes', bytes=len(blob))
        assert stream.fn['pull'](stream.state,control['remaining_pull_frames']) == 0
        assert stream.stats() == control['city_end_control']
        passed('restored future PCM hash/cursor/EOS matches uninterrupted control', stats=stream.stats())
        closed = stop_and_close(stream,stream.stats()['handle'])
        native = audio.stats()
        assert native['callbacks'] == native['source_frames'] == native['render_frames'] == 0
        assert 'unicorn' not in sys.modules
        return dict(future_behavior_matches=True, close=closed, native_audio=native,
                    device_opened=False, original_EXE_or_DLL_executed=False)
    finally:
        stream.dispose()
        audio.dispose()


class OwnBoundary(Exception):
    def __init__(self,kind,**details):
        self.kind=kind
        self.details=details


class OwnUC:
    def __init__(self):
        self.regs={1:0x70001000,2:0x12345678}
        self.memory={}
    def reg_read(self,k): return self.regs[k]


class OwnBase:
    def __init__(self):
        self.audio=NativeAudio()
        self.streams44=NativeStreams50(self.audio)
        self.uc=OwnUC()
        self.stream_calls44=[]
        self.api_trace=[]
        self.forwarded=[]
    def args(self,n): return [self.readword(self.uc.reg_read(1)+4+i*4) for i in range(n)]
    def readword(self,p): return self.uc.memory[p]
    def cstring(self,p,n): return self.uc.memory[p].split(b'\0',1)[0]
    def ret(self,value,n):
        self.last_ret=(self.readword(self.uc.reg_read(1)), self.uc.reg_read(1)+4+4*n, value&0xffffffff)
        self.uc.regs[2]=value&0xffffffff
    def api(self,dll,name):
        self.forwarded.append((dll,name))
        if name=='_AIL_close_stream@4':
            result=self.stream_closer50.close(self.args(1)[0])
            assert result['result']==0
            self.ret(self.uc.reg_read(2),1)
        else: return 'forwarded'
    def dispose(self):
        self.streams44.dispose()
        self.audio.dispose()


def adapter():
    x=type('OwnRegisters',(),dict(UC_X86_REG_ESP=1,UC_X86_REG_EAX=2))
    Runtime=with_generic_music(OwnBase,x,OwnBoundary)
    r=Runtime()
    try:
        def call(name,args,pc=None):
            sp=0x70001000
            r.uc.memory[sp]=CALLERS.get(name,0x762d1c) if pc is None else pc
            for i,a in enumerate(args):r.uc.memory[sp+4+i*4]=a
            return r.api('mss32.dll',name)
        r.uc.memory[0x10000]=CITY+b'\0'
        call('_AIL_open_stream@12',[r.audio.driver,0x10000,256000])
        h=r.last_ret[2]
        assert h==r.streams44.stats()['handle'] and r.last_ret[:2]==(0x762d46,0x70001010)
        passed('own guest32 open returns actual handle and stdcall12 stack')
        call('_AIL_set_stream_loop_count@8',[h,1])
        call('_AIL_set_stream_volume@8',[h,20])
        call('_AIL_start_stream@4',[h])
        n=r.streams44.stats()['decoded_frames']
        assert r.streams44.fn['pull'](r.streams44.state,n+100)==0
        call('_AIL_stream_status@4',[h])
        assert r.last_ret[2]==2 and r.streams44.stats()['consumed_frames']==n
        passed('same API adapter covers city loop1 with real decode/EOS, no fabricated success')
        try:call('_AIL_start_stream@4',[h],pc=0x123456)
        except OwnBoundary as e:
            assert e.kind=='Stream_caller_outside_reviewed_wrapper56'
            passed('unreviewed caller is still rejected')
        else:raise AssertionError('caller guard lost')
        call('_AIL_close_stream@4',[h])
        assert r.forwarded[-1]==('mss32.dll','_AIL_close_stream@4')
        assert r.streams44.stats()['handle']==0
        passed('close remains delegated to existing quiescent implementation')
        assert r.api('kernel32.dll','OtherAPI')=='forwarded'
        passed('other APIs remain delegated')
        assert 'unicorn' not in sys.modules
        return dict(actual_API_trace=r.stream_calls44,native_audio=r.audio.stats(),
                    fake_guest_used=True, emulator_imported=False, original_instructions_executed=0)
    finally:r.dispose()


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--mode',choices=['create','restore','adapter'],required=True)
    a=p.parse_args()
    ledger=ROOT/'evidence/stage55-batch-ledger.json'
    before=hashlib.sha256(ledger.read_bytes()).hexdigest()
    try:
        result={'create':create,'restore':restore,'adapter':adapter}[a.mode]()
        assert hashlib.sha256(ledger.read_bytes()).hexdigest()==before
        all_passed=all(q['passed'] for q in CASES)
        out=dict(passed=all_passed,mode=a.mode,cases=CASES,case_count=len(CASES),result=result,
                 behavioral_contract_tests_passed=all(q['passed'] for q in CASES
                     if q['name']!='independent afconvert PCM byte identity'),
                 closed_original_ledger_unchanged=True, original_process_budget_consumed=0,
                 original_instructions_executed=0, emulator_imported=False,
                 original_DLL_executed=False, source_audio_modified=False)
        (EV/f'stage56-music-{a.mode}.json').write_text(json.dumps(out,indent=2)+'\n')
        print(json.dumps(out))
        return 0 if all_passed else 1
    except Exception as e:
        (EV/f'stage56-music-{a.mode}-failure.json').write_text(json.dumps(dict(
            mode=a.mode,passed=False,completed_cases=CASES,exception=type(e).__name__,
            text=str(e),original_instructions_executed=0,emulator_imported='unicorn' in sys.modules,
            closed_original_ledger_unchanged=hashlib.sha256(ledger.read_bytes()).hexdigest()==before),indent=2)+'\n')
        raise


if __name__=='__main__':raise SystemExit(main())
