from pathlib import Path
import re
ranges=[(0x100d0600,0x100d06b0),(0x101bc490,0x101bc5c0),(0x101e5760,0x101e5800)]
lines=[]
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}):',line)
  if m and any(a<=int(m[1],16)<b for a,b in ranges):lines.append(line)
Path('outputs/sfm64-engine-animation-clock-writers.txt').write_text(''.join(lines));print(''.join(lines))
