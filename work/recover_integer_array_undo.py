from pathlib import Path
import re
from pe_image import Image
p=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
lines=Path('work/ifm-disassembly.txt').read_text().splitlines()
index={int(m.group(1),16):i for i,s in enumerate(lines) if (m:=re.match(r'\s*([0-9A-F]{8}):',s))}
out=[]
for a,n in [(0x10368be0,120),(0x103645a0,65)]:out.extend(lines[index[a]:index[a]+n])
for vt in (0x10b38394,0x10b38980):
 out.append('undo vtable '+hex(vt))
 for slot in range(0,36,4):
  a=p.word(vt+slot);out.append('slot '+hex(slot)+' -> '+hex(a))
  if a in index:out.extend(lines[index[a]:index[a]+80])
Path('work/ifm-integer-array-undo-records.txt').write_text('\n'.join(out))
print('\n'.join(s for s in out if s.startswith('slot ') or s.startswith('undo vtable ')))



