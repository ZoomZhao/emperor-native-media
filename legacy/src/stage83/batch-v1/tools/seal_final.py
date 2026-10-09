"""Final seal after all producing wrappers have closed their output logs."""
import json,sys
sys.dont_write_bytecode=True
from common import ROOT,S,V,sha,put,verify
from pipeline import accounting
old=json.loads((V/'delivery-manifest.json').read_text());mismatch=[]
for p,h in old['hashes'].items():
 if sha(ROOT/p)!=h:mismatch.append(p)
assert mismatch==['stage83/evidence/batch-v1-finalize.stdout.log'],mismatch
put(V/'evidence/seal-correction.json',{'previous_manifest':'stage83/batch-v1/delivery-manifest.json','invalidated_only_by_producer_stdout_closing_after_manifest_creation':mismatch,'previous_manifest_retained':True,'corrected_manifest':'stage83/batch-v1/delivery-manifest-v2.json','game_or_input_or_claim_changed':False,'original_game_execution':0})
text=(V/'reports/README.txt').read_text();(V/'reports/README-final.txt').write_text(text+'\n最终有效封存：batch-v1/delivery-manifest-v2.json。v1在其生成进程stdout日志关闭前纳入日志，故该一项hash随后变化；v1与失败原因保留于evidence/seal-correction.json。v2在所有日志生产进程完成后生成，未修改任何原版输入、claim、测试结果或旧保护文件。\n')
baseline=json.loads((V/'evidence/protection-baseline.json').read_text());verify(baseline['hashes']);assert accounting()['rounds_used']==1 and accounting()['role_processes_used']==1
roots=[V,*sorted((S/'runs').glob('batch-v1-*')),*sorted((S/'builds').glob('batch-v1-*'))]
files=[p for d in roots for p in d.rglob('*')if p.is_file()and not p.is_symlink()];files.extend(p for p in(S/'evidence').glob('batch-v1-*')if p.is_file())
pins={str(p.relative_to(ROOT)):sha(p)for p in files};manifest=V/'delivery-manifest-v2.json';put(manifest,{'hashes':pins,'protected_old_hashes_verified':baseline['count'],'old_ledger':'20/80/closed','actual_original_role_processes':1,'actual_native_game_invocations':0,'previous_seal_stdout_race_recorded':True});verify(pins)
print(json.dumps({'status':'FINAL_DELIVERY_SEAL_VERIFIED','files':len(pins),'protected_old_hashes_verified':baseline['count'],'manifest_sha256':sha(manifest),'new_original_rounds':1,'remaining_rounds':9}))
