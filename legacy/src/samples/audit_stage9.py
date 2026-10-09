#!/usr/bin/env python3
"""Read-only final source audit and structural save-frame comparison."""
import importlib.util
import json
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('own_previous_audit', ROOT / 'samples/audit_stage8.py')
previous = importlib.util.module_from_spec(spec)
spec.loader.exec_module(previous)


def main():
    p = previous.PREFLIGHT
    source_checks = []
    for relative, expected in p['source_config_hashes'].items():
        source_checks.append(previous.check(previous.SOURCE / relative, expected))
    for rec in p['source_exes']:
        source_checks.append(previous.check(previous.GAME / rec['name'], rec['sha256']))
    for rec in p['source_saves']:
        source_checks.append(previous.check(previous.GAME / rec['path'], rec['sha256']))
    for name, rec in p['source_scripts'].items():
        source_checks.append(previous.check(previous.SOURCE / 'Resources/Scripts' / name, rec['sha256']))
    source_checks.append(previous.check(previous.SOURCE / 'MacOS/wineskinlauncher', p['native_hashes']['launcher']))
    source_checks.append(previous.check(previous.SOURCE / 'SharedSupport/wine/bin/wine64', p['native_hashes']['wine64']))
    inventory = previous.STAGE7
    project_checks = [previous.check(Path(inventory['source_exe']), inventory['exe_sha256']),
                      previous.check(Path(inventory['selected_save']), inventory['save_sha256'])]
    candidates = json.loads((ROOT / 'evidence/stage9-save-candidates.json').read_text())['candidates']
    candidate_checks = [previous.check(Path(rec['frozen_copy']), rec['sha256']) for rec in candidates]
    copied_input = previous.COPY_GAME / 'Save/zoom/Stage9-Candidate2.sav'
    candidate_checks.append(previous.check(copied_input, candidates[1]['sha256']))
    copy_exes = [previous.check(previous.COPY_GAME / rec['name'], rec['sha256']) for rec in p['source_exes']]
    dll_meta = json.loads((ROOT / 'evidence/stage9-dllhelper-metadata.json').read_text())
    dll_checks = [previous.check(previous.GAME / 'dllhelper.dll', dll_meta['sha256']),
                  previous.check(previous.COPY_GAME / 'dllhelper.dll', dll_meta['sha256'])]
    outputs, preserved, raw, decoded, framing = {}, {}, {}, {}, {}
    raw['candidate2'] = Path(candidates[1]['frozen_copy']).read_bytes()
    for name in ['ST9BASE', 'ST9AFTR']:
        rec = json.loads((ROOT / f'evidence/stage9-{name.lower()}-file.json').read_text())
        outputs[name] = [previous.check(Path(rec['path']), rec['sha256']),
                         previous.check(ROOT / f'stage9-data/{name}.sav', rec['sha256'])]
        raw[name] = (ROOT / f'stage9-data/{name}.sav').read_bytes()
    for name in ['ST8BASE', 'ST8AFTR']:
        rec = json.loads((ROOT / f'evidence/stage8-{name.lower()}-file.json').read_text())
        preserved[name] = [previous.check(Path(rec['path']), rec['sha256']),
                           previous.check(ROOT / f'stage8-data/{name}.sav', rec['sha256'])]
    for name, data in raw.items():
        framing[name], decoded[name] = previous.frames(data)
    comparisons = {}
    for a, b in [('candidate2', 'ST9BASE'), ('ST9BASE', 'ST9AFTR'), ('candidate2', 'ST9AFTR')]:
        comparisons[f'{a}_to_{b}'] = dict(file_bytes=previous.compare(raw[a], raw[b]),
                                        decoded_frame_bytes=previous.compare(decoded[a], decoded[b]))
    all_output = [x for checks in outputs.values() for x in checks]
    all_preserved = [x for checks in preserved.values() for x in checks]
    result = dict(created_utc=datetime.now(timezone.utc).isoformat(),
                  source_app_checks=source_checks, original_project_checks=project_checks,
                  original_preflight_files_unchanged=all(x['unchanged'] for x in source_checks + project_checks),
                  original_preflight_check_count=len(source_checks + project_checks),
                  dll_source_copy_checks=dll_checks,
                  dll_note='DLL first hash was recorded during stage9. Matching original and copy is proven; it was not part of the earlier 21-file preflight.',
                  copied_EXE_checks=copy_exes, copied_EXEs_unchanged=all(x['unchanged'] for x in copy_exes),
                  candidate_checks=candidate_checks, candidates_unchanged=all(x['unchanged'] for x in candidate_checks),
                  outputs=outputs, outputs_match_frozen=all(x['unchanged'] for x in all_output),
                  preserved_stage8_outputs=preserved, stage8_outputs_unchanged=all(x['unchanged'] for x in all_preserved),
                  framing=framing, comparisons=comparisons,
                  interpretation='Framing and positional byte comparisons only. Variable payload lengths can shift later bytes. No population, treasury, calendar, tile, object or save-field offset is inferred.')
    (ROOT / 'evidence/stage9-final-audit.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n')
    for key in ['original_preflight_files_unchanged', 'copied_EXEs_unchanged', 'candidates_unchanged', 'outputs_match_frozen', 'stage8_outputs_unchanged']:
        assert result[key], key
    assert all(x['unchanged'] for x in dll_checks)
    assert all(rec['frame_count'] == 136 and rec['fully_framed'] for rec in framing.values())
    print(json.dumps({k:v for k,v in result.items() if k in ['original_preflight_files_unchanged', 'original_preflight_check_count', 'copied_EXEs_unchanged', 'candidates_unchanged', 'outputs_match_frozen', 'stage8_outputs_unchanged', 'comparisons']}, indent=2))
    print(json.dumps({name:{k:v for k,v in rec.items() if k != 'frames'} for name,rec in framing.items()}, indent=2))


if __name__ == '__main__':
    main()
