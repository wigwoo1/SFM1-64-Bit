from pathlib import Path
import re
ranges=[(0x1029e030,0x1029e340),(0x102ae0a0,0x102ae740),(0x102ae740,0x102aeb10)]
lines=[]
with Path('work/ifm-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}):',line)
  if m:
   va=int(m[1],16)
   if va>=ranges[-1][1]:break
   if any(a<=va<b for a,b in ranges):lines.append(line)
Path('outputs/sfm64-ifm-sample-clock-disassembly.txt').write_text(''.join(lines))
print(''.join(lines))
