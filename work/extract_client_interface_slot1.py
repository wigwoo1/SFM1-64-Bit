from pathlib import Path
import re
lines=[]
with Path('work/native-client-scale-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}):',line)
  if m:
   va=int(m[1],16)
   if va>=0x100a0af0:break
   if va>=0x100a0a30:lines.append(line)
print(''.join(lines));Path('outputs/sfm64-client-interface-slot1-disassembly.txt').write_text(''.join(lines))
