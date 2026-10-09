"""Select fixed disassembly ranges only. Never execute original sample APIs."""
import re,json
from pathlib import Path
R=Path(__file__).resolve().parents[1]
for source,windows in [('evidence/stage16-original-text.disassembly.log',[(0x761522,0x761880,'Emperor-sample-pool'),(0x761cfe,0x761df0,'Emperor-sample-reset')]),('evidence/stage25-miles-text-readonly.stdout.log',[(0x21103f00,0x21104000,'MSS-sample-allocate-wrapper'),(0x211041e0,0x211042d0,'MSS-sample-init-wrapper'),(0x21104fb0,0x21105070,'MSS-sample-status-wrapper'),(0x211138f0,0x21113b60,'MSS-sample-allocate-init-core'),(0x21117f30,0x21117f70,'MSS-sample-status-core')])]:
 lines=(R/source).read_text().splitlines()
 for lo,hi,name in windows:
  chosen=[line for line in lines if(m:=re.match(r'^\s*([0-9a-f]+):',line))and lo<=int(m[1],16)<hi]
  (R/('evidence/stage38-'+name+'.readonly.log')).write_text('\n'.join(chosen)+'\n')
print(json.dumps(dict(original_sample_code_executed=False,source='fixed existing readonly disassembly',windows=7)))
