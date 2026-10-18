from pathlib import Path
import re
lines=[]
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}):',line)
  if m:
   va=int(m[1],16)
   if va>=0x100929c0:break
   if va>=0x10092970:lines.append(line)
print(''.join(lines));Path('outputs/sfm64-client-clock-this-adjustment.txt').write_text(''.join(lines))
