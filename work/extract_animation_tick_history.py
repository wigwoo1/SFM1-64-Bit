from pathlib import Path
import re
ranges=[(0x101439d0,0x10143ae0)]
lines=[]
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}):',line)
  if m:
   va=int(m[1],16)
   if va>=0x10143ae0:break
   if va>=0x101439d0:lines.append(line)
Path('outputs/sfm64-animation-tick-history-helper.txt').write_text(''.join(lines));print(''.join(lines))
