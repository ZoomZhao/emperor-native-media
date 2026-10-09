from core import *
for n in ('configs','evidence','reports','runs','builds'):(V/n).mkdir(parents=True,exist_ok=True)
base=json.loads((OLD/'evidence/protection-baseline.json').read_text())['hashes']
base.update(json.loads((OLD/'manifest.json').read_text())['hashes']);base[str((OLD/'manifest.json').relative_to(ROOT))]=sha(OLD/'manifest.json');verify(base)
put(V/'evidence/protection-baseline.json',{'count':len(base),'hashes':base});put(V/'configs/source.json',json.loads((OLD/'configs/source.json').read_text()))
print(json.dumps({'protected_old_pins':len(base),'remaining_rounds':9,'original_execution':0}))
