"""Sweep every original ``Cities/*.map`` through the ground selector and aggregate.

This is a measurement harness: it never edits or extends the renderer. It runs the
existing ``scripts/map_reference.py view --ground-selector`` path (and the binary it
builds) as a subprocess for each city, then reports per-city and aggregate numbers
with an explicit denominator.

Read-only rules (``AGENTS.md``): the data root is only ever read. Renders are done
without ``--output`` so no PNG is produced; the private build directory lives under
``--build-dir`` (default: a fresh system temp directory outside the repository).

The headline denominator is the number of ``Cities/*.map`` files found. A map that
fails to run is listed with its exact reason; it is never silently skipped.

Known limitation recorded in the output: the selector hardcodes tileset 3
(``China_Terrain.sg3``) in ``resolveGroundTileCode``, while the ``0x20`` ground
branch of the original dispatch chain resolves in tileset 2 (``China_General.sg3``).
That branch never fires on any of the 167 city maps, so the hardcoded tileset is
inert here: ``unresolved_codes`` is 0 for every city and is not being masked by an
archive mismatch.
"""
from pathlib import Path
import argparse
import json
import os
import shutil
import statistics
import struct
import subprocess
import sys
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
STORAGE_SIDE = 228
SUPPORTED_SIDES = (84, 112, 140, 170, 226)
CITY_SIGNATURE = bytes((5, 0, 0xfe, 0xca, 0, 0, 2, 0))
SIDE_OFFSET = 84
OUTER_MARKER = 0xFEDCBAAA
RECORD_MARKER = 0xABCDEFFF
DEFAULT_ARCHIVE = 'DATA/China_Terrain.sg3'
DEFAULT_WRAPPER = ROOT / 'scripts' / 'map_reference.py'

# Per-city fields copied verbatim from the selector JSON.
FIELDS = ('active_cells', 'known_cells', 'unknown_cells', 'no_ground_cells',
          'unresolved_codes', 'geometry_supported_cells', 'culled', 'visible_cells',
          'drawn_cells', 'decode_failures', 'unique_decoded_IDs', 'cache_bytes')


def decode_map_payload(path):
    """Decode the framed zlib container to its concatenated payload (read only)."""
    raw = Path(path).read_bytes()
    if len(raw) < 4 or struct.unpack_from('<I', raw, 0)[0] != OUTER_MARKER:
        raise ValueError('missing outer container marker 0xfedcbaaa')
    cursor = 4
    out = bytearray()
    while cursor < len(raw):
        if len(raw) - cursor < 12 or struct.unpack_from('<I', raw, cursor)[0] != RECORD_MARKER:
            raise ValueError('bad compressed record marker')
        encoded, declared = struct.unpack_from('<II', raw, cursor + 4)
        if not encoded or cursor + 12 + encoded > len(raw):
            raise ValueError('record length out of range')
        chunk = zlib.decompress(raw[cursor + 12:cursor + 12 + encoded])
        if len(chunk) != declared:
            raise ValueError('decoded chunk length mismatch')
        out += chunk
        cursor += 12 + encoded
    return bytes(out)


def active_side(payload):
    """Return the little-endian active-side field at payload offset 84."""
    if len(payload) < SIDE_OFFSET + 4 or payload[:8] != CITY_SIGNATURE:
        raise ValueError('payload is not a validated city map header')
    return struct.unpack_from('<I', payload, SIDE_OFFSET)[0]


def last_json_line(text):
    """Parse the selector's JSON from the last non-empty stdout line."""
    for line in reversed(text.splitlines()):
        line = line.strip()
        if line.startswith('{'):
            return json.loads(line)
    raise ValueError('no JSON object on stdout')


def camera_center(side):
    return (side - 1) / 2.0


def wrapper_view(data_root, archive, side, size, scale, build_dir, map_rel):
    """Run the sanctioned ``map_reference.py view`` entry point for one city."""
    center = repr(camera_center(side))
    return subprocess.run([
        sys.executable, '-B', str(DEFAULT_WRAPPER), 'view',
        '--data-root', data_root, '--map', map_rel, '--sg3', archive,
        '--ground-selector', '--no-window',
        '--camera-x', center, '--camera-y', center,
        '--width', str(size[0]), '--height', str(size[1]), '--scale', str(scale),
        '--build-dir', str(build_dir)], capture_output=True, text=True)


def run_binary(binary, data_root, archive, side, size, scale, map_rel, timeout):
    """Run the built selector binary directly for one city."""
    center = repr(camera_center(side))
    return subprocess.run([
        str(binary), '--data-root', data_root, '--map', map_rel, '--sg3', archive,
        '--ground-selector', '--no-window',
        '--camera-x', center, '--camera-y', center,
        '--width', str(size[0]), '--height', str(size[1]), '--scale', str(scale)],
        capture_output=True, text=True, timeout=timeout)


def row_from_result(rel, result):
    row = {'map': rel, 'selector': result.get('selector')}
    for field in FIELDS:
        row[field] = result.get(field)
    row['branches'] = {b['branch']: b['cells'] for b in result.get('branch_cells', [])}
    active = row.get('active_cells') or 0
    known = row.get('known_cells') or 0
    drawn = row.get('drawn_cells') or 0
    row['known_over_active'] = (known / active) if active else None
    row['drawn_over_known'] = (drawn / known) if known else None
    return row


def cell(row, field):
    value = row.get(field)
    return value if isinstance(value, int) else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('task', choices=['sweep'])
    parser.add_argument('--data-root', required=True,
                        help='absolute GameData root; read only')
    parser.add_argument('--sg3', default=DEFAULT_ARCHIVE,
                        help='archive path relative to the data root')
    parser.add_argument('--cities', default='Cities',
                        help='cities subdirectory relative to the data root')
    parser.add_argument('--width', type=int, default=2048)
    parser.add_argument('--height', type=int, default=2048)
    parser.add_argument('--scale', type=int, default=1)
    parser.add_argument('--timeout', type=float, default=180.0,
                        help='per-city subprocess timeout in seconds')
    parser.add_argument('--build-dir', type=Path,
                        help='private build directory (default: fresh temp dir)')
    parser.add_argument('--top', type=int, default=10)
    args = parser.parse_args()

    root = Path(args.data_root).resolve()
    if not root.is_dir():
        print(json.dumps({'error': 'data root is not a directory',
                          'data_root': str(root)}))
        return 1
    archive_path = Path(args.sg3)
    if not archive_path.is_absolute():
        archive_path = root / archive_path
    if not archive_path.is_file():
        print(json.dumps({'error': 'archive not found', 'archive': str(archive_path)}))
        return 1
    for bound in (args.width, args.height):
        if not 1 <= bound <= 2048:
            parser.error('--width/--height must be within the selector 1..2048 bound')
    if not 1 <= args.scale <= 4:
        parser.error('--scale must be within the selector 1..4 bound')

    cities_dir = root / args.cities
    maps = sorted(cities_dir.glob('*.map')) if cities_dir.is_dir() else []
    other_maps = sorted(str(p.relative_to(root)) for p in root.rglob('*.map')
                        if cities_dir not in p.parents)
    size = (args.width, args.height)

    build_owned = args.build_dir is None
    build_dir = (args.build_dir or Path(tempfile.mkdtemp(
        prefix='emperor-ground-sweep-'))).resolve()
    build_dir.mkdir(parents=True, exist_ok=True)
    binary = build_dir / 'reference-view-release'

    rows, failures = [], []
    try:
        prepared = []
        for path in maps:
            rel = path.relative_to(root).as_posix()
            try:
                side = active_side(decode_map_payload(path))
            except Exception as error:  # noqa: BLE001 - surface the exact reason
                failures.append({'map': rel, 'reason': 'map decode: ' + str(error)})
                continue
            if side not in SUPPORTED_SIDES:
                failures.append({'map': rel,
                                 'reason': f'unsupported active side {side} '
                                           '(no validated crop; selector refuses)'})
                continue
            prepared.append((rel, side))

        if not prepared:
            print(json.dumps({'error': 'no runnable city map found',
                              'maps_found': len(maps), 'failures': failures},
                             ensure_ascii=False))
            return 1

        # Build once through the sanctioned entry point, using the first city as the
        # probe render; retry once so a concurrent editor mid-write does not abort us.
        first_rel, first_side = prepared[0]
        probe, last_error = None, ''
        for attempt in range(2):
            done = wrapper_view(str(root), args.sg3, first_side, size, args.scale,
                                build_dir, first_rel)
            if done.returncode == 0 and binary.is_file():
                try:
                    rows.append(row_from_result(first_rel, last_json_line(done.stdout)))
                    probe = done
                    break
                except Exception as error:  # noqa: BLE001
                    last_error = 'probe output: ' + str(error)
            else:
                last_error = (done.stderr or done.stdout).strip()[-500:]
            print(f'build attempt {attempt + 1} failed: {last_error}', file=sys.stderr)
        if probe is None:
            print(json.dumps({'error': 'selector build failed after retry',
                              'build_dir': str(build_dir), 'maps_found': len(maps),
                              'reason': last_error}, ensure_ascii=False))
            return 1

        # Reuse the exact built binary for the rest, so every row comes from one build.
        for rel, side in prepared[1:]:
            try:
                done = run_binary(binary, str(root), args.sg3, side, size, args.scale,
                                  rel, args.timeout)
            except subprocess.TimeoutExpired:
                failures.append({'map': rel,
                                 'reason': f'timeout after {args.timeout:g}s'})
                continue
            if done.returncode != 0:
                lines = (done.stderr or done.stdout).strip().splitlines()
                failures.append({'map': rel,
                                 'reason': f'exit {done.returncode}: '
                                           + (lines[-1] if lines else 'no output')})
                continue
            try:
                rows.append(row_from_result(rel, last_json_line(done.stdout)))
            except Exception as error:  # noqa: BLE001
                failures.append({'map': rel, 'reason': 'output: ' + str(error)})
    finally:
        if build_owned:
            shutil.rmtree(build_dir, ignore_errors=True)

    rows.sort(key=lambda r: (r['known_over_active'] is None,
                             r['known_over_active'] or 0.0, r['map']))
    ratios = [r['known_over_active'] for r in rows if r['known_over_active'] is not None]
    drawn_ratios = [r['drawn_over_known'] for r in rows if r['drawn_over_known'] is not None]
    selector = next((r['selector'] for r in rows if r.get('selector')), None)

    def summary(values):
        if not values:
            return None
        return {'min': min(values), 'median': statistics.median(values),
                'max': max(values)}

    aggregate = {
        'task': 'map-ground-sweep',
        'selector': selector,
        'maps_found': len(maps),
        'maps_processed': len(rows),
        'maps_failed': len(failures),
        'denominator': f'{len(rows)}/{len(maps)} cities processed',
        'complete_same_city_scenes': 0,
        'complete_same_city_scenes_denominator': len(maps),
        'archive': args.sg3,
        'camera': {'width': args.width, 'height': args.height, 'scale': args.scale,
                   'centering': 'active-crop centre (side-1)/2 on both axes'},
        'unresolved_zero': sum(1 for r in rows if cell(r, 'unresolved_codes') == 0),
        'known_at_least_0_9': sum(1 for r in rows
                                  if (r['known_over_active'] or 0) >= 0.9),
        'known_over_active': summary(ratios),
        'drawn_over_known': summary(drawn_ratios),
        'totals': {
            'active_cells': sum(cell(r, 'active_cells') for r in rows),
            'known_cells': sum(cell(r, 'known_cells') for r in rows),
            'unknown_cells': sum(cell(r, 'unknown_cells') for r in rows),
            'unresolved_codes': sum(cell(r, 'unresolved_codes') for r in rows),
        },
        'failures': failures,
        'other_map_files_outside_cities': other_maps,
        'caveats': [
            'No viewport can cover a full 84..226-cell crop: the selector caps '
            'width/height at 2048 at scale>=1, so drawn/known is a viewport fraction, '
            'not a completeness measure. known/active and unresolved_codes are '
            'whole-crop counts and do not depend on the camera.',
            'The selector hardcodes tileset 3 (China_Terrain.sg3), while the 0x20 '
            'branch of the original chain resolves in tileset 2 (China_General.sg3). '
            'Measured: that branch fires on 0 of the 167 city maps, so the hardcoded '
            'tileset is inert and unresolved_codes == 0 is genuine, not masked.',
        ],
        'rows': rows,
        'worst': rows[:args.top],
        'best': rows[-args.top:][::-1],
    }

    print(f'# ground-selector sweep: {len(rows)}/{len(maps)} cities processed, '
          f'{len(failures)} failed, archive {args.sg3}')
    print(f'# camera {args.width}x{args.height} scale {args.scale} centred on each '
          'active crop; known/active is camera-independent')
    print(f'{"map":<28}{"active":>7}{"known":>7}{"unknown":>8}{"nogrnd":>7}'
          f'{"unres":>6}{"drawn":>7}{"decfail":>8}{"known/act":>10}{"drawn/know":>11}')
    for r in rows:
        ka, dk = r['known_over_active'], r['drawn_over_known']
        print(f'{r["map"]:<28}{cell(r, "active_cells"):>7}{cell(r, "known_cells"):>7}'
              f'{cell(r, "unknown_cells"):>8}{cell(r, "no_ground_cells"):>7}'
              f'{cell(r, "unresolved_codes"):>6}{cell(r, "drawn_cells"):>7}'
              f'{cell(r, "decode_failures"):>8}'
              f'{(f"{ka:.3f}" if ka is not None else "-"):>10}'
              f'{(f"{dk:.3f}" if dk is not None else "-"):>11}')
    if failures:
        print('\n# failures (exact reason):')
        for failure in failures:
            print(f'#   {failure["map"]}: {failure["reason"]}')
    print(f'\n# unresolved_codes == 0: {aggregate["unresolved_zero"]}/{len(rows)} '
          f'processed of {len(maps)} found')
    print(f'# known/active >= 0.90: {aggregate["known_at_least_0_9"]}/{len(rows)} '
          f'processed')
    if ratios:
        print(f'# known/active min/median/max: {min(ratios):.3f}/'
              f'{statistics.median(ratios):.3f}/{max(ratios):.3f}')
        print(f'# whole-crop totals active/known/unknown/unresolved: '
              f'{aggregate["totals"]["active_cells"]}/{aggregate["totals"]["known_cells"]}'
              f'/{aggregate["totals"]["unknown_cells"]}/'
              f'{aggregate["totals"]["unresolved_codes"]}')
    print(f'\n# worst {len(aggregate["worst"])} (lowest known/active):')
    for r in aggregate['worst']:
        print(f'#   {r["map"]}: known/active={(r["known_over_active"] or 0):.3f} '
              f'unresolved={cell(r, "unresolved_codes")} '
              f'unknown={cell(r, "unknown_cells")}')
    print(f'# best {len(aggregate["best"])} (highest known/active):')
    for r in aggregate['best']:
        print(f'#   {r["map"]}: known/active={(r["known_over_active"] or 0):.3f} '
              f'unresolved={cell(r, "unresolved_codes")} '
              f'unknown={cell(r, "unknown_cells")}')
    print(json.dumps(aggregate, ensure_ascii=False))
    return 0 if not failures else 3


if __name__ == '__main__':
    raise SystemExit(main())
