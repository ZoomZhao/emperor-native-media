import json,sys
sys.dont_write_bytecode=True
from common import ROOT,S,sha,put,verify
assert json.loads((S/'evidence/finalize-v1.result.json').read_text())['returncode']==0
pins={str(p.relative_to(ROOT)):sha(p)for p in S.rglob('*')if p.is_file()and p.name!='manifest.json'};put(S/'manifest.json',{'hashes':pins,'prior_protection_hashes':5182,'original_game_execution':0,'old_ledger_closed':True});verify(pins);print(json.dumps({'sealed_and_rechecked':len(pins),'manifest_sha256':sha(S/'manifest.json')}))
