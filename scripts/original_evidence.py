#!/usr/bin/env python3
"""Build/test a bounded SAV19 numeric exporter and qualify three explicit candidates.

Never launches the game, modifies a SAV, infers missing fields or requests a model.
Real inputs are only read beneath the per-process EMPEROR_GAMEDATA_ROOT.
"""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shlex
import stat
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCES = tuple('Save/2026102301/' + n + '.sav' for n in ('320110', '320111', '320112'))
# Exact finite application-level allowance. It creates no directory/file and
# grants neither an OS permission nor approval to execute the experiment.
CAPTURE_SOURCES = tuple('Save/2026102801/' + str(n) + '.sav' for n in range(280001, 280017))
READ_SOURCES = SOURCES + CAPTURE_SOURCES
MAX_INPUT = 4 * 1024 * 1024
BUDGET = {'total_cycles': 80, 'total_phase_steps': 4080, 'total_seconds': 1800,
          'market_cycles': 32, 'market_phase_steps': 1632, 'market_seconds': 900,
          'market_round_trips': 2}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def write_json(path, value):
    with path.open('x', encoding='utf-8') as f:
        json.dump(value, f, ensure_ascii=False, indent=2)
        f.write('\n')


def read_candidate(root, relative):
    """Resolve an exact manifest entry with openat/O_NOFOLLOW for every component."""
    if relative not in READ_SOURCES:
        raise ValueError('source not in the exact candidate/capture read manifest')
    if not root.is_absolute() or root.resolve() != root:
        raise ValueError('data root must be an absolute canonical directory')
    directory = os.open(root, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
    try:
        parts = Path(relative).parts
        for part in parts[:-1]:
            child = os.open(part, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW, dir_fd=directory)
            os.close(directory)
            directory = child
        fd = os.open(parts[-1], os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK, dir_fd=directory)
        try:
            before = os.fstat(fd)
            if relative in CAPTURE_SOURCES and before.st_nlink != 1:
                raise ValueError('capture must not alias another file')
            if not stat.S_ISREG(before.st_mode) or not 0 < before.st_size <= MAX_INPUT:
                raise ValueError('regular bounded SAV required')
            chunks, remaining = [], before.st_size
            while remaining:
                b = os.read(fd, min(65536, remaining))
                if not b:
                    raise ValueError('short read')
                chunks.append(b)
                remaining -= len(b)
            if os.read(fd, 1):
                raise ValueError('input grew while reading')
            after = os.fstat(fd)
            identity = lambda s: (s.st_dev, s.st_ino, s.st_size, s.st_mtime_ns, s.st_ctime_ns)
            if identity(before) != identity(after):
                raise ValueError('input changed while reading')
            return b''.join(chunks), {'device': before.st_dev, 'inode': before.st_ino,
                                     'bytes': before.st_size, 'mtime_ns': before.st_mtime_ns}
        finally:
            os.close(fd)
    finally:
        os.close(directory)


def capture_target_status(root, relative):
    """Stat an exact NEW target. Never create, copy, grant permission or overwrite."""
    if relative in SOURCES:
        raise ValueError('protected candidate cannot be a capture target')
    if relative not in CAPTURE_SOURCES:
        raise ValueError('target outside exact capture allocation')
    if not root.is_absolute() or root.resolve() != root:
        raise ValueError('canonical absolute data root required')
    directory = os.open(root, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
    missing = None
    try:
        for index, part in enumerate(Path(relative).parts[:-1]):
            try:
                child = os.open(part, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW, dir_fd=directory)
            except FileNotFoundError:
                if index != 1:
                    raise ValueError('known Save directory required')
                missing = str(Path(*Path(relative).parts[:index+1]))
                break
            os.close(directory)
            directory = child
        if missing is None:
            try:
                os.stat(Path(relative).name, dir_fd=directory, follow_symlinks=False)
            except FileNotFoundError:
                pass
            else:
                raise ValueError('capture target exists; overwrite forbidden')
    finally:
        os.close(directory)
    return {'target': relative, 'state': 'reserved_name_absent', 'missing_test_player_directory': missing,
            'experimental_authorization': False, 'system_permission_granted': False,
            'must_recheck_before_game_save': True, 'overwrite_dialog_must_be_refused': True}


def risk_counters(s, b):
    """Explicit Common5 contract; never infer causes from legacy field names.

    Historical reports need re-export, not an implicit reinterpretation. Signed
    negatives are valid wire values but are not evidence of a safe starting city.
    """
    if s.get('risk_contract') != 'common5-risk-v1' or s.get('risk_units') != 'signed16_accumulators_not_percentages':
        return None
    r = b.get('risk')
    if not isinstance(r, dict) or r.get('reader_crosscheck') is not True:
        return None
    fields = (('collapse_counter', 'common5_68_s16', 'fire_risk'),
              ('ignition_counter', 'common5_70_s16', 'damage_risk'),
              ('burning_counter', 'common5_72_s16', 'burning'))
    for canonical, raw, legacy in fields:
        values = (r.get(canonical), r.get(raw), b.get(legacy))
        if any(type(v) is not int or not -32768 <= v <= 32767 for v in values) or len(set(values)) != 1:
            return None
    return r


def qualify(s):
    """Snapshot qualification is distinct from dynamic experiment admission."""
    calendar, operating, market = [], [], []
    if s.get('error'):
        calendar.append(s['error'])
    for flag in ('building_table_complete', 'building_links_complete', 'building_inputs_complete', 'city_contract_complete'):
        if s.get(flag) is not True:
            calendar.append(flag + '_required')
    clock = s.get('clock') or {}
    if not clock.get('ranges_valid') or not all(type(clock.get(k)) is int and 0 <= clock[k] <= maximum
                                              for k, maximum in (('phase', 50), ('cycle', 0xffffffff), ('month_part', 15), ('month', 11))):
        calendar.append('qualified_clock_required')
    city, buildings = s.get('city') or {}, s.get('buildings') or []
    if type(city.get('year_word_wire')) is not int or type(city.get('scenario_start_year_wire')) is not int:
        calendar.append('separate_year_fields_required')
    if type(city.get('treasury')) is not int or city['treasury'] <= 0:
        calendar.append('positive_measurable_treasury_required')
    if not buildings or any(b.get('burning') is None or b['burning'] != 0 for b in buildings if b.get('alive') in (1, 3)):
        calendar.append('complete_nonburning_building_state_required')
    houses = [b for b in buildings if 2 <= b['type'] <= 17 and b.get('residents', 0) > 0]
    if not houses or city.get('population', 0) <= 0:
        operating.append('inhabited_city_required')
    risks = [risk_counters(s, b) for b in buildings if b.get('alive') in (1, 3)]
    if not risks or any(r is None for r in risks):
        calendar.append('explicit_crosschecked_common5_risk_contract_required')
    for r in risks:
        if r is None:
            continue
        for field, cause in (('collapse_counter', 'collapse'), ('ignition_counter', 'ignition')):
            if r[field] >= 1000:
                operating.append(cause + '_counter_at_project_branch_limit')
                calendar.append(cause + '_counter_at_project_branch_limit')
            elif r[field] < 0:
                calendar.append('negative_' + cause + '_counter_unqualified')
    for b in houses:
        if b.get('goods12_crosscheck') is not True or len(b.get('goods12') or []) != 12 or b.get('food_quality_raw') is None:
            operating.append('house_goods_quality_contract_required')
            break
        if any(n < 0 for n in b['goods12']):
            operating.append('negative_house_goods_unqualified')
            break
    by_type = lambda t: [b for b in buildings if b['type'] == t and b.get('alive') in (1, 3)]
    for t, name in ((53, 'mill'), (59, 'market'), (66, 'food_shop')):
        candidates = by_type(t)
        if not candidates:
            operating.append(name + '_required')
        elif not any(b.get('alive') == 1 and (t == 59 or b.get('workers', 0) > 0) and
                     (t != 53 or b.get('road_access', 0) > 0) and
                     b.get('operational', 0) > 0 and b.get('enabled', 0) > 0 for b in candidates):
            # Market parent has no workforce; shop parent/routing is separate
            # from its Common5 road-access byte. Do not invent a staffing gate.
            operating.append(name + '_active_staffing_operation_gate_not_met')
    mill_bays = [c for b in by_type(53) for c in (b.get('storage') or [])]
    if not mill_bays:
        operating.append('framed_mill_storage_required')
    elif not any(1 <= c['resource'] <= 9 and 0 < c['amount'] <= c['capacity_wire'] for c in mill_bays):
        operating.append('positive_qualified_mill_food_stock_required')
    if any(c['amount'] > c['capacity_wire'] for c in mill_bays):
        operating.append('mill_stock_exceeds_serialized_capacity')
    markets = by_type(59)
    if any(b.get('market_storage') is None or b.get('market_quality_raw') is None or
           len(b.get('market_demand') or []) != 7 for b in markets):
        operating.append('framed_market_storage_demand_quality_required')
    parent_slots = {b['slot'] for b in markets}
    if any(b.get('parent_slot') not in parent_slots for b in by_type(66)):
        operating.append('food_shop_parent_market_required')
    actors = s.get('actors') or {}
    if actors.get('base_pool_complete') is not True:
        market.append('complete_2000_actor_pool_required:' + actors.get('reason', 'missing'))
    if actors.get('selected_records_complete') is not True:
        market.append('selected_actor_export_incomplete')
    if actors.get('strict_reader_crosscheck') is not True:
        market.append('strict_actor_reader_crosscheck_required')
    for a in actors.get('records', []):
        if a['type'] in (23, 24) and (not a.get('record_complete') or not a.get('owner_building_known') or
                                    not a.get('route_reference_valid') or not a.get('cell_and_next_bounds_valid')):
            market.append('market_actor_record_or_owner_unqualified')
    # A single snapshot is never sufficient to authorize the proposed experiment.
    manual = ['same_scenario_and_test_only_save_identity', 'paused_exact_initial_state',
              'observed_type39_visits_and_both_risk_counter_trends', 'no_collapse_or_other_scene_pollution',
              'pre_first_dispatch_baseline_for_creation_claim', 'game_date_matches_serialized_year_fields',
              'market_compound_road_and_dispatch_verified']
    return {'calendar_snapshot_ready': not calendar,
            'operating_city_snapshot_ready': not calendar and not operating,
            'market_state_export_ready': not calendar and not operating and not market,
            'experiment_ready': False, 'calendar_blockers': sorted(set(calendar)),
            'operating_blockers': sorted(set(operating)), 'market_blockers': sorted(set(market)),
            'observed_market_actor_counts': {str(t): (actors.get('live_counts_in_read_prefix') or {}).get(str(t), 0)
                                             if actors.get('absence_certified') else None for t in (23, 24)},
            'risk_counter_maxima': {field: max((r[field] for r in risks if r is not None), default=None)
                                   for field in ('collapse_counter', 'ignition_counter')},
            'hazard_safety_certified': False,
            'risk_interpretation': 'Signed counters, not percentages; below1000 is not a safe observation horizon. Tower/workers/road bytes alone do not prove preventive visits. Missing legacy-only contracts require re-export.',
            'unverified_dynamic_gates': manual}


def verify_export(s):
    """Concentrated real-data structural checks, without hardcoded save values."""
    checks = 0
    def require(value, reason):
        nonlocal checks
        checks += 1
        if not value:
            raise ValueError('real export contract: ' + reason)
    require(s.get('schema') == 'original-evidence-v1' and not s.get('error'), 'snapshot domain')
    require(s.get('building_table_complete') and s.get('building_records') == 4000, '4000-building frame')
    require(s.get('building_links_complete') and s.get('building_inputs_complete'), 'building coverage')
    require(s.get('city_contract_complete'), 'city source contract')
    require((s.get('clock') or {}).get('ranges_valid'), 'clock ranges')
    require(type(s['city'].get('year_word_wire')) is int and type(s['city'].get('scenario_start_year_wire')) is int, 'separate year words')
    for b in s.get('buildings', []):
        require(risk_counters(s, b) is not None, 'Common5 signed risk raw/canonical/alias and reader equivalence')
        if 2 <= b['type'] <= 17:
            require(b.get('goods12_crosscheck') is True and len(b.get('goods12') or []) == 12, 'house signed-word offsets')
            require(type(b.get('food_quality_raw')) is int, 'raw house quality')
        if b['type'] == 53:
            require(len(b.get('storage') or []) == 8, 'Mill8 frame')
        if b['type'] == 59:
            require(len(b.get('market_storage') or []) == 6 and len(b.get('market_demand') or []) == 7 and
                    type(b.get('market_quality_raw')) is int, 'Market6/demand7/quality frame')
    actors = s.get('actors') or {}
    require(actors.get('base_pool_complete') and actors.get('strict_reader_crosscheck'), '2000-slot actor reader equivalence')
    require(s.get('goods_services') is not None, 'Goods1 and Population7 framed services')
    for actor in actors.get('records', []):
        if actor.get('route') == 0 and actor.get('route_cursor', 0) >= actor.get('route_length', 0):
            require(actor.get('route_reference_valid') is True and actor.get('saved_route') is None and
                    actor.get('route_cursor_is_array_index') is False, 'no-route retained counter is not an array index')
    return checks


def audit_series(entries, domain):
    """Check numeric observations without driving or authorizing the game."""
    if domain not in ('calendar', 'market', 'consumption_health') or not 2 <= len(entries) <= 256:
        raise ValueError('known domain and 2..256 explicit numeric observations required')
    issues, changes = [], []
    key = {'calendar': 'calendar_snapshot_ready', 'market': 'market_state_export_ready',
           'consumption_health': 'operating_city_snapshot_ready'}[domain]
    first = entries[0]['snapshot']
    start = first['clock']['cycle']*51 + first['clock']['phase']
    if entries[0]['elapsed_seconds'] != 0:
        raise ValueError('ledger must start at elapsed_seconds=0')
    last_step, last_seconds = start, 0
    limits = (BUDGET['market_cycles'], BUDGET['market_phase_steps'], BUDGET['market_seconds']) if domain == 'market' else (BUDGET['total_cycles'], BUDGET['total_phase_steps'], BUDGET['total_seconds'])
    last, bound_reached, last_trips = first, False, 0
    for index, entry in enumerate(entries):
        if bound_reached:
            issues.append('observation_%d_after_stop_bound' % index)
        s, seconds = entry['snapshot'], entry['elapsed_seconds']
        if s.get('schema') != 'original-evidence-v1' or not qualify(s)[key]:
            issues.append('observation_%d_not_qualified' % index)
        clock = s['clock']
        if not all(type(clock.get(k)) is int for k in ('cycle', 'phase')) or not 0 <= clock['phase'] <= 50:
            raise ValueError('qualified integer cycle/phase required')
        step = clock['cycle']*51 + clock['phase']
        if type(seconds) not in (int, float) or not 0 <= seconds < float('inf'):
            raise ValueError('finite nonnegative elapsed_seconds required')
        if seconds < last_seconds or step < last_step:
            issues.append('observation_%d_time_reversed' % index)
        if clock['cycle']-first['clock']['cycle'] > limits[0] or step-start > limits[1] or seconds > limits[2]:
            issues.append('observation_%d_budget_exceeded' % index)
        if domain == 'market':
            trips = entry.get('reported_complete_round_trips')
            if type(trips) is not int or trips < last_trips or trips > BUDGET['market_round_trips'] or (index == 0 and trips != 0):
                issues.append('observation_%d_round_trip_counter_unqualified' % index)
            else:
                last_trips = trips
                bound_reached |= trips == BUDGET['market_round_trips']
        bound_reached |= step-start >= limits[1] or seconds >= limits[2] or clock['cycle']-first['clock']['cycle'] >= limits[0]
        if s.get('active_side') != first.get('active_side') or (s.get('city') or {}).get('scenario_start_year_wire') != first['city'].get('scenario_start_year_wire'):
            issues.append('observation_%d_scenario_binding_changed' % index)
        if index:
            changes.append({'index': index, 'phase_steps_since_prior': step-last_step,
                            'changed_domains': [k for k in ('clock', 'city', 'buildings', 'actors', 'goods_services') if s.get(k) != last.get(k)]})
        last_step, last_seconds, last = step, seconds, s
    return {'schema': 'evidence-ledger-audit-v1', 'domain': domain, 'observations': len(entries),
            'numeric_checks_passed': not issues, 'issues': issues, 'changes': changes,
            'phase_steps': last_step-start, 'elapsed_seconds': last_seconds,
            'stop_at_bound': bound_reached,
            'experiment_permission': False, 'algorithm_proven': False,
            'remaining_gates': 'Manual pause, action/scene log, fire/patrol, scenario identity and endpoint-complete transfer balance still required; reported trip counts are not inferred proof.'}


def protocol(reports):
    """Executable preflight result, not a launch instruction or experimental permission."""
    return {'schema': 'evidence-protocol-v1', 'original_launch_permitted': False,
            'status': 'awaiting_parent_review_and_dynamic_gates', 'hard_limits_first_bound_wins': BUDGET,
            'allowed_variable': 'normal_game_clock_only; no wage/building/policy/recipe/quality changes',
            'candidate_results': [{'source': r['source'], 'sha256': r['input_sha256'],
                                   'qualification': r['qualification']} for r in reports],
            'read_points': {
                'calendar': ['paused baseline', 'phase49->50->0', 'monthPart14->15->0', 'month11->0'],
                'market': ['before actual23/24 dispatch', 'after source debit', 'arrival', 'after destination credit', 'cleanup'],
                'consumption_health': ['same-house before/after phase39', 'phase50 boundary', 'month boundary']},
            'compare_fields': ['clock.phase/cycle/month_part/month', 'city.year_word_wire/scenario_start_year_wire',
                               'buildings.risk.common5_68_s16/common5_70_s16/common5_72_s16 (signed accumulators, not percentages)',
                               'buildings.risk.collapse_counter/ignition_counter/burning_counter; spawn_counter and actual inspector39 visits',
                               'city.treasury/population/health_target/health_smoothed/health_rating',
                               'house goods12/food_quality_raw/water_service/herbalist/residents/type/model',
                               'mill storage and market storage/demand/quality', '23/24 all base_fields and owner/route/cargo links'],
            'stop_immediately': ['fire or collapse', 'uncontrolled population/scene contamination',
                                 'treasury<=0 or unmeasurable', 'parser refusal or source/scenario mismatch',
                                 'lost reliable pause', 'any hard limit'],
            'can_establish': ['serialized field changes and their observed ordering',
                              'cargo debit/in-flight/credit balance only when all endpoints and actors are qualified'],
            'cannot_establish': ['general monthly/health/recipe algorithms from finite traces',
                                 'undecoded variable FSA state', 'pre-first-dispatch history from a partial actor pool'],
            'capture_authorization': 'This tool never launches an experiment or grants authorization. Reads only explicitly selected manifest entries; default is the three candidates.',
            'capture_target_allowance': list(CAPTURE_SOURCES),
            'capture_gate': 'Application list only, independent of external task authorization and OS permissions. Require dedicated test player, fresh target absence and reject overwrite prompts. Never overwrite candidates or existing captures.',
            'funds_net_drop_stop': 200,
            'numeric_ledger_format': {'schema': 'evidence-series-v1', 'domain': 'calendar',
                                      'observations': [{'elapsed_seconds': 0, 'reported_complete_round_trips': 0,
                                                        'snapshot': '<numeric exported snapshot object>'}]},
            'check_numeric_ledger': 'python3 scripts/original_evidence.py --series <explicit-local-ledger.json> --output-dir <new-absolute-report-directory>',
            'ledger_constraints': '2..256 observations; domain calendar, market or consumption_health; monotonic elapsed_seconds, market trip counters explicitly reported',
            'reproduce_preflight': 'EMPEROR_GAMEDATA_ROOT=<canonical isolated Game-ddraw> python3 scripts/original_evidence.py --output-dir <new absolute local directory>'}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--mode', choices=['release', 'undefined'], default='release')
    p.add_argument('--output-dir', type=Path)
    p.add_argument('--skip-real', action='store_true')
    p.add_argument('--series', type=Path, help='audit an existing numeric JSON ledger only; no build, SAV reads or game')
    p.add_argument('--source', action='append', choices=READ_SOURCES, help='exact read-only source; defaults to three protected candidates')
    p.add_argument('--check-capture-targets', action='store_true', help='stat the16 allocated target names only; no build, game or file creation')
    args = p.parse_args()
    if (args.series or args.check_capture_targets) and args.source:
        p.error('--source cannot be combined with series/target-only checks')
    if args.series and args.check_capture_targets:
        p.error('choose one operation')
    if args.source and len(args.source) != len(set(args.source)):
        p.error('duplicate source selection')
    data_root = os.environ.get('EMPEROR_GAMEDATA_ROOT', '')
    if args.output_dir:
        if not args.output_dir.is_absolute():
            p.error('--output-dir must be absolute and new')
        out = args.output_dir
        if data_root and (Path(data_root).resolve() == out.resolve() or Path(data_root).resolve() in out.resolve().parents):
            p.error('reports/builds must be outside original data root')
        out.mkdir(parents=True, exist_ok=False)
    else:
        out = Path(tempfile.mkdtemp(prefix='original-evidence-'))
    if data_root and (Path(data_root).resolve() == out.resolve() or Path(data_root).resolve() in out.resolve().parents):
        raise ValueError('reports/builds must be outside original data root')
    report = {'mode': args.mode, 'started_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'passed': False, 'jobs': [], 'dependency_sha256': {}, 'inputs': {}, 'snapshots': [],
              'GUI': False, 'original_game_executed': False, 'external_models': False, 'ASan': False}
    dependencies, original = report['dependency_sha256'], report['inputs']

    if args.check_capture_targets:
        result = {'schema': 'capture-target-preflight-v1', 'game_executed': False, 'experimental_authorization': False,
                  'system_permission_granted': False, 'targets': []}
        if not data_root:
            result['SKIP'] = 'EMPEROR_GAMEDATA_ROOT unset'
        else:
            try:
                result['targets'] = [capture_target_status(Path(data_root), name) for name in CAPTURE_SOURCES]
            except (OSError, ValueError) as exc:
                result['error'] = str(exc)
        write_json(out / 'capture-targets.json', result)
        print(json.dumps({'report': str(out/'capture-targets.json'), 'targets_absent': len(result['targets']),
                          'error': result.get('error'), 'SKIP': result.get('SKIP')}))
        return 1 if result.get('error') else 0

    if args.series:
        series = args.series
        # macOS exposes /tmp and /var as system aliases. Normalize only that
        # root alias, then still reject symlinks within the supplied report path.
        for alias in (Path('/tmp'), Path('/var')):
            if alias in series.parents:
                series = alias.resolve() / series.relative_to(alias)
                break
        if not series.is_absolute() or series.resolve() != series:
            p.error('series must be an explicit canonical absolute JSON report')
        fd = os.open(series, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
        try:
            before = os.fstat(fd)
            if not stat.S_ISREG(before.st_mode) or not 0 < before.st_size <= 16*1024*1024:
                p.error('series must be a regular JSON report <=16 MiB')
            raw = b''
            while len(raw) < before.st_size:
                chunk = os.read(fd, min(65536, before.st_size-len(raw)))
                if not chunk:
                    p.error('series short read')
                raw += chunk
            after = os.fstat(fd)
            if os.read(fd, 1) or (before.st_size, before.st_mtime_ns, before.st_ctime_ns) != (after.st_size, after.st_mtime_ns, after.st_ctime_ns):
                p.error('series changed during read')
        finally:
            os.close(fd)
        data = json.loads(raw)
        if data.get('schema') != 'evidence-series-v1':
            p.error('numeric series schema required')
        result = audit_series(data['observations'], data['domain'])
        result['ledger_sha256'] = sha(raw)
        write_json(out / 'series-audit.json', result)
        print(json.dumps({'report': str(out / 'series-audit.json'), 'numeric_checks_passed': result['numeric_checks_passed']}))
        return 0 if result['numeric_checks_passed'] else 1

    def run(command, label, expected=0, **kwargs):
        with (out / (label + '.stdout')).open('xb') as stdout, (out / (label + '.stderr')).open('xb') as stderr:
            try:
                code = subprocess.run(command, stdout=stdout, stderr=stderr, timeout=180, **kwargs).returncode
            except subprocess.TimeoutExpired:
                code = 'timeout180'
        report['jobs'].append({'label': label, 'argv': command, 'exit': code, 'expected_exit': expected})
        print(json.dumps({'label': label, 'exit': code}), flush=True)
        if code != expected:
            raise RuntimeError(label + ' failed; logs retained at ' + str(out))

    frozen = {str(f.resolve()): sha(f.read_bytes()) for folder in ('game_lab', 'file_io/include', 'scripts')
              for f in (ROOT / folder).rglob('*') if f.is_file() and ROOT in f.resolve().parents and f.suffix in ('.hpp', '.h', '.inc', '.cpp', '.mm', '.py')}
    try:
        flags = ['-std=c++17', '-Wall', '-Wextra', '-Werror', '-I' + str(ROOT / 'game_lab')]
        flags += ['-O2'] if args.mode == 'release' else ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
        objects = []
        for name in ['original_evidence_export', 'original_map', 'original_terrain', 'original_save']:
            obj, dep = out / (name + '.o'), out / (name + '.d')
            run(['/usr/bin/clang++', *flags, '-MMD', '-MF', str(dep), '-c', str(ROOT / 'game_lab' / (name + '.cpp')), '-o', str(obj)], 'compile-' + name)
            objects.append(str(obj))
            for token in shlex.split(dep.read_text().replace('\\\n', ' '))[1:]:
                f = Path(token)
                if f.is_file() and ROOT in f.resolve().parents:
                    key = str(f.resolve())
                    dependencies[key] = frozen.get(key, sha(f.read_bytes()))
        binary = out / 'original-evidence-export'
        run(['/usr/bin/clang++', *flags, *objects, '-lz', '-o', str(binary)], 'link-exporter')
        report['binary'] = str(binary)
        report['binary_sha256'] = sha(binary.read_bytes())
        tests = out / 'original-evidence-tests'
        run(['/usr/bin/clang++', *flags, str(ROOT / 'game_lab/original_evidence_tests.cpp'), '-o', str(tests)], 'compile-tests')
        for f in (ROOT / 'game_lab/original_evidence_tests.cpp', Path(__file__).resolve(), ROOT / 'scripts/original_evidence_tests.py'):
            dependencies[str(f)] = frozen[str(f)]
        run([str(tests)], 'authored-format-tests')
        run(['/usr/bin/python3', '-B', str(ROOT / 'scripts/original_evidence_tests.py')], 'path-preflight-tests')
        run([str(binary)], 'malformed-container-refusal', expected=3, input=b'authored invalid SAV')
        if json.loads((out / 'malformed-container-refusal.stdout').read_text()).get('error') != 'supported_snapshot_required':
            raise RuntimeError('malformed container did not withhold snapshot domains')
        run([str(binary)], 'stdin-budget-refusal', expected=2, input=b'x'*(MAX_INPUT+1))
        if (out / 'stdin-budget-refusal.stdout').stat().st_size:
            raise RuntimeError('over-budget input leaked partial JSON')
        if args.skip_real or not data_root:
            report['SKIP'] = 'explicit --skip-real' if args.skip_real else 'EMPEROR_GAMEDATA_ROOT unset'
            print(json.dumps({'SKIP': report['SKIP']}), flush=True)
        else:
            root = Path(data_root)
            for i, source in enumerate(args.source or SOURCES):
                raw, identity = read_candidate(root, source)
                original[source] = {'sha256': sha(raw), 'identity': identity}
                run([str(binary)], 'candidate-' + str(i), input=raw)
                value = json.loads((out / ('candidate-' + str(i) + '.stdout')).read_text())
                value['structural_checks_passed'] = verify_export(value)
                value.update(source=source, input_sha256=sha(raw), input_identity=identity)
                value['source_role'] = 'protected_candidate' if source in SOURCES else 'reserved_capture'
                value['qualification'] = qualify(value)
                write_json(out / ('candidate-' + str(i) + '.json'), value)
                report['snapshots'].append(value)
            report['protocol'] = str(out / 'protocol.json')
            write_json(out / 'protocol.json', protocol(report['snapshots']))
            for source, before in original.items():
                raw, identity = read_candidate(root, source)
                if sha(raw) != before['sha256'] or identity != before['identity']:
                    raise RuntimeError('input changed during export: ' + source)
            report['inputs_unchanged'] = True
        report['passed'] = True
    except (OSError, ValueError, RuntimeError, KeyError) as e:
        report['error'] = str(e)
    if original:
        try:
            for source, before in original.items():
                raw, identity = read_candidate(Path(data_root), source)
                if sha(raw) != before['sha256'] or identity != before['identity']:
                    raise RuntimeError('input changed after export: ' + source)
            report['inputs_unchanged'] = True
        except (OSError, ValueError, RuntimeError) as e:
            report['inputs_unchanged'] = False
            report['passed'] = False
            report['protection_error'] = str(e)
    report['source_stable'] = all(Path(f).is_file() and sha(Path(f).read_bytes()) == h for f, h in dependencies.items())
    report['passed'] = report['passed'] and report['source_stable']
    report['finished_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    write_json(out / 'verification.json', report)
    print(json.dumps({'passed': report['passed'], 'report': str(out / 'verification.json')}), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
