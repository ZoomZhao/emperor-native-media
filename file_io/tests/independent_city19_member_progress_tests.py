#!/usr/bin/env python3
"""Bounded independent member-progress/cache properties. Synthetic inputs only."""
import argparse, hashlib, json, os, struct, subprocess, sys, time
from pathlib import Path
ROOT = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--next-output', type=Path, required=True)
parser.add_argument('--housing-source-root', type=Path, required=True)
parser.add_argument('--next-source-root', type=Path, required=True)
args = parser.parse_args()
if sys.flags.optimize: parser.error('optimized test invocation unsupported')
out = args.output.resolve()
if out.exists(): parser.error('fresh output directory required')
out.mkdir(parents=True)
inputs = out/'synthetic-inputs'; inputs.mkdir()
begin = time.monotonic(); checks = []; commands = []
sha = lambda b: hashlib.sha256(b).hexdigest()
def run(argv, label, env=None):
    remaining = 180-(time.monotonic()-begin)
    if remaining <= 0: raise RuntimeError('180-second total budget exhausted')
    t = time.monotonic()
    q = subprocess.run([str(x) for x in argv], capture_output=True, text=True,
                       timeout=min(10, remaining), env=env)
    (out/(label+'.stdout')).write_text(q.stdout)
    (out/(label+'.stderr')).write_text(q.stderr)
    commands.append(dict(argv=[str(x) for x in argv], exit_code=q.returncode,
                         elapsed_seconds=time.monotonic()-t,
                         stdout_sha256=sha(q.stdout.encode()), stderr_sha256=sha(q.stderr.encode())))
    return q

baseline = json.loads((args.next_output/'results.json').read_text())
tool = args.next_source_root.resolve()/'file_io/tools/qualify_city19_next_member_tag.cpp'
if baseline['tool_source_sha256'] != sha(tool.read_bytes()):
    raise RuntimeError('native test binary source report does not match frozen tool')

# Source read widths spelled independently: root/section6, component2, helper/owner,
# four fixed requests, member header, class declaration and two embedded versions.
prelude = (struct.pack('<HHBIII',19,6,1,71,72,73) + b'OWN_TEST_GUID_16' +
           struct.pack('<III',74,75,76) + struct.pack('<HIII8II',2,81,82,83,*range(91,99),84) +
           struct.pack('<HH',3,4) + b'R'*3256)
if len(prelude) != 3355: raise RuntimeError('independent prelude size')
mh = lambda n: struct.pack('<Hi',1,n)
decl = lambda name,schema=0: struct.pack('<HHH',65535,schema,len(name))+name
payload1 = struct.pack('<HIHIII',1,7101,1,7102,7103,7104)
payload2 = struct.pack('<HIHIII',1,8101,1,8102,8103,8104)
prefix = lambda n: prelude+mh(n)+decl(b'cHousingGoal')+payload1
if len(prefix(1)) != 3399: raise RuntimeError('independent first payload boundary')
tags = [('object-compact',struct.pack('<H',2),False),
        ('object-extended',struct.pack('<HI',32767,2),False),
        ('class-compact',struct.pack('<H',32769),True),
        ('class-extended',struct.pack('<HI',32767,0x80000001),True)]
cases = []
for count in [1,2,3,2147483647]:
    for nextcount in ([1,2,3,2147483647] if count==1 else [None]):
        for label,tag,new_instance in tags:
            name = 'count-'+str(count)+'-next-'+str(nextcount)+'-'+label
            head = mh(nextcount) if count==1 else b''
            # Poisoned payload-looking tail must remain uninterpreted for object refs.
            tail = payload2 if new_instance else b'\xff'*20
            body = prefix(count)+head+tag+tail+b'UNQUALIFIED_NEXT_BOUNDARY'
            end = 3399+len(head)+len(tag)+(20 if new_instance else 0)
            member = 1 if count==1 else 0
            complete = nextcount==1 if count==1 else count==2
            cases.append(dict(name=name,body=body,valid=True,end=end,member=member,
                              complete=complete,following_member=member+int(complete),
                              following_kind='next_member_header' if complete else 'next_object_tag_same_member',
                              new_instance=new_instance,
                              source='560040 positive count loop/55a7a0 stride18;77fd90 ref vs factory branch; conditional no-preseed cache1/2'))
for name,tag in [('extended-null',struct.pack('<HI',32767,0)),
                 ('extended-class-zero',struct.pack('<HI',32767,0x80000000))]:
    cases.append(dict(name=name,body=prefix(1)+mh(1)+tag,valid=False,
                      source='null consumer dereferences object+4 (modern refusal), class index0 source invalid'))
for value in [0,65535]:
    for offset,label in [(0,'base'),(6,'derived')]:
        altered=payload2[:offset]+struct.pack('<H',value)+payload2[offset+2:]
        cases.append(dict(name=label+'-version-'+str(value),body=prefix(1)+mh(1)+struct.pack('<H',32769)+altered,
                          valid=False,source='explicit modern versions1 subset; original readers have no returned-version field gate'))
    cases.append(dict(name='member-version-'+str(value),body=prefix(1)+struct.pack('<Hi',value,1)+struct.pack('<H',2),
                      valid=False,source='explicit modern member version1 subset;560040 has no local returned-version gate'))
for name,identifier,schema in [('pending-u16-schema',b'cUnknownNext',65535),('pending-known-name',b'cHousingGoal',0)]:
    body=prefix(1)+mh(1)+decl(identifier,schema)+payload2
    cases.append(dict(name=name,body=body,valid=True,pending=True,end=3405+6+len(identifier),schema=schema,
                      source='rawffff declaration/schema/name protocol; modern unknown lookup halts before registration/payload'))

for case in cases:
    body=case['body']; blob=struct.pack('<III',0xfedcbaaa,0xaaabcdef,len(body))+body
    path=inputs/(case['name']+'.bin');path.write_bytes(blob)
    for mode in ['release','ubsan']:
        exe=args.next_output/('next-member-tag-'+mode)
        q=run([exe,path,len(blob),262144,4915584,1,0xfedcbaaa,0xabcdefff,0xaaabcdef,36144,36144,sha(blob),sha(body)],mode+'-'+case['name'])
        seen=json.loads(q.stdout) if q.stdout else None
        if not case['valid']:
            reason=('positive member consumer requires nonnull object' if case['name']=='extended-null' else
                    'source next class reference zero invalid' if case['name']=='extended-class-zero' else
                    'next member version outside modern scope1' if case['name'].startswith('member-version') else
                    'referenced Housing section versions outside modern scope1')
            good=q.returncode==2 and not q.stdout and reason in q.stderr
        else:
            good=(q.returncode==0 and seen is not None and seen['parsed_end']==case['end'] and
                  seen['following_consumer_offset']==case['end'] and
                  seen['remaining_uninterpreted_decoded_bytes']==len(body)-case['end'] and
                  seen['actual_descriptor_resolved'] is False and seen['cache_runtime_state_proved'] is False and
                  seen['original_class_factory_or_schema_reader_executed'] is False and
                  seen['mainpool_indices_interpreted'] is False and seen['object_field_values_emitted'] is False)
            if case.get('pending'):
                good=good and (seen['next_declared_schema']==case['schema'] and
                               seen['next_pending_class_model_id']==3 and seen['next_pending_object_model_id']==4 and
                               seen['conditional_cache_next_after_item']==3 and seen['next_payload_bytes_consumed']==0 and
                               seen['next_item_complete_under_source_model'] is False and
                               seen['following_consumer_kind']=='pending_class_payload')
            else:
                new=case['new_instance']
                good=good and (seen['next_member_index']==case['member'] and
                               seen['next_member_complete_after_qualified_item']==case['complete'] and
                               seen['following_member_index']==case['following_member'] and
                               seen['following_consumer_kind']==case['following_kind'] and
                               seen['next_payload_bytes_consumed']==(20 if new else 0) and
                               seen['conditional_cache_next_after_item']==(4 if new else 3) and
                               seen['next_reference_index']==(1 if new else 2) and
                               seen['next_object_reference_reuses_existing_metadata']==(not new) and
                               seen['next_payload_sha256']==(sha(payload2) if new else None))
        checks.append(dict(name=case['name'],mode=mode,passed=bool(good),expectation_source=case['source'],observed=seen))
        if path.read_bytes()!=blob: raise RuntimeError('synthetic input changed')

guard_checks=[]
for label,script in [('housing','city19_housing_payload_tests.py'),('next','city19_next_member_tag_tests.py')]:
    root=(args.housing_source_root if label=='housing' else args.next_source_root).resolve()
    for variant,flags,opt in [('O',['-O'],None),('OO',['-OO'],None),('env1',[],'1'),('env2',[],'2')]:
        destination=out/(label+'-'+variant+'-must-not-exist')
        env=os.environ.copy();env.pop('PYTHONOPTIMIZE',None)
        if opt is not None:env['PYTHONOPTIMIZE']=opt
        q=run([sys.executable,*flags,root/'file_io/tests'/script,'--source-root',root,'--output',destination],label+'-guard-'+variant,env)
        guard_checks.append(dict(name=label+' '+variant,passed=q.returncode==2 and not destination.exists() and
                                 'optimized test invocation unsupported' in q.stderr,
                                 expectation_source='explicit runner optimized-start rejection before output mkdir',output_created=destination.exists()))
result=dict(native_passed=sum(q['passed'] for q in checks),native_failed=sum(not q['passed'] for q in checks),native_cases_per_mode=len(cases),
            optimization_passed=sum(q['passed'] for q in guard_checks),optimization_failed=sum(not q['passed'] for q in guard_checks),
            checks=checks,optimization_checks=guard_checks,commands=commands,elapsed_seconds=time.monotonic()-begin,total_budget_seconds=180,
            real_input_or_EXE_read=False,original_or_derived_game_executed=False,new_REA_queries=False,main_written=False,pushed=False)
(out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:result[k] for k in ['native_passed','native_failed','native_cases_per_mode','optimization_passed','optimization_failed','elapsed_seconds']}))
raise SystemExit(bool(result['native_failed'] or result['optimization_failed']))
