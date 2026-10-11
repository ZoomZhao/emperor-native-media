#!/usr/bin/env python3
"""Review-gated original-profile integration on independent synthetic data only."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo', type=Path, required=True)
parser.add_argument('--profile', type=Path, required=True)
parser.add_argument('--gate', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
repo = args.repo.resolve()
root = Path(__file__).resolve().parent
out = args.output
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
gate = json.loads(args.gate.read_text())
assert gate['schema'] == 'INDEPENDENT_PROFILE_AND_OWNED_REVIEW_GATE_V1'
assert gate['approved_source_profile'] and gate['approved_owned_wrapper'], 'both independent reviews required'
assert gate['profile_commit'] == '4ac4fc861fffa9d4f63e330f8c8df951b453c41e'
assert gate['owned_commit'] == '08152cc48fa0229ea2e0d592a1c6f201145f45b8'
assert sha(args.profile) == gate['profile_sha256']
for item in gate['review_artifacts']:
    path = Path(item['path'])
    if not path.is_absolute(): path = repo/path
    assert sha(path) == item['sha256']
for relative, expected in gate['component_source_sha256'].items():
    assert sha(repo/relative) == expected
profile = json.loads(args.profile.read_text())
markers = profile['marker_parameters']
head = markers['container_marker']['value_u32']
comp = markers['compressed_marker']['value_u32']
raw = markers['raw_marker']['value_u32']
encoded_limit = profile['fixed_mode']['native_fixed_max_encoded_block']
decoded_limit = profile['fixed_mode']['native_fixed_max_decoded_block']
assert encoded_limit == decoded_limit == 36144
if not out.is_absolute() or out.exists(): parser.error('fresh absolute output directory required')
out.mkdir(parents=True)
commands, results = [], []
logs = out/'command-logs'
logs.mkdir()

def command(argv, timeout=60):
    result = subprocess.run([str(x) for x in argv], capture_output=True, text=True, timeout=timeout)
    name = 'command-%04d' % len(commands)
    stdout = logs/(name+'.stdout'); stderr = logs/(name+'.stderr')
    stdout.write_text(result.stdout); stderr.write_text(result.stderr)
    commands.append({'argv': [str(x) for x in argv], 'exit_code': result.returncode,
                     'stdout':str(stdout), 'stdout_sha256':sha(stdout),
                     'stderr':str(stderr), 'stderr_sha256':sha(stderr)})
    assert result.returncode == 0, result.stderr
    return result.stdout

corpus = out/'synthetic-corpus'
command([sys.executable, '-B', repo/'tests/original_file_fixtures/generate_fixtures.py', '--output', corpus,
         '--compressed-marker-hex', comp.to_bytes(4,'little').hex(),
         '--raw-marker-hex', raw.to_bytes(4,'little').hex()])
manifest = json.loads((corpus/'manifest.json').read_text())
assert manifest['synthetic_only'] is True and manifest['case_count'] == 166
cases = {c['id']: c for c in manifest['cases']}
containers = out/'synthetic-containers'
containers.mkdir()

def data(spec):
    path = (corpus/spec['path']).resolve()
    assert corpus in path.parents
    value = path.read_bytes()
    assert len(value) == spec['bytes'] and hashlib.sha256(value).hexdigest() == spec['sha256']
    return value

for case in cases.values():
    payload = data(case['input'])
    (containers/(case['id']+'.bin')).write_bytes(head.to_bytes(4,'little')+payload)
    if case['expected_output']: data(case['expected_output'])

small = data(cases['raw_small']['expected_output'])
dynamic = data(cases['zlib_dynamic']['expected_output'])
policy = {
    'physical_eof_before_marker': ([], 'end', {}),
    'unknown_complete_marker': ([], 'unknown_marker', {}),
    'whole_frame_then_file_junk': ([small], 'unknown_marker', {}),
    'multiblock_raw_raw': ([small,small], 'end', {}),
    'multiblock_zlib_zlib': ([small,dynamic], 'end', {}),
    'multiblock_raw_zlib': ([small,small], 'end', {}),
    'multiblock_zlib_raw': ([small,small], 'end', {}),
    'multiblock_good_then_corrupt': ([small], 'truncated', {}),
    'multiblock_empty_then_nonempty': ([b'',small], 'end', {}),
    'multiblock_empty_128': ([b'']*64, 'block_limit', {'blocks':64}),
    'aggregate_budget_two_blocks': ([small], 'output_limit', {'output':len(small)}),
}
sdk = command(['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path']).strip()
for mode, flags in [('release',['-O2']),('undefined',['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all'])]:
    exe = out/('profile-owned-'+mode)
    command(['/usr/bin/clang++','-std=c++17','-Wall','-Wextra','-Werror','-isysroot',sdk,
             '-I',repo/'file_io/include',repo/'file_io/src/strict_zlib.cpp',repo/'file_io/src/framed_reader.cpp',
             root/'tests/profile_integration_driver.cpp',*flags,'-lz','-o',exe])
    def invoke(path, ownership, output_budget=1048576, block_budget=1024, capacity=decoded_limit):
        return json.loads(command([exe,path,head,comp,raw,encoded_limit,capacity,1048576,
                                   output_budget,block_budget,ownership],timeout=5))
    def record(identifier, kind, action):
        try:
            detail = action()
            results.append({'mode':mode,'id':identifier,'kind':kind,'passed':True,'detail':detail})
        except Exception as error:
            results.append({'mode':mode,'id':identifier,'kind':kind,'passed':False,'error':str(error)})
    for ownership in ('copy','move'):
        for case in cases.values():
            if case['expected']['status'] != 'ready_block_assertion': continue
            def block_check(c=case, own=ownership):
                observations = c['trusted_codec_observations_not_native_results']
                capacity = observations[0]['observation']['capacity'] if observations else decoded_limit
                value = invoke(containers/(c['id']+'.bin'),own,capacity=capacity)
                first = value['events'][0]
                assert value['sticky']
                # A required-frame zero-byte prefix was a singleblock reject.
                # Header-only container is a separate intentional modern End policy.
                if c['input']['bytes'] == 0:
                    assert first['status'] == 'end' and first['unchanged']
                    return {'status':'end','basis':'modern header-only container policy; does not revise original required-frame expectation'}
                if c['expected']['block_outcome'] == 'accept':
                    assert first['status'] == 'frame'
                    output = data(c['expected_output'])
                    assert bytes.fromhex(first['output_hex']) == output
                    layout = c['construction_layout_not_reader_result']
                    assert first['input_offset'] == 4+layout['payload_offset']+layout['declared_n']
                    assert first['frame_start'] == 4 and first['output_available'] == len(output)
                    if layout['kind'] == 'zlib':
                        observation = observations[0]['observation']
                        assert first['opaque'] == layout['opaque_word']
                        assert first['codec_consumed'] == observation['codec_consumed_bytes']
                        assert first['codec_tail'] == observation['unused_data_bytes']
                    expected_end = 'unknown_marker' if c['id']=='raw_declared_short_extra_file_bytes' else 'end'
                    assert value['events'][-1]['status'] == expected_end
                else:
                    reason = c['expected']['reason']
                    expected = 'truncated' if reason.startswith('short_') else (
                        'input_limit' if reason.startswith('input_limit') else (
                            'output_limit' if reason=='output_limit' else 'codec_error'))
                    assert first['status'] == expected and first['output_available'] == 0 and first['unchanged']
                detail = {k:v for k,v in first.items() if k!='output_hex'}
                actual = bytes.fromhex(first['output_hex'])
                detail.update(output_bytes=len(actual), output_sha256=hashlib.sha256(actual).hexdigest(),
                              terminal_status=value['events'][-1]['status'])
                return detail
            kind = 'modern_empty_container_policy' if case['input']['bytes']==0 else 'independent_fixture_profile_integration'
            record(case['id']+'/'+ownership,kind,block_check)
        for identifier,(outputs,terminal,limits) in policy.items():
            def policy_check(name=identifier, expected=outputs, ending=terminal, opts=limits, own=ownership):
                value = invoke(containers/(name+'.bin'),own,output_budget=opts.get('output',1048576),
                               block_budget=opts.get('blocks',1024))
                events = value['events']
                assert [bytes.fromhex(e['output_hex']) for e in events[:-1]] == expected
                assert events[-1]['status'] == ending and events[-1]['unchanged'] and value['sticky']
                assert events[-1]['blocks'] == len(expected) and events[-1]['total_output'] == sum(map(len,expected))
                return {'statuses':[e['status'] for e in events], 'total_output':events[-1]['total_output']}
            record(identifier+'/'+ownership,'modern_stream_policy_only',policy_check)
        for length in range(4):
            path = containers/('header-cut-%d.bin'%length)
            path.write_bytes(head.to_bytes(4,'little')[:length])
            def header_check(p=path,own=ownership):
                value = invoke(p,own)
                assert value['events'][0]['status']=='truncated' and value['events'][0]['unchanged'] and value['sticky']
                return {'status':'truncated'}
            record('outer-header-cut-%d/'%length+ownership,'modern_header_boundary',header_check)
        path = containers/'wrong-complete-header.bin'
        path.write_bytes((head^1).to_bytes(4,'little')+data(cases['raw_small']['input']))
        def mismatch_check(p=path,own=ownership):
            value = invoke(p,own)
            assert value['events'][0]['status']=='unsupported_header' and value['events'][0]['unchanged'] and value['sticky']
            assert value['events'][0]['output_available']==0
            return {'status':'unsupported_header','basis':'modern reject policy; original passthrough remains distinct'}
        record('wrong-complete-header/'+ownership,'modern_header_boundary',mismatch_check)
    failures = [r for r in results if r['mode']==mode and not r['passed']]
    print(mode, sum(r['mode']==mode and r['passed'] for r in results), 'passed', len(failures), 'failed', flush=True)

receipt = {'schema':'APPROVED_SOURCE_PROFILE_SYNTHETIC_INTEGRATION_V1','gate':gate,
           'profile_sha256':sha(args.profile),'synthetic_manifest_sha256':sha(corpus/'manifest.json'),
           'results':results,'commands':commands,'original_data_reused_or_decoded':False,'original_program_executed':False,
           'new_API_added':False,'real_file_accepted':False,'ASan_executed':False,
           'source_hashes':{str(p):sha(p) for p in [Path(__file__).resolve(),root/'tests/profile_integration_driver.cpp',
               repo/'file_io/include/owned_reader.hpp',repo/'file_io/include/framed_reader.hpp',
               repo/'file_io/src/strict_zlib.cpp',repo/'file_io/src/framed_reader.cpp']}}
(out/'results.json').write_text(json.dumps(receipt,ensure_ascii=False,indent=2)+'\n')
if any(not r['passed'] for r in results): raise SystemExit(1)
