#!/usr/bin/env python3
"""Move SR aliases before data references; no instructions are changed."""
import sys
from pathlib import Path
p=Path(sys.argv[1]);lines=p.read_text().splitlines(True)
aliases=[l for l in lines if l.startswith('define ')]
lines=[l for l in lines if not l.startswith('define ')]
index=next(i for i,l in enumerate(lines) if l.startswith('datasegment'))
lines[index:index]=aliases+['\n']
p.write_text(''.join(lines))
print(f'Moved {len(aliases)} alias definitions before data; instruction includes unchanged.')
