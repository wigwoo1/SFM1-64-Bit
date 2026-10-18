from pathlib import Path
import re
ranges=[(0x10176ae0,0x10176b80),(0x10183090,0x10183130),(0x101bc5f0,0x101bc6b0)]
lines=[]
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}):',line)
  if m and any(a<=int(m[1],16)<b for a,b in ranges):lines.append(line)
Path('outputs/sfm64-engine-animation-interval-writers.txt').write_text(''.join(lines));print(''.join(lines))
