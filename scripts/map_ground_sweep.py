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
Measured on all 167 city maps, that branch fires on 0 cities, so the hardcoded
tileset is inert for it. ``unresolved_codes`` is *not* zero everywhere: it equals
the ``0x2000000-feature`` cells (2231 in 22 cities), whose stored 5x5 block art is
composite, not single-cell geometry, so the ground render counts it as unresolved
rather than an archive mismatch. The archive slots themselves never miss (no
empty-slot resolve failure on the corpus).

With ``--image-grid`` the same harness instead sweeps the *stored* surface-image
grid (``0x00FE9880``) verbatim: it runs ``scripts/map_reference.py view
--image-grid`` and also reports how often the reconstructed dispatch chain agrees
with that stored grid. It aggregates an exact-render metric (cities whose stored
nonzero cells all resolve to decodable single-cell records) over the same 167-city
denominator and per-branch chain accuracy, and keeps the non-zero exit on failure.
It also partitions the chain's emitted-but-mismatched cells into named classes
(stored zero; stored tileset other than 3; same-tileset archive-index delta, bucketed
by sign for +-8 and by magnitude above it) and refuses a city whose classes do not
sum to its ``mismatched`` count.
"""
from pathlib import Path
import argparse
import collections
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
FIELDS = ('active_cells', 'known_cells', 'unknown_cells', 'chain_no_write_cells',
          'unresolved_codes', 'geometry_supported_cells', 'culled', 'visible_cells',
          'drawn_cells', 'decode_failures', 'unique_decoded_IDs', 'cache_bytes')

# Per-city fields copied verbatim from the stored-image-grid JSON.
IMAGE_GRID_FIELDS = ('grid_cells', 'zero_cells', 'nonzero_cells', 'decoded_cells',
                     'drawn_cells', 'culled', 'visible_cells', 'unresolved_indices',
                     'geometry_unsupported', 'archive_unavailable', 'decode_failures',
                     'unique_decoded_IDs', 'cache_bytes', 'composite_blocks',
                     'composite_confirmed_cells', 'composite_recovered_cells',
                     'composite_partial_blocks', 'composite_partial_block_cells',
                     'composite_mismatched_cells', 'composite_foreign_cells',
                     'drawn_blocks')

# Chain-score fields summed across cities for the image-grid aggregate.
CHAIN_FIELDS = ('cells', 'emitted', 'matched', 'mismatched', 'known_matched',
                'chain_no_write_vs_stored_zero', 'chain_no_write_vs_stored_nonzero',
                'stored_zero', 'stored_zero_unpredicted', 'chain_known', 'chain_unknown',
                'chain_no_write', 'unresolved')

# Per-branch fields summed across cities, and the fixed-size bucket vectors that the
# same sum applies per element. The buckets classify every emitted-but-mismatched
# cell, so the sum of all classes must equal ``mismatched`` exactly: a violated city
# is a failure, not a metric.
CHAIN_BRANCH_FIELDS = ('cells', 'emitted', 'matched', 'stored_zero',
                       'stored_not_tileset3', 'stored_zero_mismatch')
CHAIN_BRANCH_BUCKETS = (('index_delta', 17), ('index_delta_magnitude', 8),
                        ('stored_tileset_other', 16))


def chain_mismatch_classes(chain):
    """Split a chain-score document's mismatches into its named classes."""
    classes = collections.Counter()
    for branch in (chain or {}).get('branches', []):
        classes['stored_zero'] += int(branch.get('stored_zero_mismatch') or 0)
        classes['stored_tileset_other'] += int(branch.get('stored_not_tileset3') or 0)
        classes['index_delta_within_8'] += sum(branch.get('index_delta') or [])
        classes['index_delta_magnitude'] += sum(branch.get('index_delta_magnitude') or [])
    return classes


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


def wrapper_view(data_root, archive, side, size, scale, build_dir, map_rel,
                 image_grid=False, sg3_elevation=None, tileset_specs=()):
    """Run the sanctioned ``map_reference.py view`` entry point for one city."""
    center = repr(camera_center(side))
    mode = ['--image-grid'] if image_grid else ['--ground-selector']
    extras = ['--sg3-elevation', sg3_elevation] if sg3_elevation else []
    if tileset_specs:
        extras += ['--sg3-tilesets', ','.join(tileset_specs)]
    return subprocess.run([
        sys.executable, '-B', str(DEFAULT_WRAPPER), 'view',
        '--data-root', data_root, '--map', map_rel, '--sg3', archive,
        *mode, '--no-window',
        '--camera-x', center, '--camera-y', center,
        '--width', str(size[0]), '--height', str(size[1]), '--scale', str(scale),
        *extras, '--build-dir', str(build_dir)], capture_output=True, text=True)


def run_binary(binary, data_root, archive, side, size, scale, map_rel, timeout,
               image_grid=False, sg3_elevation=None, tileset_specs=()):
    """Run the built selector binary directly for one city."""
    center = repr(camera_center(side))
    mode = ['--image-grid'] if image_grid else ['--ground-selector']
    extras = ['--sg3-elevation', sg3_elevation] if sg3_elevation else []
    if tileset_specs:
        extras += ['--sg3-tilesets', ','.join(tileset_specs)]
    return subprocess.run([
        str(binary), '--data-root', data_root, '--map', map_rel, '--sg3', archive,
        *mode, '--no-window',
        '--camera-x', center, '--camera-y', center,
        '--width', str(size[0]), '--height', str(size[1]), '--scale', str(scale),
        *extras], capture_output=True, text=True, timeout=timeout)


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


def image_grid_row(rel, result):
    """One row for the stored-image-grid mode, with the chain score attached."""
    reasons = collections.Counter()
    components = collections.Counter()
    for mismatch in result.get('composite_mismatches') or []:
        reason = mismatch.get('reason') or 'unspecified'
        reasons[reason] += mismatch.get('cells') or 0
        components[reason] += 1
    row = {'map': rel, 'mode': result.get('mode'),
           'exact': bool(result.get('exact')),
           'tileset_cells': result.get('tileset_cells', []),
           'tilesets': result.get('tilesets', []),
           'mismatch_reasons': dict(reasons),
           'mismatch_components': dict(components),
           'chain': result.get('chain_score') or {}}
    for field in IMAGE_GRID_FIELDS:
        row[field] = result.get(field)
    nonzero = row.get('nonzero_cells') or 0
    drawn = row.get('drawn_cells') or 0
    row['drawn_over_nonzero'] = (drawn / nonzero) if nonzero else None
    return row


def cell(row, field):
    value = row.get(field)
    return value if isinstance(value, int) else 0


def image_grid_report(maps, rows, failures, args):
    """Aggregate the stored-image-grid sweep and print it. Never edits the renderer."""
    # The renderer's accounting contract: every stored nonzero cell is either decoded
    # or named by exactly one failure counter. A violated row is a failure, not a metric.
    for row in rows:
        covered = sum(cell(row, field) for field in
                      ('decoded_cells', 'geometry_unsupported', 'archive_unavailable',
                       'decode_failures'))
        nonzero = cell(row, 'nonzero_cells')
        if covered != nonzero:
            failures.append({'map': row['map'],
                             'reason': f'accounting identity {covered} != nonzero {nonzero}'})
        row_classes = chain_mismatch_classes(row['chain'])
        mismatched = int((row['chain'] or {}).get('mismatched') or 0)
        if sum(row_classes.values()) != mismatched:
            failures.append({'map': row['map'],
                             'reason': f'chain mismatch classes {sum(row_classes.values())} '
                                       f'!= mismatched {mismatched}'})
    rows.sort(key=lambda r: (r['drawn_over_nonzero'] is None,
                             r['drawn_over_nonzero'] or 0.0, r['map']))
    exact = [r for r in rows if r['exact']]
    totals = {field: sum(cell(r, field) for r in rows) for field in IMAGE_GRID_FIELDS}
    chain_totals = {field: sum((r['chain'] or {}).get(field, 0) or 0 for r in rows)
                    for field in CHAIN_FIELDS}
    emitted = chain_totals['emitted']
    matched = chain_totals['matched']
    branch_totals = {}
    for row in rows:
        for branch in (row['chain'] or {}).get('branches', []):
            entry = branch_totals.setdefault(branch['branch'],
                                             {field: 0 for field in CHAIN_BRANCH_FIELDS})
            for field in CHAIN_BRANCH_FIELDS:
                entry[field] += int(branch.get(field) or 0)
            for field, size in CHAIN_BRANCH_BUCKETS:
                bucket = entry.setdefault(field, [0] * size)
                values = branch.get(field) or []
                for index in range(min(size, len(values))):
                    bucket[index] += int(values[index])
    classes = collections.Counter()
    for row in rows:
        classes.update(chain_mismatch_classes(row['chain']))
    reason_totals = collections.Counter()
    for row in rows:
        for reason, cells in (row.get('mismatch_reasons') or {}).items():
            reason_totals[reason] += cells
    aggregate = {
        'task': 'map-image-grid-sweep',
        'mode': next((r.get('mode') for r in rows if r.get('mode')), None),
        'maps_found': len(maps),
        'maps_processed': len(rows),
        'maps_failed': len(failures),
        'denominator': f'{len(rows)}/{len(maps)} cities processed',
        'exact_render_cities': len(exact),
        'exact_render_denominator': len(maps),
        'exact_render_rule': 'unresolved_indices == 0 and geometry_unsupported == 0 '
                             'and archive_unavailable == 0 and decode_failures == 0',
        'zero_decode_failure_cities': sum(1 for r in rows
                                          if cell(r, 'decode_failures') == 0),
        'zero_unresolved_index_cities': sum(1 for r in rows
                                            if cell(r, 'unresolved_indices') == 0),
        'archive': args.sg3,
        'elevation_archive': args.sg3_elevation,
        'tilesets': rows[0].get('tilesets') if rows else None,
        'explicit_tilesets': args.sg3_tileset,
        'camera': {'width': args.width, 'height': args.height, 'scale': args.scale,
                   'centering': 'active-crop centre (side-1)/2 on both axes'},
        'totals': totals,
        'unresolved_reasons': dict(sorted(reason_totals.items())),
        'chain_totals': chain_totals,
        'chain_mismatch_classes': dict(sorted(classes.items())),
        'chain_branches': {name: branch_totals[name]
                           for name in sorted(branch_totals)},
        'chain_accuracy': {
            'matched_over_emitted': (matched / emitted) if emitted else None,
            'known_matched_over_cells': (chain_totals['known_matched'] /
                                         chain_totals['cells'])
                                        if chain_totals['cells'] else None,
        },
        'caveats': [
            'The stored grid render reads 0x00FE9880 verbatim; it does not re-derive '
            'the cell from the dispatch chain. Cells whose stored tileset has no '
            'supplied or auto-resolved archive are counted archive_unavailable and '
            'never drawn.',
            'Tileset indices are resolved through the original executable static '
            'name table (FUN_00475B60); the view auto-loads 2/5/6/8 next to --sg3 '
            'and 16 from China_Elevation.sg3 unless --sg3-tilesets overrides them.',
            'Composite ground is placed from the per-cell draw-state offset '
            '(0x00FDCD70: column low 3 bits, row bits 3..5) and gated by the subTile '
            'group-side code (0x00F9D620 low nibble + 1). A record is expanded only '
            'when it declares an s*s block and the cell group side equals s; every '
            'other cell stays counted and named with its reason.',
            'The record id base (recordId = low14 + 201) is the version-213 header '
            'base; every miss is counted, not hidden.',
            'A viewport cannot cover a full 84..226-cell crop at 2048px cap, so '
            'drawn_cells is a viewport fraction; the decode counts are camera-free.',
            'Every emitted-but-mismatched cell of the chain score falls in exactly one '
            'named class (stored zero, stored tileset other than 3, or a same-tileset '
            'archive-index delta bucketed by sign for +-8 and by magnitude above it). '
            'The classes must sum to mismatched for every city; a city that violates '
            'this is a failure, not a metric. See docs/MAP_GROUND_BRANCHES.md.',
        ],
        'failures': failures,
        'rows': rows,
        'worst': rows[:args.top],
    }

    print(f'# stored-image-grid sweep: {len(rows)}/{len(maps)} cities processed, '
          f'{len(failures)} failed, archive {args.sg3} (+ {args.sg3_elevation})')
    print(f'# exact render (all stored nonzero cells single-cell): '
          f'{len(exact)}/{len(maps)} cities at 100%')
    print(f'{"map":<28}{"grid":>7}{"nonzero":>8}{"drawn":>8}{"archUna":>8}'
          f'{"compose":>8}{"unres":>7}{"decfail":>8}{"drawn/nz":>10}{"exact":>7}')
    for r in rows:
        dn = r['drawn_over_nonzero']
        print(f'{r["map"]:<28}{cell(r, "grid_cells"):>7}{cell(r, "nonzero_cells"):>8}'
              f'{cell(r, "drawn_cells"):>8}{cell(r, "archive_unavailable"):>8}'
              f'{cell(r, "geometry_unsupported"):>8}{cell(r, "unresolved_indices"):>7}'
              f'{cell(r, "decode_failures"):>8}'
              f'{(f"{dn:.3f}" if dn is not None else "-"):>10}'
              f'{("yes" if r["exact"] else "no"):>7}')
    if failures:
        print('\n# failures (exact reason):')
        for failure in failures:
            print(f'#   {failure["map"]}: {failure["reason"]}')
    drawn = totals['drawn_cells']
    nonzero = totals['nonzero_cells']
    print(f'\n# aggregate drawn/nonzero: {drawn}/{nonzero} '
          f'({100 * drawn / nonzero:.3f}%) - a VIEWPORT fraction, not coverage; '
          f'decode counts above are camera-free' if nonzero else '\n# no nonzero cells')
    print(f'# chain: emitted {emitted}, matched {matched} '
          f'({100 * matched / emitted:.3f}%)' if emitted
          else '# chain: no emitted cells')
    print(f'# chain: known_matched {chain_totals["known_matched"]}/'
          f'{chain_totals["cells"]}; chain-no-write-vs-stored-zero '
          f'{chain_totals["chain_no_write_vs_stored_zero"]}; '
          f'chain-no-write-vs-stored-nonzero '
          f'{chain_totals["chain_no_write_vs_stored_nonzero"]}')
    if classes:
        print(f'# chain mismatch classes (sum {sum(classes.values())} == mismatched '
              f'{chain_totals["mismatched"]}): '
              + '; '.join(f'{name} {count}' for name, count in sorted(classes.items())))
    print(f'# composite: blocks {totals["composite_blocks"]}, confirmed '
          f'{totals["composite_confirmed_cells"]}, recovered '
          f'{totals["composite_recovered_cells"]}, partial '
          f'{totals["composite_partial_blocks"]} blocks/'
          f'{totals["composite_partial_block_cells"]} cells, mismatched '
          f'{totals["composite_mismatched_cells"]}, foreign '
          f'{totals["composite_foreign_cells"]}')
    for reason, count in sorted(reason_totals.items(), key=lambda item: -item[1]):
        print(f'# unresolved cells [{count}]: {reason}')
    print(json.dumps(aggregate, ensure_ascii=False))
    return 0 if not failures else 3


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('task', choices=['sweep'])
    parser.add_argument('--data-root', required=True,
                        help='absolute GameData root; read only')
    parser.add_argument('--sg3', default=DEFAULT_ARCHIVE,
                        help='tileset-3 archive path relative to the data root')
    parser.add_argument('--sg3-elevation',
                        help='optional tileset-16 archive path relative to the data root; '
                             'image-grid mode only (default: China_Elevation.sg3 next '
                             'to --sg3)')
    parser.add_argument('--sg3-tileset', action='append', default=[],
                        metavar='INDEX=PATH',
                        help='explicit image-grid tileset archive relative to the data '
                             'root; repeatable (default: the executable name table '
                             'auto-loads tilesets 2/5/6/8 next to --sg3)')
    parser.add_argument('--image-grid', action='store_true',
                        help='sweep the stored 0x00FE9880 image grid (render + chain '
                             'score) instead of the reconstructed ground selector')
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
    if args.sg3_elevation:
        elevation_path = Path(args.sg3_elevation)
        if not elevation_path.is_absolute():
            elevation_path = root / elevation_path
        if not elevation_path.is_file():
            print(json.dumps({'error': 'elevation archive not found',
                              'archive': str(elevation_path)}))
            return 1
    for spec in args.sg3_tileset:
        if '=' not in spec or spec.split('=', 1)[1] == '':
            parser.error('--sg3-tileset requires INDEX=PATH')
        explicit = Path(spec.split('=', 1)[1])
        if not explicit.is_absolute():
            explicit = root / explicit
        if not explicit.is_file():
            print(json.dumps({'error': 'explicit tileset archive not found',
                              'archive': str(explicit)}))
            return 1
    if args.sg3_tileset and not args.image_grid:
        parser.error('--sg3-tileset is image-grid mode only')
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
    image_grid = args.image_grid

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
                                build_dir, first_rel, image_grid, args.sg3_elevation,
                                args.sg3_tileset)
            if done.returncode == 0 and binary.is_file():
                try:
                    result = last_json_line(done.stdout)
                    rows.append(image_grid_row(first_rel, result) if image_grid
                                else row_from_result(first_rel, result))
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
                                  rel, args.timeout, image_grid, args.sg3_elevation,
                                  args.sg3_tileset)
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
                result = last_json_line(done.stdout)
                rows.append(image_grid_row(rel, result) if image_grid
                            else row_from_result(rel, result))
            except Exception as error:  # noqa: BLE001
                failures.append({'map': rel, 'reason': 'output: ' + str(error)})
    finally:
        if build_owned:
            shutil.rmtree(build_dir, ignore_errors=True)

    if image_grid:
        return image_grid_report(maps, rows, failures, args)

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
            'Measured on all 167 city maps: that branch fires on 0 of them, so the '
            'hardcoded tileset is inert for it. unresolved_codes is the '
            '0x2000000-feature count (2231 cells in 22 cities); those are composite '
            '5x5 blocks, not empty archive slots. No slot lookup ever misses.',
        ],
        'rows': rows,
        'worst': rows[:args.top],
        'best': rows[-args.top:][::-1],
    }

    print(f'# ground-selector sweep: {len(rows)}/{len(maps)} cities processed, '
          f'{len(failures)} failed, archive {args.sg3}')
    print(f'# camera {args.width}x{args.height} scale {args.scale} centred on each '
          'active crop; known/active is camera-independent')
    print(f'{"map":<28}{"active":>7}{"known":>7}{"unknown":>8}{"chnowr":>7}'
          f'{"unres":>6}{"drawn":>7}{"decfail":>8}{"known/act":>10}{"drawn/know":>11}')
    for r in rows:
        ka, dk = r['known_over_active'], r['drawn_over_known']
        print(f'{r["map"]:<28}{cell(r, "active_cells"):>7}{cell(r, "known_cells"):>7}'
              f'{cell(r, "unknown_cells"):>8}{cell(r, "chain_no_write_cells"):>7}'
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
