from pathlib import Path
import re
lines=[]
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}):',line)
  if m:
   va=int(m[1],16)
   if va>=0x10182f90:break
   if va>=0x10182f00:lines.append(line)
Path('outputs/sfm64-animation-clock-threadmode-selection.txt').write_text(''.join(lines));print(''.join(lines))
