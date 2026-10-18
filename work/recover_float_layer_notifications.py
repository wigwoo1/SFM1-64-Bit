from pathlib import Path
import re
from pe_image import Image
p=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
lines=Path('work/ifm-disassembly.txt').read_text().splitlines()
index={int(m.group(1),16):i for i,s in enumerate(lines) if (m:=re.match(r'\s*([0-9A-F]{8}):',s))}
out=[]
for slot in (0,4,8):
 address=p.word(0x10b9202c+slot)
 out.append('float layer slot '+hex(slot)+' -> '+hex(address))
 out.extend(lines[index[address]:index[address]+75])
Path('work/ifm-float-layer-notification-slots.txt').write_text('\n'.join(out))
print('\n'.join(out))
