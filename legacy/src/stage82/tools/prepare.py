import json,sys
from pathlib import Path
sys.dont_write_bytecode=True
from common import ROOT,S,sha,put,decode
def main():
 S.mkdir(exist_ok=True)
 for d in ('evidence','configs','runs','builds','reports'):(S/d).mkdir(exist_ok=True)
 old=json.loads((ROOT/'stage81/evidence/protection-baseline.json').read_text());old=old.get('hashes',old.get('protected',old))
 if not isinstance(old,dict):raise RuntimeError('Unknown previous baseline')
 manifest=json.loads((ROOT/'stage81/manifest.json').read_text());pins=dict(old);pins.update(manifest['hashes']);pins['stage81/manifest.json']=sha(ROOT/'stage81/manifest.json')
 put(S/'evidence/protection-baseline.json',{'hashes':pins,'count':len(pins)})
 state=ROOT/'runs/stage12-state-cycle62p19/state.json';raw=json.loads(state.read_text());h=decode(raw['host'])
 summary={'current_PC':raw['registers']['eip'],'current_tid':h['current_tid'],'heap_records':len(h['allocations']),'lock_records':len(h['critical_sections']),'thread_statuses':{k:v['status']for k,v in h['guest_threads20'].items()},'watch_state':encode_watch(h['watch_state54']),'no_original_execution':True}
 put(S/'evidence/source-static-summary.json',summary);print(json.dumps({k:v for k,v in summary.items()if k!='watch_state'}))
def encode_watch(v):
 from common import encode
 return encode(v)
if __name__=='__main__':main()
