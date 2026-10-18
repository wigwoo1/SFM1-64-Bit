from pathlib import Path
import re
lines=[]
with Path('work/native-client-scale-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}):',line)
  if m:
   va=int(m[1],16)
   if va>=0x10099940:break
   if va>=0x10099820:lines.append(line)
Path('outputs/sfm64-animation-clock-installation-disassembly.txt').write_text(''.join(lines));print(''.join(lines))
