"""Configuration, byte-backed analysis and sparse PE/SCI preparation only."""
import hashlib,json,shutil,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'samples'))
from profile_analysis import PE,Analysis
from build_pe import write_pe
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def number(x):return int(x,0)if isinstance(x,str)else x
def prepare(config,out):
 cfg=json.loads(Path(config).read_text());pe=PE(cfg['original_EXE']);assert sha(cfg['original_EXE'])==cfg['original_sha256']
 windows=[(number(a),number(b))for a,b in cfg['windows']]
 analysis=Analysis(pe,dict(roots=cfg['roots'],windows=cfg['windows'],resolved_indirect_edges=cfg.get('resolved_indirect_edges',{}),proved_branch_targets=cfg.get('proved_branch_targets',{}),declined_boundary_entries=list(cfg.get('boundary_functions',{})))).run()
 assert sum(q['end']-q['pc']for q in analysis['instructions'])==sum(b-a for a,b in windows),(analysis['unknowns'],len(analysis['instructions']))
 code=analysis['instructions'];out.mkdir(parents=True,exist_ok=False);relocs=[];pending=[];auto=[];manual=[]
 for row in analysis['address_candidates']:
  site=number(row['site']);target=number(row['target'])
  if row['operand_kind']=='mem'or row['confidence']=='high':auto.append(row);relocs.append(site-pe.base)
  elif row['site']in cfg.get('reviewed_pointer_sites',{}):manual.append(row);relocs.append(site-pe.base)
  else:pending.append(row) # Kept as original uint32; never guessed/rebased.
 pages={}
 def page(addr,codepage=False):
  base=addr&~4095
  if base not in pages:
   s=pe.section(base);assert s
   b=bytearray(4096)
   if not codepage:
    n=max(0,min(4096,s['va']+s['rawsize']-base))
    if n:b[:n]=pe.read(base,n)
   pages[base]=dict(data=b,flags=0x60000020 if codepage else s['flags'])
  return pages[base]['data']
 for lo,hi in windows:
  for i,v in enumerate(pe.read(lo,hi-lo)):page(lo+i,True)[(lo+i)&4095]=v
 for row in auto+manual:page(number(row['target']))
 boundaries={number(p):name for p,name in cfg.get('boundary_functions',{}).items()}
 for p in boundaries:page(p,True)
 entries=sorted(set(number(p)for p in cfg['roots'])|{q['target']for q in code if q['name']=='call'and q['target']not in boundaries})
 aliases={p:'function_%x'%p for p in entries};aliases[number(cfg['entry_PC'])]=cfg['entry_symbol']
 if cfg.get('existing_fixture'):
  shutil.copyfile(ROOT/cfg['existing_fixture'],out/'guestVA-six.exe')
 else:
  sections=[]
  for i,(base,p)in enumerate(sorted(pages.items())):
   if sections and sections[-1]['flags']==p['flags']and sections[-1]['rva']+pe.base+len(sections[-1]['data'])==base:sections[-1]['data'].extend(p['data'])
   else:sections.append(dict(name='.s%02d'%i,rva=base-pe.base,data=bytearray(p['data']),flags=p['flags']))
  write_pe(out/'guestVA-six.exe',sections,number(cfg['roots'][0])-pe.base,sorted(set(relocs)),{v:k for k,v in aliases.items()},max_bytes=2097152)
 fixture=PE(str(out/'guestVA-six.exe'))
 for a,b in windows:assert fixture.read(a,b-a)==pe.read(a,b-a)
 if cfg.get('existing_fixture'):
  # Existing fixture uses the same code pages; build its ignored intervals from
  # configured windows rather than reading old generated SCI files.
  pages={s['va']+i:dict(flags=s['flags'])for s in fixture.sections if s['flags']&0x20 for i in range(0,s['size'],4096)}
 ignored=[]
 for base,p in sorted(pages.items()):
  if not p['flags']&0x20:continue
  at=base
  for a,b in sorted((max(a,base),min(b,base+4096))for a,b in windows if a<base+4096 and b>base):
   if at<a:ignored.append((at,a-at))
   at=b
  if at<base+4096:ignored.append((at,base+4096-at))
 for name,text in {
  'global_aliases.sci':''.join('loc_%X,%s\n'%(p,n)for p,n in aliases.items()),
  'external_procedures.sci':''.join('loc_%X,%s\n'%(p,n)for p,n in boundaries.items()),
  'ignored_areas.sci':''.join('loc_%X,%d\n'%r for r in ignored),
  'instruction_flags.sci':''.join('loc_%X,%s,%s\n'%(number(q['site']),q['sr_mask'],q['sr_input_mask'])for q in analysis['flags']['annotations']),
  'SR.cfg':'esp_dword_aligned=yes\nebp_dword_aligned=no\n','extern.llinc':''.join('proc '+n+' external\n'for n in boundaries.values()),'macros.llinc':'','relocations.csv':''}.items():(out/name).write_text(text)
 metadata=dict(config_path=str(config),config_sha256=sha(config),id=cfg['id'],original_bytes=sum(b-a for a,b in windows),instructions=len(code),
  reconstructed_relocations=len(relocs),automatic_relocations=len(auto),reviewed_address_metadata=len(manual),unclassified_uint32_immediates=pending,
  automatic_relocation_percent=round(100*len(auto)/len(relocs),3)if relocs else 100,
  flags_annotations=len(analysis['flags']['annotations']),flags_generated_from_CFG=True,
  flags_unknown_contracts=analysis['flags']['incoming_or_unknown_contracts'],analysis_unknowns=analysis['unknowns'],
  aliases_generated=len(aliases),ignored_padding_generated=sum(n for p,n in ignored),original_instruction_bytes_changed=0,
  per_address_instruction_replacements=0,runtime_eligible=cfg['runtime_eligible'],boundary_functions=cfg.get('boundary_functions',{}))
 (out/'analysis.json').write_text(json.dumps(analysis,indent=2)+'\n');(out/'preparation.json').write_text(json.dumps(metadata,indent=2)+'\n')
 return cfg,analysis,metadata
