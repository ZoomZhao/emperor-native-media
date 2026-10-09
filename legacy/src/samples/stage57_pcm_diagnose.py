"""Only diagnose the existing offline city PCM comparison; no guest/device run."""
import argparse
import array
import collections
import ctypes
import hashlib
import json
import math
import struct
import sys
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EV = ROOT / 'evidence'
RUNS = ROOT / 'runs'
LEDGER_SHA = '5ed6414885425fa973e6d8de6165784f090639113ed1bc878d5fccb23457b931'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def fourcc(text):
    return int.from_bytes(text.encode('ascii'), 'big')


def capture():
    from stage37_audio import NativeAudio
    from stage56_music import NativeStreams56
    audio = NativeAudio()
    stream = NativeStreams56(audio)
    toolbox = ctypes.CDLL('/System/Library/Frameworks/AudioToolbox.framework/AudioToolbox')
    get_file = toolbox.ExtAudioFileGetProperty
    get_conv = toolbox.AudioConverterGetProperty
    for fn in (get_file, get_conv):
        fn.restype = ctypes.c_int32
        fn.argtypes = [ctypes.c_void_p, ctypes.c_uint32,
                       ctypes.POINTER(ctypes.c_uint32), ctypes.c_void_p]

    def prop(fn, obj, name, length):
        output = ctypes.create_string_buffer(length)
        size = ctypes.c_uint32(length)
        status = fn(obj, fourcc(name), ctypes.byref(size), output)
        return dict(OSStatus=status, bytes=size.value,
                    hex=output.raw[:size.value].hex() if status == 0 else None)

    def asbd(record):
        if record['OSStatus'] == 0:
            fields = struct.unpack('<d8I', bytes.fromhex(record['hex']))
            record['format'] = dict(zip(['rate', 'format_id', 'flags', 'bytes_per_packet',
                                        'frames_per_packet', 'bytes_per_frame', 'channels',
                                        'bits', 'reserved'], fields))
            record['format']['format_fourcc'] = fields[1].to_bytes(4, 'big').decode('ascii')
        return record

    try:
        rc, handle = stream.open_guest(b'audio\\music\\BG01-Lonely_Er_Hu.MP3')
        assert rc == 0 and handle, (rc, handle)
        stats = stream.stats()
        image = stream.snapshot()
        # Own documented State layout only. No original address or guest memory.
        assert stream.fn['size']() == len(image) == 257120
        assert ctypes.string_at(stream.state, len(image)) == image
        file_ref = ctypes.c_void_p.from_address(stream.state + len(image)).value
        pcm_ref = ctypes.c_void_p.from_address(stream.state + len(image) + 8).value
        assert file_ref and pcm_ref and ctypes.sizeof(ctypes.c_void_p) == 8
        conv_record = prop(get_file, file_ref, 'acnv', 8)
        assert conv_record['OSStatus'] == 0
        conv_ref = struct.unpack('<Q', bytes.fromhex(conv_record['hex']))[0]
        records = {}
        for name in ('srcq', 'srca', 'prmm', 'cdqu', 'dith', 'dbit', 'srcp'):
            rec = prop(get_conv, conv_ref, name, 4)
            if rec['OSStatus'] == 0:
                rec['uint32'] = struct.unpack('<I', bytes.fromhex(rec['hex']))[0]
                if name == 'srca':
                    rec['fourcc'] = rec['uint32'].to_bytes(4, 'big').decode('ascii')
            records[name] = rec
        records['prim'] = prop(get_conv, conv_ref, 'prim', 8)
        if records['prim']['OSStatus'] == 0:
            records['prim']['leading_trailing_frames'] = list(struct.unpack(
                '<2I', bytes.fromhex(records['prim']['hex'])))
        records['chmp'] = prop(get_conv, conv_ref, 'chmp', 8)
        if records['chmp']['OSStatus'] == 0:
            records['chmp']['channels'] = list(struct.unpack(
                '<2i', bytes.fromhex(records['chmp']['hex'])))
        formats = dict(file=asbd(prop(get_file, file_ref, 'ffmt', 40)),
                       client=asbd(prop(get_file, file_ref, 'cfmt', 40)),
                       converter_input=asbd(prop(get_conv, conv_ref, 'acid', 40)),
                       converter_output=asbd(prop(get_conv, conv_ref, 'acod', 40)))
        pcm = ctypes.string_at(pcm_ref, stats['decoded_frames'] * 4)
        output = RUNS / 'stage57-backend-city.wav'
        with wave.open(str(output), 'wb') as dest:
            dest.setparams((2, 2, 22050, 0, 'NONE', 'not compressed'))
            dest.writeframes(pcm)
        native = audio.stats()
        assert native['phase'] == native['callbacks'] == native['source_frames'] == native['render_frames'] == 0
        return dict(source_contract=stream.last_open_contract56, stats=stats, formats=formats,
                    converter_properties=records, output=str(output), PCM_sha256=hashlib.sha256(pcm).hexdigest(),
                    file_sha256=sha(output), native_audio=native,
                    pointer_inspection='self-authored State only; read-only bytes',
                    original_instructions=0, audio_device_opened=False)
    finally:
        stream.dispose()
        audio.dispose()


def read_wav(path):
    with wave.open(str(path), 'rb') as f:
        fmt = dict(channels=f.getnchannels(), sample_bytes=f.getsampwidth(),
                   rate=f.getframerate(), frames=f.getnframes(), compression=f.getcomptype())
        assert (fmt['channels'], fmt['sample_bytes'], fmt['rate'], fmt['compression']) == (2, 2, 22050, 'NONE')
        raw = f.readframes(fmt['frames'])
    samples = array.array('h')
    samples.frombytes(raw)
    if sys.byteorder != 'little':
        samples.byteswap()
    return fmt, raw, samples


def edges(samples):
    frames = len(samples)//2
    first = next((i for i in range(frames) if samples[2*i] or samples[2*i+1]), None)
    last = next((i for i in range(frames-1, -1, -1) if samples[2*i] or samples[2*i+1]), None)
    return dict(first_nonzero_frame=first, last_nonzero_frame=last,
                leading_zero_frames=frames if first is None else first,
                trailing_zero_frames=frames if last is None else frames-1-last,
                first_16_frames=[list(samples[i:i+2]) for i in range(0, 32, 2)],
                last_16_frames=[list(samples[i:i+2]) for i in range(len(samples)-32, len(samples), 2)])


def compare(reference, label):
    afmt, araw, a = read_wav(RUNS/'stage57-backend-city.wav')
    bfmt, braw, b = read_wav(reference)
    assert afmt == bfmt and len(a) == len(b)
    frames = afmt['frames']
    signed = collections.Counter()
    bins = collections.Counter()
    channels = [dict(samples=0, different=0, error_sum=0, error_squares=0,
                     native_squares=0, reference_squares=0, cross_product=0, max_abs_error=0,
                     native_min=32767, native_max=-32768, reference_min=32767, reference_max=-32768)
                for _ in range(2)]
    regions = [dict(start_frame=i, end_frame=min(i+22050, frames), different_samples=0,
                    error_squares=0, max_abs_error=0) for i in range(0, frames, 22050)]
    first = last = None
    for i, (x, y) in enumerate(zip(a, b)):
        d = x-y
        ad = abs(d)
        q = channels[i%2]
        q['samples'] += 1
        q['different'] += bool(d)
        q['error_sum'] += d
        q['error_squares'] += d*d
        q['native_squares'] += x*x
        q['reference_squares'] += y*y
        q['cross_product'] += x*y
        q['max_abs_error'] = max(q['max_abs_error'], ad)
        q['native_min'] = min(q['native_min'], x)
        q['native_max'] = max(q['native_max'], x)
        q['reference_min'] = min(q['reference_min'], y)
        q['reference_max'] = max(q['reference_max'], y)
        signed[d] += 1
        bucket = next((str(t) for t in (0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 1024, 32768, 65536) if ad <= t))
        bins[bucket] += 1
        r = regions[i//(2*22050)]
        r['different_samples'] += bool(d)
        r['error_squares'] += d*d
        r['max_abs_error'] = max(r['max_abs_error'], ad)
        if d:
            if first is None:
                first = i
            last = i
    for q in channels:
        n = q['samples']
        q['error_RMS_LSB'] = math.sqrt(q['error_squares']/n)
        q['error_mean_LSB'] = q['error_sum']/n
        q['native_RMS_LSB'] = math.sqrt(q['native_squares']/n)
        q['reference_RMS_LSB'] = math.sqrt(q['reference_squares']/n)
        q['gain_native_over_reference_least_squares'] = q['cross_product']/q['reference_squares'] if q['reference_squares'] else None
        q['normalized_cross_product'] = q['cross_product']/math.sqrt(q['native_squares']*q['reference_squares']) if q['native_squares'] and q['reference_squares'] else None
        q['error_dbFS'] = 20*math.log10(q['error_RMS_LSB']/32768) if q['error_RMS_LSB'] else None
    for r in regions:
        r['error_RMS_LSB'] = math.sqrt(r['error_squares']/((r['end_frame']-r['start_frame'])*2))
    # Distributed observations detect a fixed delay without any playback.
    # Positive lag compares native[t] with reference[t+lag]. Not an exhaustive delay proof.
    points = [4096 + k*(frames-8192)//1024 for k in range(1024)]
    best = []
    for lag in range(-2048, 2049):
        squares = sum((a[2*t]-b[2*(t+lag)])**2 for t in points)
        if lag in (-2048, -1024, -512, -256, -1, 0, 1, 256, 512, 1024, 2048):
            best.append(dict(lag=lag, sample_error_RMS=math.sqrt(squares/len(points))))
        if lag == -2048 or squares < winner[0]:
            winner = (squares, lag)
    swapped = a[1::2] == b[::2] and a[::2] == b[1::2]
    byte_swapped = array.array('h', a)
    byte_swapped.byteswap()
    total_squares = sum(q['error_squares'] for q in channels)
    return dict(label=label, reference=str(reference), format=afmt,
                native_PCM_sha256=hashlib.sha256(araw).hexdigest(), reference_PCM_sha256=hashlib.sha256(braw).hexdigest(),
                byte_identity=araw==braw, channels_swapped_identity=swapped,
                byte_swap_identity=byte_swapped==b,
                different_samples=sum(q['different'] for q in channels), total_samples=len(a),
                different_sample_fraction=sum(q['different'] for q in channels)/len(a),
                max_abs_error_LSB=max(q['max_abs_error'] for q in channels),
                error_RMS_LSB=math.sqrt(total_squares/len(a)),
                error_RMS_full_scale=math.sqrt(total_squares/len(a))/32768,
                first_difference_sample=first, last_difference_sample=last,
                signed_error_histogram_most_common=signed.most_common(32),
                signed_error_unique_count=len(signed), abs_error_bins_upper_inclusive=dict(bins),
                channels=channels, native_edges=edges(a), reference_edges=edges(b),
                best_tested_lag=winner[1], best_lag_sample_error_RMS=math.sqrt(winner[0]/len(points)),
                delay_search=dict(min=-2048, max=2048, points=1024, candidates=best),
                one_second_distribution=regions)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--mode', choices=['capture', 'compare'], required=True)
    parser.add_argument('--reference', type=Path)
    parser.add_argument('--label', default='default')
    args = parser.parse_args()
    assert sha(EV/'stage55-batch-ledger.json') == LEDGER_SHA
    if args.mode == 'capture':
        result = capture()
    else:
        result = compare(args.reference, args.label)
    assert sha(EV/'stage55-batch-ledger.json') == LEDGER_SHA
    result['original_ledger_unchanged'] = True
    output = EV / ('stage57-' + args.mode + ('-'+args.label if args.mode == 'compare' else '') + '.json')
    output.write_text(json.dumps(result, indent=2))
    print(json.dumps(dict(evidence=str(output),
                          max_error=result.get('max_abs_error_LSB'), RMS=result.get('error_RMS_LSB'),
                          identity=result.get('byte_identity'))))


if __name__ == '__main__':
    main()
