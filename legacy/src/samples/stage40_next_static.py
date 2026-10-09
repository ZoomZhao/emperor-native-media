import json,re
from pathlib import Path
R=Path(__file__).resolve().parents[1];lines=(R/'evidence/stage16-original-text.disassembly.log').read_text().splitlines()
for lo,hi,name in [(0x475740,0x475940,'startup-after-audio'),(0x770ae0,0x770d00,'current-CRT-parser')]:
 out=[l for l in lines if(m:=re.match(r'^\s*([0-9a-f]+):',l))and lo<=int(m[1],16)<hi];(R/f'evidence/stage40-{name}.readonly.log').write_text('\n'.join(out)+'\n')
print((R/'evidence/stage40-startup-after-audio.readonly.log').read_text())
