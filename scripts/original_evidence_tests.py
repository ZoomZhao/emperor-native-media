#!/usr/bin/env python3
"""Authored I/O and admission counterexamples; no original data or GUI."""
import copy
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from original_evidence import SOURCES, CAPTURE_SOURCES, READ_SOURCES, MAX_INPUT, BUDGET, read_candidate, capture_target_status, qualify, protocol, audit_series, risk_counters


def city_fixture():
    def b(slot, kind):
        return {'slot': slot, 'type': kind, 'alive': 1, 'workers': 1, 'road_access': 1,
                'operational': 1, 'enabled': 1, 'burning': 0, 'fire_risk': 0, 'damage_risk': 0,
                'risk': {'common5_68_s16': 0, 'common5_70_s16': 0, 'common5_72_s16': 0,
                         'collapse_counter': 0, 'ignition_counter': 0, 'burning_counter': 0, 'reader_crosscheck': True}}
    house = b(1, 3)
    house.update(residents=7, goods12=[0]*12, food_quality_raw=0, goods12_crosscheck=True)
    mill, market, shop = b(2, 53), b(3, 59), b(4, 66)
    mill['storage'] = [{'resource': 1, 'amount': 100, 'capacity_wire': 400}]
    market.update(market_storage=[], market_quality_raw=0, market_demand=[0]*7)
    shop['parent_slot'] = 3
    return {'risk_contract': 'common5-risk-v1', 'risk_units': 'signed16_accumulators_not_percentages',
            'building_table_complete': True, 'building_links_complete': True,
            'building_inputs_complete': True, 'city_contract_complete': True,
            'schema': 'original-evidence-v1', 'active_side': 100,
            'clock': {'ranges_valid': True, 'phase': 0, 'cycle': 0, 'month_part': 0, 'month': 0},
            'city': {'population': 7, 'treasury': 100, 'year_word_wire': 0, 'scenario_start_year_wire': -1000},
            'buildings': [house, mill, market, shop],
            'actors': {'base_pool_complete': True, 'strict_reader_crosscheck': True, 'selected_records_complete': True, 'absence_certified': True, 'records': []}}


class EvidenceTests(unittest.TestCase):
    def test_risk_counter_causes_and_no_safe_probability(self):
        for field, raw, legacy, cause in (('collapse_counter', 'common5_68_s16', 'fire_risk', 'collapse'),
                                          ('ignition_counter', 'common5_70_s16', 'damage_risk', 'ignition')):
            for alive in (1, 3):
                for n in (0, 918, 999, 1000, 32767):
                    s = city_fixture(); b = s['buildings'][0]; b['alive'] = alive
                    b['risk'][field] = b['risk'][raw] = b[legacy] = n
                    q = qualify(s)
                    self.assertIsNotNone(risk_counters(s, b))
                    self.assertEqual(q['risk_counter_maxima'][field], n)
                    self.assertEqual(cause + '_counter_at_project_branch_limit' in q['operating_blockers'], n >= 1000)
                    self.assertFalse(q['experiment_ready']); self.assertFalse(q['hazard_safety_certified'])
                    self.assertNotIn('collapse_risk_at_limit', q['operating_blockers'])

    def test_risk_missing_or_conflicting_contract_is_not_safe(self):
        for field in ('risk_contract', 'risk_units'):
            s = city_fixture(); del s[field]
            self.assertFalse(qualify(s)['calendar_snapshot_ready'])
        for field in ('common5_68_s16', 'common5_70_s16', 'common5_72_s16',
                      'collapse_counter', 'ignition_counter', 'burning_counter', 'reader_crosscheck'):
            for value in (None, True, '918', 32768, -32769, 1):
                s = city_fixture(); b = s['buildings'][0]; b['risk'][field] = value
                if field == 'reader_crosscheck' and value is True:
                    continue
                self.assertIsNone(risk_counters(s, b), (field, value))
                self.assertFalse(qualify(s)['calendar_snapshot_ready'])
        for field in ('fire_risk', 'damage_risk', 'burning'):
            s = city_fixture(); s['buildings'][0][field] = 1
            self.assertFalse(qualify(s)['calendar_snapshot_ready'])
        for field, raw, legacy in (('collapse_counter','common5_68_s16','fire_risk'),
                                  ('ignition_counter','common5_70_s16','damage_risk')):
            s = city_fixture(); b = s['buildings'][0]
            b['risk'][field] = b['risk'][raw] = b[legacy] = -32768
            self.assertIsNotNone(risk_counters(s, b))
            self.assertFalse(qualify(s)['calendar_snapshot_ready'])

    def test_exact_future_allowance_never_creates_or_grants(self):
        self.assertEqual(len(CAPTURE_SOURCES), 16)
        self.assertEqual(len(READ_SOURCES), 19)
        self.assertTrue(set(SOURCES).isdisjoint(CAPTURE_SOURCES))
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp).resolve()
            (root/'Save').mkdir()
            for name in CAPTURE_SOURCES:
                result = capture_target_status(root,name)
                self.assertEqual(result['state'], 'reserved_name_absent')
                self.assertFalse(result['system_permission_granted'])
                self.assertFalse(result['experimental_authorization'])
                self.assertIsNotNone(result['missing_test_player_directory'])
                self.assertFalse((root/name).exists())
            self.assertFalse((root/'Save/2026102801').exists())

    def test_protected_candidates_cannot_be_capture_targets(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp).resolve()
            for name in SOURCES:
                target=root/name
                target.parent.mkdir(parents=True,exist_ok=True)
                target.write_bytes(b'protected candidate')
                before=target.read_bytes()
                with self.assertRaisesRegex(ValueError,'protected candidate'):
                    capture_target_status(root,name)
                self.assertEqual(target.read_bytes(),before)

    def test_missing_future_input_and_existing_target_collision(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp).resolve()
            target=root/CAPTURE_SOURCES[0]
            (root/'Save').mkdir()
            with self.assertRaises(FileNotFoundError):
                read_candidate(root,CAPTURE_SOURCES[0])
            target.parent.mkdir()
            self.assertIsNone(capture_target_status(root,CAPTURE_SOURCES[0])['missing_test_player_directory'])
            target.write_bytes(b'authored new capture')
            self.assertEqual(read_candidate(root,CAPTURE_SOURCES[0])[0],b'authored new capture')
            with self.assertRaisesRegex(ValueError,'overwrite forbidden'):
                capture_target_status(root,CAPTURE_SOURCES[0])
            self.assertEqual(target.read_bytes(),b'authored new capture')

    def test_future_target_exact_scope_and_symlink_refusals(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp).resolve()
            (root/'Save').mkdir()
            for name in ('Save/2026102801/280017.sav','Save/2026102801/*.sav',
                         'Save/2026102801/../2026102301/320112.sav','/Save/2026102801/280001.sav',
                         'Save/other/280001.sav','Save/2026102801/subdir/280001.sav'):
                with self.assertRaises(ValueError):
                    capture_target_status(root,name)
                with self.assertRaises(ValueError):
                    read_candidate(root,name)
            outside=root/'authored-other';outside.mkdir()
            (root/'Save/2026102801').symlink_to(outside)
            with self.assertRaises(OSError):
                capture_target_status(root,CAPTURE_SOURCES[0])
            with self.assertRaises(OSError):
                read_candidate(root,CAPTURE_SOURCES[0])

    def test_future_capture_alias_and_final_symlink_refusals(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp).resolve();target=root/CAPTURE_SOURCES[0];target.parent.mkdir(parents=True)
            original=root/SOURCES[0];original.parent.mkdir(parents=True);original.write_bytes(b'protected')
            os.link(original,target)
            with self.assertRaisesRegex(ValueError,'alias'):
                read_candidate(root,CAPTURE_SOURCES[0])
            with self.assertRaises(ValueError):
                capture_target_status(root,CAPTURE_SOURCES[0])
            target.unlink();target.symlink_to(original)
            with self.assertRaises(OSError):
                read_candidate(root,CAPTURE_SOURCES[0])
            with self.assertRaises(ValueError):
                capture_target_status(root,CAPTURE_SOURCES[0])
            self.assertEqual(original.read_bytes(),b'protected')

    def test_read_exact_manifest_and_identity(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp).resolve()
            f = root / SOURCES[0]
            f.parent.mkdir(parents=True)
            f.write_bytes(b'authored')
            raw, identity = read_candidate(root, SOURCES[0])
            self.assertEqual(raw, b'authored')
            self.assertEqual(identity['bytes'], 8)
            self.assertEqual(read_candidate(root, SOURCES[0]), (raw, identity))
            for name in ('../secret', '/etc/passwd', 'Save/other.sav'):
                with self.assertRaises(ValueError):
                    read_candidate(root, name)

    def test_symlink_and_nonregular_refused(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp).resolve()
            f = root / SOURCES[0]
            f.parent.mkdir(parents=True)
            target = root / 'authored'
            target.write_bytes(b'a')
            f.symlink_to(target)
            with self.assertRaises(OSError):
                read_candidate(root, SOURCES[0])
            f.unlink()
            os.mkfifo(f)
            with self.assertRaises(ValueError):
                read_candidate(root, SOURCES[0])
            f.unlink()
            f.write_bytes(b'')
            with self.assertRaises(ValueError):
                read_candidate(root, SOURCES[0])
            with f.open('wb') as out:
                out.truncate(MAX_INPUT+1)
            with self.assertRaises(ValueError):
                read_candidate(root, SOURCES[0])

    def test_symlink_parent_refused(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp).resolve()
            target = root / 'actual'
            target.mkdir()
            (root / 'Save').symlink_to(target)
            with self.assertRaises(OSError):
                read_candidate(root, SOURCES[0])

    def test_mutation_during_read_refused(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp).resolve()
            f = root / SOURCES[0]
            f.parent.mkdir(parents=True)
            f.write_bytes(b'original')
            actual_read = os.read
            modified = False
            def mutate_after_first_read(fd, size):
                nonlocal modified
                value = actual_read(fd, size)
                if not modified:
                    modified = True
                    with f.open('ab') as out:
                        out.write(b'x')
                return value
            with patch('original_evidence.os.read', side_effect=mutate_after_first_read):
                with self.assertRaises(ValueError):
                    read_candidate(root, SOURCES[0])

    def test_valid_snapshot_is_not_experiment_authorization(self):
        s = city_fixture()
        before = copy.deepcopy(s)
        q = qualify(s)
        self.assertTrue(q['calendar_snapshot_ready'])
        self.assertTrue(q['operating_city_snapshot_ready'])
        self.assertTrue(q['market_state_export_ready'])
        self.assertFalse(q['experiment_ready'])
        self.assertEqual(s, before)

    def test_missing_is_never_zero_or_absent(self):
        for flag in ('building_table_complete', 'building_links_complete', 'building_inputs_complete', 'city_contract_complete'):
            s = city_fixture()
            s[flag] = False
            self.assertFalse(qualify(s)['calendar_snapshot_ready'])
        for value in (None, 0, -1):
            s = city_fixture()
            s['city']['treasury'] = value
            self.assertFalse(qualify(s)['calendar_snapshot_ready'])
        s = city_fixture()
        s['actors'] = {'base_pool_complete': False, 'reason': 'variable_fsa_unqualified', 'records': []}
        self.assertFalse(qualify(s)['market_state_export_ready'])
        self.assertIn('variable_fsa_unqualified', qualify(s)['market_blockers'][0])

    def test_operating_counterexamples(self):
        changes = [('workers', 0), ('road_access', 0), ('operational', 0), ('enabled', 0)]
        for key, value in changes:
            s = city_fixture()
            s['buildings'][1][key] = value
            self.assertFalse(qualify(s)['operating_city_snapshot_ready'])
        for amount in (0, 401):
            s = city_fixture()
            s['buildings'][1]['storage'][0]['amount'] = amount
            self.assertFalse(qualify(s)['operating_city_snapshot_ready'])
        s = city_fixture()
        s['buildings'][0]['goods12_crosscheck'] = False
        self.assertFalse(qualify(s)['operating_city_snapshot_ready'])
        s = city_fixture()
        s['buildings'][3]['parent_slot'] = 999
        self.assertFalse(qualify(s)['operating_city_snapshot_ready'])
        s = city_fixture()
        s['buildings'][0]['burning'] = 1
        self.assertFalse(qualify(s)['calendar_snapshot_ready'])

    def test_protocol_limits_and_no_launch(self):
        s = city_fixture()
        s.update(source=SOURCES[0], input_sha256='authored', qualification=qualify(s))
        plan = protocol([s])
        self.assertFalse(plan['original_launch_permitted'])
        self.assertEqual(plan['hard_limits_first_bound_wins'], BUDGET)
        self.assertEqual(BUDGET['total_cycles']*51, BUDGET['total_phase_steps'])
        self.assertIn('fire or collapse', plan['stop_immediately'])
        self.assertTrue(json.loads(json.dumps(plan)) == plan)

    def test_compound_market_is_not_a_staffed_shop(self):
        s = city_fixture()
        s['buildings'][2]['workers'] = 0
        s['buildings'][3]['road_access'] = 0
        self.assertTrue(qualify(s)['operating_city_snapshot_ready'])
        s['buildings'][3]['parent_common_wire'] = 3
        s['buildings'][3]['parent_slot'] = None
        self.assertFalse(qualify(s)['operating_city_snapshot_ready'])

    def test_semantic_and_export_budget_refusals(self):
        s = city_fixture()
        s['buildings'][1]['storage'][0]['resource'] = 10
        self.assertFalse(qualify(s)['operating_city_snapshot_ready'])
        for flag in ('base_pool_complete', 'selected_records_complete', 'strict_reader_crosscheck'):
            s = city_fixture()
            s['actors'][flag] = False
            self.assertFalse(qualify(s)['market_state_export_ready'])
        for flag in ('record_complete', 'owner_building_known', 'route_reference_valid', 'cell_and_next_bounds_valid'):
            s = city_fixture()
            actor = dict(type=24, record_complete=True, owner_building_known=True, route_reference_valid=True, cell_and_next_bounds_valid=True)
            actor[flag] = False
            s['actors']['records'] = [actor]
            self.assertFalse(qualify(s)['market_state_export_ready'])
        for field, value in (('month', 12), ('phase', 51), ('month_part', -1), ('cycle', True)):
            s = city_fixture()
            s['clock'][field] = value
            self.assertFalse(qualify(s)['calendar_snapshot_ready'])

    def test_series_hard_bounds_reverse_and_scenario(self):
        a, b = city_fixture(), city_fixture()
        b['clock']['cycle'] = 80
        entries = [{'elapsed_seconds': 0, 'snapshot': a}, {'elapsed_seconds': 1800, 'snapshot': b}]
        audit = audit_series(entries, 'calendar')
        self.assertTrue(audit['numeric_checks_passed'])
        self.assertTrue(audit['stop_at_bound'])
        b['clock']['phase'] = 1
        self.assertFalse(audit_series(entries, 'calendar')['numeric_checks_passed'])
        b['clock']['cycle'] = 0
        entries[1]['elapsed_seconds'] = 1801
        self.assertFalse(audit_series(entries, 'calendar')['numeric_checks_passed'])
        entries[1]['elapsed_seconds'] = 1
        a['clock']['cycle'] = 1
        self.assertFalse(audit_series(entries, 'calendar')['numeric_checks_passed'])
        a['clock']['cycle'] = 0
        b['city']['scenario_start_year_wire'] = -999
        self.assertFalse(audit_series(entries, 'calendar')['numeric_checks_passed'])
        entries[1]['elapsed_seconds'] = float('nan')
        with self.assertRaises(ValueError):
            audit_series(entries, 'calendar')

    def test_market_uses_stricter_bound(self):
        a, b = city_fixture(), city_fixture()
        b['clock']['cycle'] = 33
        entries = [{'elapsed_seconds': 0, 'snapshot': a}, {'elapsed_seconds': 10, 'snapshot': b}]
        self.assertTrue(audit_series(entries, 'calendar')['numeric_checks_passed'])
        self.assertFalse(audit_series(entries, 'market')['numeric_checks_passed'])

    def test_market_two_trips_is_a_stop_not_algorithm_proof(self):
        a, b = city_fixture(), city_fixture()
        b['clock']['cycle'] = 1
        entries = [{'elapsed_seconds': 0, 'reported_complete_round_trips': 0, 'snapshot': a},
                   {'elapsed_seconds': 10, 'reported_complete_round_trips': 2, 'snapshot': b}]
        report = audit_series(entries, 'market')
        self.assertTrue(report['numeric_checks_passed'])
        self.assertTrue(report['stop_at_bound'])
        self.assertFalse(report['algorithm_proven'])
        entries.append(copy.deepcopy(entries[-1]))
        self.assertFalse(audit_series(entries, 'market')['numeric_checks_passed'])


if __name__ == '__main__':
    unittest.main()
