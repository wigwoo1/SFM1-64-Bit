from pathlib import Path
import re
lines=[]
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}):',line)
  if m:
   va=int(m[1],16)
   if va>=0x10060e10:break
   if va>=0x10060cf0:lines.append(line)
Path('outputs/sfm64-engine-client-init-clock-argument.txt').write_text(''.join(lines));print(''.join(lines))
