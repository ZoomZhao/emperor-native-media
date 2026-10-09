import hashlib,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'samples'))
from stage22_ini import NativeINI
from stage17_file import NativeSavedFile
row=next(r for r in json.loads((ROOT/'evidence/stage19-resources-hashed.json').read_text())['rows']if r['relative_path']=='Emperor.ini');p=Path(row['copy_path']);assert hashlib.sha256(p.read_bytes()).hexdigest()==row['copy_sha256'];fd=NativeSavedFile(p);body=fd.read(0,row['copy_size']);assert hashlib.sha256(body).hexdigest()==row['copy_sha256'];core=NativeINI();rows=[]
for q in json.loads((ROOT/'evidence/stage22-ini-Wine.json').read_text())['queries']:
 args=[q[k].encode()if q[k]is not None else None for k in ['section','key']]
 if q['type']=='string':v,e,out=core.string(body,*args,q['fallback'].encode()if q['fallback']is not None else None,q['size'],bytes([0xa5])*128);equal=out.hex()==q['entire_128_byte_output_hex']
 else:v,e=core.integer(body,*args,q['fallback']);equal=True
 checks=dict(return_equal=v==q['return_value'],error_equal=e==q['last_error'],entire_buffer_equal=equal);rows.append(dict(query={k:q[k]for k in ['type','section','key','fallback']},checks=checks));assert all(checks.values()),(q,v,e,out.hex()if q['type']=='string'else '')
out=dict(all_passed=True,query_count=len(rows),rows=rows,source_sha256=row['copy_sha256'],backend='authored arm64 C parser, actual readonly baseline file bytes',scope='fixed ASCII file; no registry writes, locale or system changes',Wine_enumeration_small_buffer_single_NUL_and_87_behavior_recorded=True);(ROOT/'evidence/stage22-ini-native-regression.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps({k:v for k,v in out.items()if k!='rows'}));fd.dispose()
