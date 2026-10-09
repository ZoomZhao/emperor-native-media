#!/usr/bin/env python3
"""Read-only audit of original files and bounded local save-frame comparison."""
import hashlib
import json
import struct
import zlib
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PREFLIGHT = json.loads((ROOT / 'evidence/stage8-local-app-preflight.json').read_text())
STAGE7 = json.loads((ROOT / 'evidence/stage7-input-inventory.json').read_text())
SOURCE = Path(PREFLIGHT['source_app']) / 'Contents'
GAME = SOURCE / 'Resources/drive_c/EmperorRotMK[ZeaS]'
COPY = Path(PREFLIGHT['prepared_copy']) / 'Contents'
COPY_GAME = COPY / 'Resources/drive_c/EmperorRotMK[ZeaS]'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def check(path, expected):
    data = path.read_bytes()
    actual = digest(data)
    return dict(path=str(path), size=len(data), expected_sha256=expected,
                sha256=actual, unchanged=actual == expected)


def frames(data):
    assert len(data) < 2 * 1024 * 1024
    assert struct.unpack_from('<I', data)[0] == 0xfedcbaaa
    pos = 4
    records, decoded = [], []
    while pos + 12 <= len(data):
        marker, compressed, declared = struct.unpack_from('<III', data, pos)
        if marker != 0xabcdefff:
            break
        assert 0 < compressed <= 131072 and 0 < declared <= 32768
        assert pos + 12 + compressed <= len(data)
        body = data[pos + 12:pos + 12 + compressed]
        decoder = zlib.decompressobj()
        out = decoder.decompress(body, declared + 1)
        assert len(out) == declared and decoder.eof
        assert not decoder.unconsumed_tail and not decoder.unused_data
        records.append(dict(index=len(records), file_offset=pos,
                            compressed_size=compressed, decoded_size=len(out),
                            decoded_sha256=digest(out)))
        decoded.append(out)
        pos += 12 + compressed
        assert len(records) <= 512
    payload = b''.join(decoded)
    return dict(frames=records, frame_count=len(records), parsed_end=pos,
                remaining_bytes=len(data) - pos, fully_framed=pos == len(data),
                decoded_size=len(payload), decoded_sha256=digest(payload)), payload


def compare(a, b):
    overlap = min(len(a), len(b))
    offsets = [i for i in range(overlap) if a[i] != b[i]]
    return dict(size_a=len(a), size_b=len(b), overlap_differing_bytes=len(offsets),
                unmatched_bytes=abs(len(a) - len(b)),
                first_differing_offsets=offsets[:64],
                identical=a == b)


def main():
    source_checks = []
    for relative, expected in PREFLIGHT['source_config_hashes'].items():
        source_checks.append(check(SOURCE / relative, expected))
    for record in PREFLIGHT['source_exes']:
        source_checks.append(check(GAME / record['name'], record['sha256']))
    for record in PREFLIGHT['source_saves']:
        source_checks.append(check(GAME / record['path'], record['sha256']))
    for name, record in PREFLIGHT['source_scripts'].items():
        source_checks.append(check(SOURCE / 'Resources/Scripts' / name, record['sha256']))
    source_checks.append(check(SOURCE / 'MacOS/wineskinlauncher', PREFLIGHT['native_hashes']['launcher']))
    source_checks.append(check(SOURCE / 'SharedSupport/wine/bin/wine64', PREFLIGHT['native_hashes']['wine64']))
    project_checks = [check(Path(STAGE7['source_exe']), STAGE7['exe_sha256']),
                      check(Path(STAGE7['selected_save']), STAGE7['save_sha256'])]
    fixed_checks = [check(Path(STAGE7['frozen_copy']), STAGE7['save_sha256']),
                    check(Path(PREFLIGHT['fixed_save_copy']), STAGE7['save_sha256'])]
    save_checks, framing, raw, decoded = {}, {}, {}, {}
    raw['input'] = Path(STAGE7['frozen_copy']).read_bytes()
    for name in ['ST8BASE', 'ST8AFTR']:
        record = json.loads((ROOT / f'evidence/stage8-{name.lower()}-file.json').read_text())
        # Recorded fields come from the actual original game output, not a writer.
        expected = record['sha256']
        save_checks[name] = [check(COPY_GAME / f'Save/zoom/{name}.sav', expected),
                            check(ROOT / f'stage8-data/{name}.sav', expected)]
        raw[name] = (ROOT / f'stage8-data/{name}.sav').read_bytes()
    for name, data in raw.items():
        framing[name], decoded[name] = frames(data)
    comparisons = {}
    for a, b in [('input', 'ST8BASE'), ('ST8BASE', 'ST8AFTR'), ('input', 'ST8AFTR')]:
        comparisons[f'{a}_to_{b}'] = dict(file_bytes=compare(raw[a], raw[b]),
                                       decoded_frame_bytes=compare(decoded[a], decoded[b]))
    audit = dict(created_utc=datetime.now(timezone.utc).isoformat(),
                 source_app_checks=source_checks, original_project_checks=project_checks,
                 fixed_input_checks=fixed_checks, game_output_checks=save_checks,
                 original_files_unchanged=all(x['unchanged'] for x in source_checks + project_checks),
                 fixed_input_unchanged=all(x['unchanged'] for x in fixed_checks),
                 game_outputs_match_frozen_copies=all(x['unchanged'] for checks in save_checks.values() for x in checks),
                 framing=framing, comparisons=comparisons,
                 interpretation='Decoded frame bytes are compared structurally only; no inferred date, economy, citizen, or object field semantics.')
    (ROOT / 'evidence/stage8-final-audit.json').write_text(json.dumps(audit, ensure_ascii=False, indent=2) + '\n')
    assert audit['original_files_unchanged'] and audit['fixed_input_unchanged']
    assert audit['game_outputs_match_frozen_copies']
    print(json.dumps({k: v for k, v in audit.items() if k in ['original_files_unchanged', 'fixed_input_unchanged', 'game_outputs_match_frozen_copies', 'comparisons']}, ensure_ascii=False, indent=2))
    print(json.dumps({name: {k: v for k, v in record.items() if k != 'frames'} for name, record in framing.items()}, indent=2))


if __name__ == '__main__':
    main()
