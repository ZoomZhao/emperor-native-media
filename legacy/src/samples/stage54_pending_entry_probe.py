"""Test only the production missing-approval gate; never import an original runtime."""
import json,sys
from stage54_batch import LEDGER,APPROVAL,OLD,R,sha
if APPROVAL.exists():raise RuntimeError('approval has arrived; do not run the pending-gate probe')
before=sha(LEDGER);old=sha(OLD)
import checkpoint_startup54 as wrapper
assert 'unicorn' not in sys.modules and 'checkpoint_startup51' not in sys.modules
sys.argv=['checkpoint_startup54.py','--mode','restore','--resume','returnwatch51','--tag','pendinggate54']
result=wrapper.main()
assert result==2 and sha(LEDGER)==before and sha(OLD)==old
assert 'unicorn' not in sys.modules and 'checkpoint_startup51' not in sys.modules
out=dict(passed=True,entry_returncode=result,production_ledger_unchanged=True,
         old_closed_ledger_unchanged=True,original_instructions_executed=0,
         runtime_imported=False,emulator_imported=False,native_DLL_loaded=False,
         production_executions_claimed=0,explicit_new_approval_received=False)
(R/'evidence/stage54-pending-entry-probe.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(out))
