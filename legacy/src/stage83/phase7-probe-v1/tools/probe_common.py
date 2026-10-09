"""Data-only gate helpers. No execution engine or native library imports."""
import sys,json,hashlib,time,struct,copy
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[3]
N=ROOT/'stage83/phase7-probe-v1'
sys.path.insert(0,str(ROOT/'stage83/phase7-v2/tools'))
from core import sha,put,verify,Boundary,HostStore,SOURCE,fixed
LEDGER_SHA='4e499d8560cc4c7a9bc4a2e725e27f502fa13979de6366e9f66bd06f237edcc9'
def accounting():
 claims=list((ROOT/'stage83/runs').glob('batch-v1-round*/round.claim.json'))
 return {'used':len(claims),'remaining':10-len(claims),'maximum':10}
def check_old():
 p=ROOT/'evidence/stage55-batch-ledger.json';assert sha(p)==LEDGER_SHA
 v=json.loads(p.read_text());assert len(v['executions'])==20 and v['remaining_processes']==80 and v['goal_closed']
def source_config():return json.loads((ROOT/'stage83/phase7-v1/configs/source.json').read_text())
def intv(v):return int(v,0)if isinstance(v,str)else v
