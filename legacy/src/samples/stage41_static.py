import json,re
from pathlib import Path
R=Path(__file__).resolve().parents[1];lines=(R/'evidence/stage16-original-text.disassembly.log').read_text().splitlines()
for lo,hi,name in [(0x527850,0x527990,'current-loop'),(0x527ac0,0x527c80,'adjacent-grid-loops'),(0x4757d0,0x475b20,'startup-after-models')]:
 out=[l for l in lines if(m:=re.match(r'^\s*([0-9a-f]+):',l))and lo<=int(m[1],16)<hi];(R/f'evidence/stage41-{name}.readonly.log').write_text('\n'.join(out)+'\n')
result=dict(registers=json.loads((R/'evidence/stage40-result.json').read_text())['current_guest_registers'],original_code_executed=False)
(R/'evidence/stage41-static.json').write_text(json.dumps(result,indent=2));print(json.dumps(result));print((R/'evidence/stage41-current-loop.readonly.log').read_text())
