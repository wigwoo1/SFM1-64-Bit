from pathlib import Path
import re
from pe_image import Image
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10160e40,0x10160ea0),(0x1015b410,0x1015b4bb),(0x1029aae0,0x1029ac1e),(0x104d3760,0x104d3800),(0x104d34e0,0x104d3640),(0x104bba30,0x104bbacb)]
ranges.extend([(0x1017c0c0,0x1017c0e2),(0x104bc680,0x104bc70b),(0x1097adb0,0x1097ae21)])
ranges.append((0x107ca310,0x107ca398))
ranges.extend([(0x104bbf30,0x104bbf52),(0x104fbaf0,0x104fbc00)])
lines=[]
for line in Path('work/ifm-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2} )+)',line)
 if not m:continue
 address=int(m[1],16)
 if not any(lo<=address<hi for lo,hi in ranges):continue
 wire=bytes.fromhex(m[2]);o=image.offset(address)
 if image.b[o:o+len(wire)]!=wire:raise RuntimeError('disassembly does not match current ifm.dll')
 lines.append(line)
Path('outputs/sfm64-ifm-current-shot-trace.txt').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print('\n'.join(lines))
