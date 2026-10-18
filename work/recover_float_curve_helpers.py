from pathlib import Path
from pe_image import Image
import struct
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
print('scale',hex(im.word(0x10A8DFD4)),struct.unpack('<f',im.b[im.offset(0x10A8DFD4):im.offset(0x10A8DFD4)+4])[0])
lines=Path('work/ifm-disassembly.txt').read_text().splitlines()
for addr,name,count in [(0x104FCA50,'curve-evaluator',230),(0x1051B020,'curve-key-gather',230)]:
 start=next(i for i,s in enumerate(lines) if s.startswith(f'  {addr:08X}:'))
 part=lines[start:start+count]
 Path(f'work/ifm-float-{name}.txt').write_text('\n'.join(part)+'\n')
 if name=='curve-evaluator': print('\n'.join(part[:65]))
