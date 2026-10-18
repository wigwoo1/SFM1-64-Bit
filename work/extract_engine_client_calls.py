from pathlib import Path
import re
lines=[];remaining=0
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  if '104B5594h' in line:remaining=24
  if remaining:lines.append(line);remaining-=1
Path('outputs/sfm64-engine-client-interface-call-contexts.txt').write_text(''.join(lines))
print(''.join(lines)[:11000])
