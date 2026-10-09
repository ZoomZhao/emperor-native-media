"""Build-only real six-function SR slice; no engine import or library loading."""
import collections,hashlib,json,re,struct,subprocess,sys
from pathlib import Path
from auto_i386_analysis import PE
from build_pe import write_pe
ROOT=Path(__file__).resolve().parents[1];E=ROOT/'evidence';RUN=ROOT/'runs/stage71-SR-real';RUN.mkdir(exist_ok=True)
BASE=0x400000;EXE=Path('<LOCAL_ORIGINAL_PROJECT>/Exe/ghidra/input/EmperorEN.exe');SHA='8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    global RUN
    attempt=sys.argv[1]if len(sys.argv)>1 else 'v1'
    if attempt.startswith('v3'):
        RUN=ROOT/'runs/stage71-SR-resume';RUN.mkdir(exist_ok=True)
    pe=PE(EXE);assert sha(EXE)==SHA
    closure=json.loads((E/'stage69-static-closure.json').read_text());a=json.loads((ROOT/'runs/stage70-city/analysis.json').read_text())
    assert closure['code_bytes']==728 and a['instruction_count']==214
    windows=[(int(q['entry'],16),int(q['exclusive_end'],16))for q in closure['functions']]
    pages={};rows=[]
    def page(addr,code=False):
        base=addr&~4095
        if base not in pages:
            section=pe.section(base);assert section
            buf=bytearray(4096)
            if not code:
                n=max(0,min(4096,section['va']+section['rawsize']-base))
                if n:buf[:n]=pe.read(base,n)
            pages[base]=dict(data=buf,flags=0x60000020 if code else section['flags'])
        return pages[base]['data']
    for q,(lo,hi) in zip(closure['functions'],windows):
        body=pe.read(lo,hi-lo);assert hashlib.sha256(body).hexdigest()==q['sha256']
        for i,value in enumerate(body):page(lo+i,True)[(lo+i)&4095]=value
    inputs=json.loads((E/'stage70-real-input-contract.json').read_text())
    for r in inputs['extracted_spans'][:11]:
        addr=int(r['guest_address'],16)
        for base in range(addr&~4095,(addr+r['bytes']+4095)&~4095,4096):page(base)
    reviewed={
        0x4af316:'MOV ECX container address passed to exact vector-index accessor; Stage69 pointer chain binds container+4.',
        0x4af368:'CMP EAX exclusive end of top10 pair array; EAX starts at pair field and advances8 in same original loop.',
        0x4af376:'MOV EDI label-table base immediately consumed by unchanged REP STOSD.',
        0x4af388:'MOV EDI second label-table base immediately consumed by unchanged REP STOSD.',
        0x4af3ff:'MOV EAX top10 size-field address used by original array loop.',
        0x4af40c:'CMP EAX exclusive end of same top10 size-field loop.',
        0x4af41f:'MOV EAX final top10 pair address used by original shift loop.',
        0x4af4a3:'MOV EAX ring base immediately indexed/stored by original queue code.',
        0x4af4d7:'MOV EBP neighbor-delta table base immediately read and advanced4.',
        0x4af552:'CMP EBP one-past four neighbor deltas, same original pointer loop.'}
    relocs=[]
    for q in a['address_candidates']:
        site=int(q['site'],16);target=int(q['target'],16);ins=next(x for x in a['instructions']if x['pc']==int(q['instruction'],16))
        assert any(lo<=site and site+4<=hi for lo,hi in windows)
        assert int.from_bytes(pe.read(site,4),'little')==target
        if q['operand_kind']=='mem':reason='Decoded 32-bit absolute or indexed image-memory displacement; target is original input/output span.'
        elif q['confidence']=='high':reason='Existing original-byte constant-use proof: '+'; '.join(q['evidence'])
        else:assert site in reviewed,(hex(site),q);reason=reviewed[site]
        page(target);relocs.append(site-BASE);rows.append(dict(site=hex(site),original_u32=hex(target),instruction=hex(ins['pc']),operand_kind=q['operand_kind'],prior_auto_confidence=q['confidence'],reviewed_evidence=reason))
    sections=[]
    for i,(base,item)in enumerate(sorted(pages.items())):
        if sections and sections[-1]['flags']==item['flags']and sections[-1]['rva']+BASE+len(sections[-1]['data'])==base:sections[-1]['data'].extend(item['data'])
        else:sections.append(dict(name='.s%02d'%i,rva=base-BASE,data=bytearray(item['data']),flags=item['flags']))
    names={lo:'city_'+format(lo,'x')for lo,hi in windows}
    if attempt.startswith('v3'):names[0x4af311]='city_resume_4af311'
    fixture=RUN/'real-six.exe';write_pe(fixture,sections,0x4af350-BASE,sorted(set(relocs)),{n:p for p,n in names.items()},max_bytes=2097152)
    fixture_pe=PE(fixture)
    for lo,hi in windows:assert fixture_pe.read(lo,hi-lo)==pe.read(lo,hi-lo)
    ignored=[]
    for base,item in sorted(pages.items()):
        if not item['flags']&0x20:continue
        covered=sorted((max(lo,base),min(hi,base+4096))for lo,hi in windows if lo<base+4096 and hi>base);at=base
        for lo,hi in covered:
            if at<lo:ignored.append((at,lo-at))
            at=hi
        if at<base+4096:ignored.append((at,base+4096-at))
    files={'global_aliases.sci':''.join('loc_%X,%s\n'%(p,n)for p,n in names.items()),
        'ignored_areas.sci':''.join('loc_%X,%d\n'%q for q in ignored),
        'instruction_flags.sci':''.join('loc_%X,%s,%s\n'%(int(q['site'],16),q['sr_mask'],q['sr_input_mask'])for q in a['flags']['annotations']),
        'SR.cfg':'esp_dword_aligned=yes\nebp_dword_aligned=no\n','extern.llinc':'','macros.llinc':''}
    for name,body in files.items():(RUN/name).write_text(body)
    metadata=dict(original_EXE_sha256=SHA,original_PE_base_relocation_directory_absent=True,
        fixture_sha256=sha(fixture),original_instruction_bytes=728,functions=closure['functions'],instruction_count=214,
        instruction_bytes_changed=0,relocation_records=rows,relocation_policy='Only decoded address fields with original semantic provenance; reconstructed metadata, not claimed original relocation records. Dynamic heap pointers not scanned/rebased.',
        data_pages='Original PE file bytes where backed, otherwise original virtual BSS zero; no saved state or expected result seeded. Build fixture is not the runtime checkpoint loader.',
        metadata_files={name:dict(sha256=sha(RUN/name),body=body)for name,body in files.items()},
        flag_producer_evidence=a['flags']['producer_evidence'],conditional_reads=a['flags']['conditional_read_evidence'],
        flag_policy=a['flags']['policy'],known_flag_contract_unknowns=a['flags']['incoming_or_unknown_contracts'],ignored_padding_bytes=sum(n for p,n in ignored),
        original_calls=closure['direct_calls'],imports=0,indirect_targets=0,instruction_replacements=0)
    (E/('stage71-'+attempt+'-metadata.json')).write_text(json.dumps(metadata,indent=2)+'\n')
    steps=[]
    def command(label,args):
        out=E/(label+'-'+attempt+'.stdout.log');err=E/(label+'-'+attempt+'.stderr.log')
        with out.open('wb')as o,err.open('wb')as e:q=subprocess.run([str(x)for x in args],cwd=RUN,stdout=o,stderr=e,timeout=30)
        row=dict(step=label,command=[str(x)for x in args],cwd=str(RUN),returncode=q.returncode,stdout=str(out),stderr=str(err));steps.append(row)
        (E/('stage71-'+attempt+'-build-steps.json')).write_text(json.dumps(steps,indent=2)+'\n')
        assert q.returncode==0,(label,q.returncode)
    command('stage71-SRW',[ROOT/'build/bin/SRW-llasm','real-six.exe','real-six.llasm'])
    original=(RUN/'real-six.llasm').read_text();lines=original.splitlines(True);aliases=[s for s in lines if s.startswith('define ')];lines=[s for s in lines if not s.startswith('define ')];i=next(i for i,s in enumerate(lines)if s.startswith('datasegment'));lines[i:i]=aliases+['\n']
    normalized=''.join(lines)
    (RUN/'normalized.llasm').write_text(normalized)
    (E/('stage71-'+attempt+'-normalization.json')).write_text(json.dumps(dict(original_sha256=hashlib.sha256(original.encode()).hexdigest(),normalized_sha256=sha(RUN/'normalized.llasm'),aliases_moved=len(aliases),instruction_translation_edits=0),indent=2)+'\n')
    command('stage71-llasm',[ROOT/'build/bin/llasm','normalized.llasm','-I',ROOT/'vendor-src/SR/SR/llasm-support','-m64','-pic','-ptrofs','-O','-o','real-six.ll'])
    command('stage71-clang-LLVM',['/usr/bin/clang','-Wno-override-module','-O2','-c','real-six.ll','-o','real-six.o'])
    result=dict(status='REAL728B_SRW_LLASM_LLVM_ARM64_OBJECT_BUILT_UNEXECUTED',steps=steps,original_instruction_bytes=728,instructions=214,original_code_changed=False,object_sha256=sha(RUN/'real-six.o'),object=str(RUN/'real-six.o'),generated_object_executed_or_loaded=False,original_EXE_or_Unicorn_executed=False)
    (E/('stage71-'+attempt+'-build-result.json')).write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)
if __name__=='__main__':main()
