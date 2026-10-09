"""Offline complete host conversion, historical data only."""
from core import *
def main():
 t=time.monotonic();s=source();host=decode(s['host']);manifest=prepare(host,V/'prepared-host')
 meta={k:v for k,v in s.items()if k!='host'}
 meta.update(original_source=str((SOURCE/'state.json').relative_to(ROOT)),original_source_pins=PINS,host_manifest=str(manifest.relative_to(ROOT)),host_manifest_sha256=sha(manifest),original_execution_authorized=False)
 put(V/'configs/source.json',meta)
 old=json.loads((ROOT/'stage83/repair-v2/evidence/protection-baseline.json').read_text())['hashes']
 for m in (ROOT/'stage83/repair-v2/manifest.json',ROOT/'stage83/input-route-review-v1/manifest.json'):
  old.update(json.loads(m.read_text())['hashes']);old[str(m.relative_to(ROOT))]=sha(m)
 verify(old);put(V/'evidence/protection-baseline.json',{'hashes':old,'count':len(old)})
 put(V/'evidence/prepare.json',{'host_fields':len(host),'source_current_tid':host['current_tid'],'thread_statuses':{str(k):v['status']for k,v in host['guest_threads20'].items()},'seconds':time.monotonic()-t,'source_pins':PINS,'all_old_protected_hashes_passed':len(old),'original_execution':0})
 print(json.dumps({'status':'COMPLETE_PHASE7_HOST_PREPARED','fields':len(host),'old_pins':len(old),'seconds':time.monotonic()-t}))
if __name__=='__main__':main()
