#!/usr/bin/env python3
"""Read-only final evidence audit; does not import or execute third-party code."""
import hashlib, json, struct, zlib
from pathlib import Path

root = Path(__file__).resolve().parents[1]
inventory = json.loads((root / 'evidence/stage7-input-inventory.json').read_text())
checkpoint_path = root / 'evidence/stage7-checkpoint-crt-city-initialized-io-final.json'
checkpoint = json.loads(checkpoint_path.read_text())
hash_checks = []
for key, expected_key in [('source_exe', 'exe_sha256'), ('selected_save', 'save_sha256'), ('frozen_copy', 'save_sha256')]:
    p = Path(inventory[key])
    actual = hashlib.sha256(p.read_bytes()).hexdigest()
    assert actual == inventory[expected_key], (key, actual)
    hash_checks.append(dict(path=str(p), sha256=actual, unchanged=True))
b = Path(inventory['frozen_copy']).read_bytes()
compressed_bytes, declared_bytes = struct.unpack_from('<II', b, 8)
decoded = zlib.decompress(b[16:16+compressed_bytes])
assert len(decoded) == declared_bytes
steps = []
for item in checkpoint['steps']:
    unknown_api_events = int(item.get('boundary', {}).get('kind') == 'unimplemented_API')
    steps.append(dict(entry=item['entry'], status=item['status'],
                      instruction_hook_events=item['instructions'],
                      original_instruction_events=item['instructions'] - len(item['api_calls']) - unknown_api_events,
                      implemented_api_events=len(item['api_calls']),
                      boundary=item.get('boundary')))
logs = [json.loads(p.read_text()) for p in (root / 'evidence').glob('stage7-*.result.json')]
result = dict(input_hash_checks=hash_checks, original_save_producer_and_exe_compatibility='not established',
              first_block_stdlib_decode=dict(compressed_bytes=compressed_bytes, output_bytes=len(decoded),
                                            sha256=hashlib.sha256(decoded).hexdigest(),
                                            decoded_by_original_loader=False),
              latest_checkpoint=checkpoint_path.name, latest_steps=steps,
              latest_original_instruction_events=sum(s['original_instruction_events'] for s in steps),
              raw_checkpoint_metric_correction='instructions includes API thunk entries; unique_original_addresses in the historical checkpoint also included thunks; driver corrected without rerunning game code',
              bounded_processes_before_audit=len(logs), timeouts_before_audit=sum(p.get('timed_out', False) for p in logs),
              original_CreateFileA_or_ReadFile_executed_in_latest_checkpoint=any(s['name'] in ('CreateFileA', 'ReadFile') for item in checkpoint['steps'] for s in item['api_calls']),
              full_city_loaded=False, full_scheduler_cycle_executed=False, original_save_restored=False,
              arm64_chain_compared=False, complete_chain_passed=False)
(root / 'evidence/stage7-final-findings.json').write_text(json.dumps(result, indent=2, ensure_ascii=False)+'\n')
print(json.dumps(result, indent=2, ensure_ascii=False))
