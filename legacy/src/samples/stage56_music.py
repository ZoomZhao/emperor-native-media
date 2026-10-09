"""Frozen MP3 contract over the existing self-authored offline stream backend.

No emulator, original EXE, original DLL or audio-device start is used here.
The old backend/library and its version1 snapshot format remain unchanged.
"""
import hashlib
import json
import math
import struct
import subprocess
import xml.etree.ElementTree as ET
from pathlib import Path

from stage50_close_stream import NativeStreams50

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / 'stage8-local/EmperorBaseline.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]'
AUDIO = GAME / 'Audio'


class AudioContractError(ValueError):
    pass


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        for block in iter(lambda: f.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


class FrozenAudioCatalog:
    """Use the frozen-data inventory, not a hand-maintained song-name list."""
    def __init__(self):
        self.root = AUDIO.resolve(strict=True)
        baseline = json.loads((ROOT / 'evidence/stage52-postflight.json').read_text())
        self.expected = {}
        for q in baseline['checks']:
            p = Path(q['path'])
            try:
                p.relative_to(AUDIO)
            except ValueError:
                continue
            if p.suffix.casefold() == '.mp3':
                self.expected[str(p)] = q['sha256']
        if not self.expected:
            raise AudioContractError('frozen MP3 inventory is absent')

    def _contained(self, path):
        p = Path(path)
        if not p.is_absolute():
            raise AudioContractError('host audio path must be absolute')
        try:
            relative = p.relative_to(AUDIO)
        except ValueError:
            raise AudioContractError('audio outside frozen GameData')
        if any(part in ('.', '..') for part in relative.parts):
            raise AudioContractError('audio path traversal')
        current = AUDIO
        for part in relative.parts:
            current = current / part
            if current.is_symlink():
                raise AudioContractError('audio symlinks are unsupported')
        if p.resolve(strict=True).parent != p.parent.resolve(strict=True):
            raise AudioContractError('audio parent identity mismatch')
        if self.root not in p.resolve(strict=True).parents:
            raise AudioContractError('audio escaped frozen root')
        if not p.is_file() or p.suffix.casefold() != '.mp3':
            raise AudioContractError('only frozen MP3 regular files are supported')
        if str(p) not in self.expected:
            raise AudioContractError('audio is not part of the frozen inventory')
        return p

    def resolve_guest(self, raw):
        if not isinstance(raw, bytes) or not raw or b'\0' in raw or len(raw) >= 260:
            raise AudioContractError('invalid guest audio string')
        try:
            text = raw.decode('ascii')
        except UnicodeDecodeError:
            raise AudioContractError('unsupported guest audio filename encoding')
        if ':' in text or text.startswith(('/', '\\')):
            raise AudioContractError('absolute or drive audio path')
        parts = text.replace('\\', '/').split('/')
        if any(p in ('', '.', '..') for p in parts) or parts[0].casefold() != 'audio':
            raise AudioContractError('guest audio path outside Audio or traverses')
        current = GAME
        for part in parts:
            matches = [p for p in current.iterdir() if p.name.casefold() == part.casefold()]
            if len(matches) != 1:
                raise AudioContractError('missing or ambiguous frozen audio component')
            current = matches[0]
            if current.is_symlink():
                raise AudioContractError('audio symlinks are unsupported')
        return self._contained(current)

    def inspect(self, path):
        p = self._contained(path)
        actual = digest(p)
        if actual != self.expected[str(p)]:
            raise AudioContractError('frozen compressed audio hash changed')
        proc = subprocess.run(['/usr/bin/afinfo', '-x', str(p)], capture_output=True,
                              check=True, timeout=10)
        root = ET.fromstring(proc.stdout)
        ns = {'a': 'http://apple.com/core_audio/audio_info'}
        tracks = root.findall('.//a:track', ns)
        if len(tracks) != 1:
            raise AudioContractError('exactly one audio track required')
        track = tracks[0]
        def value(name):
            return track.find('a:' + name, ns).text
        codec = value('format_type')
        channels = int(value('num_channels'))
        rate = float(value('sample_rate'))
        duration = float(value('duration'))
        # This is the implemented Apple decode/resample contract, not all MSS formats.
        if codec != '.mp3' or channels != 2 or rate != 44100:
            raise AudioContractError('unsupported frozen stream format; require stereo44100 MP3')
        if not math.isfinite(duration) or not 0 < duration < 240:
            raise AudioContractError('stream exceeds existing 240-second offline decoder capacity')
        return dict(path=str(p), sha256=actual, bytes=p.stat().st_size,
                    codec=codec, channels=channels, sample_rate=rate,
                    estimated_duration_seconds=duration, read_only=True,
                    catalog='existing frozen-data SHA inventory; no per-song whitelist')


class NativeStreams56(NativeStreams50):
    """General supported MP3/loop values; ABI and native snapshots remain version1."""
    def __init__(self, audio, catalog=None):
        self.catalog56 = catalog or FrozenAudioCatalog()
        self.last_open_contract56 = None
        super().__init__(audio)

    def open(self, path, bytes=256000):
        if type(bytes) is not int or bytes != 256000:
            raise AudioContractError('original wrapper buffer must be256000')
        proof = self.catalog56.inspect(Path(path))
        rc, handle = super().open(Path(proof['path']), bytes)
        if rc == 0:
            if digest(proof['path']) != proof['sha256']:
                self.command(handle, 5)
                raise AudioContractError('frozen audio changed during decode')
            self.last_open_contract56 = proof
        return rc, handle

    def open_guest(self, path, bytes=256000):
        return self.open(self.catalog56.resolve_guest(path), bytes)

    def command(self, handle, command, value=0):
        if type(value) is not int:
            raise AudioContractError('stream parameter must be an integer')
        if command == 1 and not 0 <= value <= 0x7fffffff:
            raise AudioContractError('loop count must be a nonnegative signed32 total-play count')
        if command == 2 and not 0 <= value <= 127:
            raise AudioContractError('supported stream volume is0..127')
        return super().command(handle, command, value)

    def restore(self, blob):
        if len(blob) != 257120:
            raise AudioContractError('unknown native stream snapshot size')
        fields = struct.unpack_from('<13I', blob)
        if fields[3]:
            if fields[5] > 0x7fffffff or fields[6] > 127:
                raise AudioContractError('unsupported saved loop or volume')
            path = blob[96:1120].split(b'\0', 1)[0]
            if not path:
                raise AudioContractError('saved live stream path missing')
            self.catalog56.inspect(Path(path.decode('utf-8')))
        # Validation precedes the existing quiescent resource replacement.
        return super().restore(blob)
