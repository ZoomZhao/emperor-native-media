#!/usr/bin/env python3
"""Finite original-data hashes from actual PE startup names; no mapping/execution."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];dep=json.loads((ROOT/'evidence/stage18-elevation-dependency.json').read_text());original=Path(dep['original_path']).parent.parent;copy=Path(dep['copy_path']).parent.parent;groups=json.loads((ROOT/'evidence/stage19-graphics-groups-static.json').read_text());rows=[]
for group in groups['groups']:
 if not group['name']:continue
 name=group['name'];paths=['data/ChinaMapProj.sg3','data/'+name+'.jpg'] if group['index']==13 else ['data/'+name+'.sg3','data/'+name+'.555']
 for rel in paths:rows.append(dict(group_index=group['index'],group_name=name,relative_path=rel,request_provenance='original group table; MapOfChina special branches 0x5cd309 and 0x5cd5e3',actually_opened_during_stage19=False))
for rel in ['Emperor.ini','EmperorText.eng','EmperorMM.eng','EmperorMap.inf','Model/EmperorEventMessageCategories.txt']:rows.append(dict(relative_path=rel,request_provenance='original PE filenames / WinMain configuration-language-data calls',actually_opened_during_stage19=False))
for row in rows:
 row.update(original_path=str(original/row['relative_path']),copy_path=str(copy/row['relative_path']))
 for role,base in [('original',original),('copy',copy)]:
  p=base/row['relative_path'];row[role+'_exists']=p.is_file()
  if p.is_file():
   body=p.read_bytes();row[role+'_size']=len(body);row[role+'_sha256']=hashlib.sha256(body).hexdigest()
 row['original_copy_equal']=row.get('original_sha256') is not None and row.get('original_sha256')==row.get('copy_sha256')
result=dict(EXE_sha256=groups['source_original_EXE_sha256'],row_count=len(rows),rows=rows,all_present_equal=all(r['original_copy_equal']for r in rows),readonly_review_only=True,no_new_game_resource_mapping_added=True,all_transitive_resources_not_claimed=True)
(ROOT/'evidence/stage19-resources-hashed.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(rows=len(rows),all_present_equal=result['all_present_equal'],missing=[r['relative_path']for r in rows if not r['original_exists']],unequal=[r['relative_path']for r in rows if r['original_exists']and not r['original_copy_equal']],total_original_bytes=sum(r.get('original_size',0)for r in rows))))
