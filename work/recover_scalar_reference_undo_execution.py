from pathlib import Path
import re
from pe_image import Image
p=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
lines=Path('work/ifm-disassembly.txt').read_text().splitlines()
index={int(m.group(1),16):i for i,s in enumerate(lines) if (m:=re.match(r'\s*([0-9A-F]{8}):',s))}
out=[]
for slot in (0,4,0x14,0x20):
 a=p.word(0x10b3872c+slot);out.append('reference undo slot '+hex(slot)+' -> '+hex(a));out.extend(lines[index[a]:index[a]+90])
a=0x1025bf80;out.extend(lines[index[a]:index[a]+65])
Path('work/ifm-scalar-reference-undo-execution.txt').write_text('\n'.join(out));print('\n'.join(out[:185]))
