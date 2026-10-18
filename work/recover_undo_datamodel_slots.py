from pathlib import Path
import re
from pe_image import Image
p=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
lines=Path('work/ifm-disassembly.txt').read_text().splitlines()
index={int(m.group(1),16):i for i,s in enumerate(lines) if (m:=re.match(r'\s*([0-9A-F]{8}):',s))}
out=[]
for slot in (0xf4,0x13c):
 a=p.word(0x10b37094+slot);out.append('vtable 10B37094 slot '+hex(slot)+' -> '+hex(a));out.extend(lines[index[a]:index[a]+85])
Path('work/ifm-undo-datamodel-slots.txt').write_text('\n'.join(out));print('\n'.join(out[:100]))

