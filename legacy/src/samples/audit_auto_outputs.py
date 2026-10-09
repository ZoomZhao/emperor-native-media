#!/usr/bin/env python3
"""Audit exact raw decoding with installed Apple objdump and artifact closure."""
import json,re,subprocess,hashlib
from pathlib import Path
from auto_i386_analysis import PE
ROOT=Path(__file__).resolve().parents[1]
objdump='<LOCAL_MACHINE_PATH>'
analyses={};unique=set();bound_processes=0;artifacts={}
for name in ('leaf','tax','write'):
    r=json.loads((ROOT/f'runs/auto-{name}/analysis.json').read_text());analyses[name]=r
    assert r['conversion_ready']
    static={}
    for lo,hi in r['configuration']['windows']:
        lo,hi=int(lo,0),int(hi,0)
        result=subprocess.run([objdump,'-d',f'--start-address={lo:#x}',f'--stop-address={hi:#x}',r['source']],capture_output=True,text=True,timeout=60,check=True);bound_processes+=1
        for line in result.stdout.splitlines():
            m=re.match(r'\s*([0-9a-f]+):\s+((?:[0-9a-f]{2}\s+)+)',line)
            if m:static[int(m[1],16)]=''.join(m[2].split())
    for ins in r['instructions']:
        assert static[ins['pc']]==ins['bytes'],(name,ins)
        unique.add(ins['pc'])
    build=json.loads((ROOT/f'runs/auto-{name}-built/auto-build.json').read_text());expected={int(a['site'],16) for a in r['address_candidates'] if not a['initializer_scope'] and a['confidence']=='high'}|{int(a['slot'],16) for a in r['runtime_pointer_bindings'] if a['confidence']=='high'}
    assert {int(x,16) for x in build['relocations']}==expected
    assert (ROOT/f'runs/auto-{name}/instruction_flags.sci').read_bytes()==(ROOT/f'runs/auto-{name}-built/instruction_flags.sci').read_bytes()
    fixture=PE(str(ROOT/f'samples/auto-{name}-built.exe'))
    indexed={ins['pc']:ins for ins in r['instructions']};module_nodes=set();queue=[int(x,0) for x in r['configuration']['roots']]
    while queue:
        pc=queue.pop()
        if pc in module_nodes:continue
        module_nodes.add(pc);ins=indexed[pc];control=ins['control']
        if control=='ret':continue
        if control in ('jmp','jcc','call'):queue.append(ins['target'])
        if control!='jmp':queue.append(ins['end'])
    for pc in module_nodes:
        ins=indexed[pc];assert fixture.read(pc,ins['end']-pc).hex()==ins['bytes']
    for path in (f'samples/auto-{name}-built.exe',f'runs/auto-{name}-built/verify-arm64',f'runs/auto-{name}/analysis.json',f'runs/auto-{name}/instruction_flags.sci'):
        b=(ROOT/path).read_bytes();artifacts[path]=dict(size=len(b),sha256=hashlib.sha256(b).hexdigest())
commands=[]
for p in sorted((ROOT/'evidence').glob('stage3-*.result.json')):
    row=json.loads(p.read_text());assert not row['timed_out'] and row['duration_seconds']<=60;commands.append(dict(label=p.stem[:-7],**row))
results={n:json.loads((ROOT/f'evidence/stage3-verify-auto-{n}{"-final" if n=="write" else ""}.stdout.log').read_text()) for n in ('leaf','tax','write')}
assert all(r['passed'] for r in results.values())
regression=json.loads((ROOT/'evidence/stage3-analysis-regression.json').read_text());assert len(regression['negative_controls'])==10
original=PE(analyses['tax']['source']);assert hashlib.sha256(original.data).hexdigest()=='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
for tool,sha in [('build/bin/SRW-llasm','58bdee5f45110b738fb92d185eb08efe607efb64994dcfaf821c1f8aa93d6fac'),('build/bin/llasm','f9e230357d9356b038d2e6585bbda1aa672a6f90f594958bf88d0e1c0bf95e22')]:assert hashlib.sha256((ROOT/tool).read_bytes()).hexdigest()==sha
report=dict(analyses=analyses,regression=regression,native_results=results,holdout_oracle=json.loads((ROOT/'evidence/stage3-write-oracle.json').read_text()),independent_objdump_instruction_addresses=len(unique),independent_objdump_processes=bound_processes,artifacts=artifacts,commands=commands,max_recorded_seconds=max(r['duration_seconds'] for r in commands),timeouts=0,baseline_limit='original raw instructions interpreted by authored ISA model; no direct original Win32/i386 execution',automation_limit='reviewed roots/windows/layout/model inputs still manual; candidate metrics are relative to prior reviewed fixture, not original stripped relocation ground truth')
(ROOT/'evidence/stage3-final-results.json').write_text(json.dumps(report,indent=2))
print(json.dumps(dict(raw_instruction_decodings_checked=len(unique),objdump_processes=bound_processes,negative_controls=10,native_regressions_passed=3,recorded_commands=len(commands),max_seconds=report['max_recorded_seconds'],timeouts=0)))
