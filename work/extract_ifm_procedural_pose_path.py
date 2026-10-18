from pathlib import Path
import re
lines=Path('work/ifm-disassembly.txt').read_text().splitlines();ranges=[(0x105C3580,0x105C3630),(0x1072CB50,0x1072CD60)]
for first,last in ranges:
 selected=[]
 for line in lines:
  match=re.match(r'\s*([0-9A-F]{8}):',line)
  if match and first<=int(match[1],16)<last:selected.append(line)
 text='\n'.join(selected)+'\n';Path(f'outputs/sfm64-ifm-pose-path-{first:08x}.txt').write_text(text);print(text)
