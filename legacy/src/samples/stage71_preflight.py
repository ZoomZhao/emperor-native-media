"""Static-only preflight. Never imports Unicorn or loads a native library."""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
    out=dict(status='REFUSED_EXECUTION_BUILD_ONLY',original_or_native_instructions_executed=0,
        unresolved_contracts=[
            'DS/SS/ES cached flat bases have no verified saved-state/provenance evidence.',
            'Unicorn opaque tid1 adapter exists as uncalled source; public restore/read not performed.',
            'SR static native data-label addresses must be bound to original guest map, not blindly PTR2REG-rebased.',
            'Original guest return PCs require explicit SR continuation mapping and caller-stop trap.',
            'SR c_city_resume wrapper pushes __return_procedure onto guest stack and is unsuitable for mid-function restore; built own stack-neutral entry does not resolve return mapping.',
            'SR REP helper has no checked per-element fault/write trace or cold-persistence boundary.',
            'Native state loader, ordered writes, x87 sidecar and exact return-state oracle are not implemented.' ],
        ledger='Closed20/80 remains unchanged; no run21 or automatic historical replay authorization',
        next_scope='Only develop/review SR mapping and context preflight adapters until all refusals close; no own instruction-dispatcher expansion')
    (ROOT/'evidence/stage71-preflight-refusal.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out))
if __name__=='__main__':main()
