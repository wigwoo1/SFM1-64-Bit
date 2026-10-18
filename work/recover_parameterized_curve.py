from pathlib import Path
from pe_image import Image
import struct
lines=Path('work/ifm-disassembly.txt').read_text().splitlines();i=next(i for i,s in enumerate(lines) if s.startswith('  1049B7E0:'));p=lines[i:i+150];Path('work/ifm-curve-helper-1049B7E0.txt').write_text('\n'.join(p)+'\n');print('\n'.join(p[:85]))
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
for a in [0x10A99868,0x10A8A66C]:
 o=im.offset(a);print(hex(a),struct.unpack('<f',im.b[o:o+4])[0])
