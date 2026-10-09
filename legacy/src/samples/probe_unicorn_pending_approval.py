#!/usr/bin/env python3
"""Prepared only. Requires new explicit approval; not executed in stage 3.
Imports one fixed local official wheel and creates x86-32 context; no guest code.
"""
import sys,json,platform
from pathlib import Path
root=Path(__file__).resolve().parents[1]
package=root/'vendor-tools/unicorn-2.1.4'
if not (package/'unicorn').is_dir():raise SystemExit('Approved package not present')
sys.path.insert(0,str(package))
import unicorn
assert Path(unicorn.__file__).resolve().is_relative_to(package.resolve()) if hasattr(Path,'.is_relative_to') else str(Path(unicorn.__file__).resolve()).startswith(str(package.resolve())+'/')
assert unicorn.__version__=='2.1.4'
context=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_32)
print(json.dumps(dict(version=unicorn.__version__,module=unicorn.__file__,host=platform.machine(),x86_32_context_created=True,guest_instructions_executed=0)))
del context
