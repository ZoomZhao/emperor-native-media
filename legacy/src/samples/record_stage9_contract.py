#!/usr/bin/env python3
"""Record observed original-game state separately from unproved binary fields."""
import json
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    versions = json.loads((ROOT / 'evidence/stage9-version-mapping.json').read_text())
    audit = json.loads((ROOT / 'evidence/stage9-final-audit.json').read_text())
    candidates = json.loads((ROOT / 'evidence/stage9-save-candidates.json').read_text())['candidates']
    base = json.loads((ROOT / 'evidence/stage9-st9base-file.json').read_text())
    after = json.loads((ROOT / 'evidence/stage9-st9aftr-file.json').read_text())
    state_contract = dict(
        schema='stage9-observed-city-v1',
        execution_kind='Original PE32/i386 Chinese 1.0.1.0 under the explicitly approved existing bundled Wine. No native city conversion run.',
        executed_EXE=versions['inputs']['CH'], canonical_future_candidate=versions['inputs']['EN'],
        nonempty_source=candidates[1],
        file_states=[dict(name='ST9BASE', sha256=base['sha256'], size=base['size'],
                          observed_at_save_dialog=dict(population=28, money=1633, calendar='2038 BCE September'),
                          save_dialog_evidence='evidence/stage8-s9b-base-name-check.png',
                          reloaded_first_observed=dict(population=28, money=1633, calendar='2038 BCE September', housing_capacity=70, monthly_new_immigrants=0, added_housing_at_ui_point=False),
                          reload_pid=63550, reload_evidence='evidence/stage8-s9f-base-city-freeze.png'),
                     dict(name='ST9AFTR', sha256=after['sha256'], size=after['size'],
                          observed_at_save_dialog=dict(population=77, money=1604, calendar='2038 BCE November'),
                          save_dialog_evidence='evidence/stage8-s9b-after-name-check.png',
                          reloaded_first_observed=dict(population=77, money=1604, calendar='2038 BCE November', housing_capacity=7, monthly_new_immigrants=12, added_housing_at_ui_point=True),
                          reload_pid=63550, reload_evidence='evidence/stage8-s9f-after-city-freeze.png')],
        production_game_pid=60192,
        actions=dict(normal_tool='平民住宅', displayed_cost=15, single_placement_click=[430,350],
                     coordinate_kind='point in inspected 1024x796 window; world tile coordinates not recovered',
                     green_preview_evidence='evidence/stage8-s9b-house-preview2.png',
                     click_evidence='evidence/stage8-ui-s9b-build-house.json',
                     post_save_lot_evidence='evidence/stage8-s9b-exit-menu.png',
                     collapse_popup_present_on_initial_observation=True,
                     fire_popup_later_observed=True,
                     cheats_or_guest_memory_writes=False),
        timing=dict(game_speed_observed_percent=80, scroll_speed_observed_percent=60,
                    settings_changed=False, shortcut_pause_proved=False,
                    repeat_reload_click_delay_seconds=0.25,
                    paused_or_exact_scheduler_tick_proved=False,
                    capture_method='one own native UI script clicks normal Load, waits 0.25 seconds, opens File menu and captures; visible values are observations, not decoded save fields',
                    earlier_BASE_reload=dict(pid=62136, first_observed_population=52, money=1633, calendar='2038 BCE September',
                                             screenshot='evidence/stage8-s9r-base-city.png',
                                             interpretation='Delayed observation after normal reload. Immediate repeat observed 28. Automatic resumed simulation explains the difference plausibly; no corrupt-save inference.')),
        binary_binding=dict(population_save_offset=None, money_save_offset=None, calendar_save_offset=None,
                            housing_object_record_offset=None, world_tile_coordinates=None,
                            field_to_runtime_VA_proved=False,
                            precise_tick_and_rng_state=None,
                            archive_structure='136 complete bounded zlib frames with fedcbaaa/abcdefff wrapper; payload lengths differ',
                            original_loader_root='0x534a30', original_deserializer='0x533f20',
                            original_save_root=None,
                            mapping_manifest='evidence/stage9-version-mapping.json'),
        next_native_acceptance=dict(entry='original load and post-load path',
                                    input='fixed nonempty save compatible with the one selected canonical executable',
                                    gate_and_phase='observe natural initialized values; no manually forced gate, phase, empty containers or fake successful APIs',
                                    observable='original object graph and defined scalar state after load, one natural scheduler cycle, original serialization, fresh process restore and continuation',
                                    reference='original i386 byte execution with evidenced platform boundaries; Wine UI baseline complements it but does not prove Windows API equivalence',
                                    required_comparison=['32-bit address domain and valid object handles', 'defined/control flags and registers', 'stack and image/BSS/heap state', 'actual call and callback targets', 'specified state fields and serialization effects'],
                                    stop='Unclosed load/indirect target/exception dependency or only hand-written game rules and bypasses can achieve success.'))
    # Resolve the actual historic filenames without inventing an evidence link.
    screenshots = sorted((ROOT / 'evidence').glob('stage8-s9b-*base*name*.png'))
    if len(screenshots) == 1:
        state_contract['file_states'][0]['save_dialog_evidence'] = str(screenshots[0].relative_to(ROOT))
    for state in state_contract['file_states']:
        assert (ROOT / state['save_dialog_evidence']).is_file()
        assert (ROOT / state['reload_evidence']).is_file()
    click = json.loads((ROOT / state_contract['actions']['click_evidence']).read_text())
    assert click['action'] == 'click' and click['x'] == 430 and click['y'] == 350
    (ROOT / 'evidence/stage9-city-state-contract.json').write_text(json.dumps(state_contract, ensure_ascii=False, indent=2) + '\n')
    findings = dict(created_utc=datetime.now(timezone.utc).isoformat(), stage='9 completed bounded baseline and static mapping',
                    nonempty_city_original_execution=True,
                    normal_housing_object_placement_and_persistence_observed=True,
                    population_treasury_calendar_ui_restored_in_new_process=True,
                    save_binary_field_semantics_proved=False,
                    arm64_real_city_execution_proved=False, full_native_game_proved=False,
                    source_audit=dict(preflight_count=audit['original_preflight_check_count'], original_files_unchanged=audit['original_preflight_files_unchanged'],
                                      frozen_outputs_match=audit['outputs_match_frozen'], stage8_outputs_preserved=audit['stage8_outputs_unchanged']),
                    version_mapping=dict(code_windows=27, core_windows_file_bytes_identical=True, known_registered_DLL_code_hooks=8, core_window_direct_patch_overlaps=0,
                                         DLL_transitive_data_effects_fully_proved=False, complete_runtime_CN_EN_equivalence_proved=False),
                    canonical_recommendation=dict(version='English 1.0.1.0', sha256=versions['inputs']['EN']['sha256'],
                                                  reason='Exactly the existing stage1-7 source bytes; avoids the additional CN entry LoadLibrary and its registered hooks. This is a recommendation for the next fixed version, not evidence that the English full game or these saves ran.',
                                                  English_actual_save_compatibility_proved=False),
                    cleanup='normal Exit Yes; no inspected game windows or recorded game/launcher PIDs; original Shuangpin restored',
                    report='reports/stage9-nonempty-city-and-version-mapping.md',
                    evidence=['evidence/stage9-city-state-contract.json','evidence/stage9-version-mapping.json','evidence/stage9-final-audit.json','evidence/stage9-cleanup.json'],
                    remaining_uncertainty=['canonical English nonempty save compatibility and original runtime baseline', 'actual serialization field/VA/object bindings and exact scheduler/RNG state', 'CRT/Win32/SEH/heap/callback load dependency closure', 'real original load-update-save chain translated to arm64 and independently compared'],
                    permission_blocker=None, automatic_approval_rejection=None)
    (ROOT / 'evidence/stage9-final-findings.json').write_text(json.dumps(findings, ensure_ascii=False, indent=2) + '\n')
    print(json.dumps(dict(nonempty_city=True, restored_BASE_population=28, restored_AFTR_population=77,
                          native_city_pass=False, original_files_unchanged=audit['original_preflight_files_unchanged']), indent=2))


if __name__ == '__main__':
    main()
