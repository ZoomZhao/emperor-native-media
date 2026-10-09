"""Pinned configuration build. Loading is a separate audited read-only action."""
import argparse,hashlib,json,re,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage81';sys.path.insert(0,str(S/'tools'))
from separate_code_targets import transform
from scalar_guards import instrument
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--config',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args();c=json.loads(a.config.read_text());assert c['original_game_execution_maximum']==0;assert all(sha(ROOT/n)==h for n,h in c['pinned_inputs'].items());out=a.out.resolve();assert out.is_relative_to(S/'builds');out.mkdir(exist_ok=False);steps=[];objects=[];rewrites=[]
 def run(name,args):
  with(out/(name+'.stdout.log')).open('xb')as o,(out/(name+'.stderr.log')).open('xb')as e:q=subprocess.run([str(x)for x in args],cwd=ROOT,stdout=o,stderr=e,timeout=30)
  steps.append(dict(name=name,command=[str(x)for x in args],returncode=q.returncode));assert q.returncode==0,name
 for row in c['LLVM_modules']:
  text,r=transform((ROOT/row['path']).read_text(),expected_lookups=row['expected_resolver_calls']);r['module']=row['name']
  if row['add_scalar_guards']:text,g=instrument(text);r.update(g)
  file=out/(row['name']+'.ll');file.write_text(text);obj=out/(row['name']+'.o');run('clang-'+row['name'],['/usr/bin/clang','-Wno-override-module','-O2','-c',file,'-o',obj]);objects.append(obj);rewrites.append(r)
 for n in c['authored_C_sources']:
  obj=out/(Path(n).stem+'.o');run('clang-'+obj.stem,['/usr/bin/clang','-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(S/'tools'),'-I'+str(ROOT/'stage80/tools'),'-I'+str(ROOT/'stage79/tools'),'-I'+str(ROOT/'stage78/tools'),'-I'+str(ROOT/'stage77/tools'),'-c',ROOT/n,'-o',obj]);objects.append(obj)
 objects +=[ROOT/n for n in c['frozen_authored_helper_objects']];archive=out/'phase8-separated-OPEN.a';run('archive',['/usr/bin/ar','rcs',archive]+objects);library=out/'phase8-load-only.dylib';run('link-load-only',['/usr/bin/clang','-dynamiclib','-Wl,-undefined,dynamic_lookup','-o',library]+objects);run('undefined',['/usr/bin/nm','-u',library]);symbols=[]
 for l in(out/'undefined.stdout.log').read_text().splitlines():
  m=re.fullmatch(r'\s*(?:U\s+)?(\S+)',l)
  if m:symbols.append(m[1])
 report=dict(status='FULL_FRESH_SR_GAME_OBJECT_CODE_DATA_SEPARATED_BUILD_ONLY',source_functions=203,source_instructions=7346,source_bytes=20089,rewrites=rewrites,native_pointer_tail_transfers=sum(r['native_pointer_tail_transfers']for r in rewrites),original_guest_addresses_changed=0,unproven_common_native_guest_offset_requirement_removed=True,source_instruction_bytes_changed=0,original_game_execution_maximum=0,game_library_loaded_by_builder=False,unresolved_symbols=symbols,steps=steps,configuration_sha256=sha(a.config),archive_sha256=sha(archive),load_only_library_sha256=sha(library),runtime_authored_PC_range=[0xa0000000,0xa0001000],game_PC_execution_gate_open=False,full_phase8_runtime_allowed=False)
 (out/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in ['steps','rewrites']}))
if __name__=='__main__':main()
