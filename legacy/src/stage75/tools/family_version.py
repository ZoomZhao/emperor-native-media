"""New configuration/builder/runtime versions; earlier successful build retained."""
import json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];S=ROOT/'stage75'
def main():
 snap=S/'builds/type53-v4/source-snapshot';snap.mkdir(exist_ok=False)
 for p in (S/'tools').glob('*'):
  if p.is_file():shutil.copyfile(p,snap/p.name)
 cfg=json.loads((S/'configs/phase8-type53.json').read_text());inventory=json.loads((S/'evidence/vtable-inventory.json').read_text());vt=[]
 for row in inventory['tables']:
  if row['target200']=='0x5d4770'and row['target612']=='0x5d4a60'and row['full_slot_pointers'][179]=='0x546be0'and row['full_slot_pointers'][182]=='0x546c40':vt.append(row['vtable'])
 assert vt==['0x7b72c8','0x7bdbf4','0x7bded4','0x7be1bc']
 cfg['id']='phase8-type53-family-guarded';cfg['profile_contract']['root_vtable_whitelist']=vt
 cfg['profile_contract']['kind']='Four fixed original vtables sharing all four required methods; typeword53/component guards; only first vtable saved-observed; NOT all input/object types'
 cfg['profile_contract']['additional_tables_not_saved_observed']=vt[1:]
 out=S/'configs/phase8-type53-family.json';assert not out.exists();out.write_text(json.dumps(cfg,indent=2)+'\n')
 text=(S/'tools/profile_runtime.inc').read_text().replace('s75_require(s75_read(object,4)==0x7b72c8&&s75_read(object+20,2)==53,object);','uint32_t vtable=s75_read(object,4);int accepted=0;\n  for(uint32_t k=0;k<S75_ROOT_TABLE_COUNT;k++)if(vtable==s75_root_tables[k])accepted=1;\n  s75_require(accepted&&s75_read(object+20,2)==53,object);')
 text=text.replace('0x7b7390','vtable+200').replace('0x7b752c','vtable+612').replace('0x7b7594','vtable+716').replace('0x7b75a0','vtable+728').replace('0x7b72c8);','vtable);')
 rt=S/'tools/profile_runtime_v2.inc';assert not rt.exists();rt.write_text(text)
 builder=(S/'tools/builder.py').read_text().replace("runtime+=(ROOT/'stage75/tools/profile_runtime.inc').read_text();(R/'runtime.c').write_text(runtime)","tables=cfg['profile_contract'].get('root_vtable_whitelist',[cfg['profile_contract']['root_vtable']])\n    runtime+='\\n#define S75_ROOT_TABLE_COUNT %d\\nstatic const uint32_t s75_root_tables[]={%s};\\n'%(len(tables),','.join(tables))\n    runtime+=(ROOT/'stage75/tools/profile_runtime_v2.inc').read_text();(R/'runtime.c').write_text(runtime)")
 builder=builder.replace('libstage73_SR_diff_UNEXECUTED.dylib','libphase8_type53_UNEXECUTED.dylib')
 builder=builder.replace("actual_new_semantics_unvalidated_until_differential=True)","actual_new_semantics_unvalidated_until_differential=True,profile_contract=cfg['profile_contract'],runtime_eligible=False,exact_entry_CPU_and_stop_SP_missing=cfg['stop_SP']is None,indirect_binding_count=len(targetrows),manual_indirect_edge_metadata=len(cfg['resolved_indirect_edges']),manual_branch_domain_proofs=len(cfg['proved_branch_targets']))")
 bp=S/'tools/builder_v2.py';assert not bp.exists();bp.write_text(builder)
 print(json.dumps(dict(new_config=str(out),whitelisted_vtables=vt,new_builder=str(bp),earlier_source_snapshot=str(snap))))
if __name__=='__main__':main()
