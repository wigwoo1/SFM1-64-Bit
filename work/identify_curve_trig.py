from pathlib import Path
from pe_image import Image
import struct
lines=Path('work/ifm-disassembly.txt').read_text().splitlines();i=next(i for i,s in enumerate(lines) if s.startswith('  10A0FA7F:'));print('\n'.join(lines[i:i+50]))
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
for a in [0x10DFE1D0,0x10DFE1D8,0x10DFE1E0,0x10DFE1E8]:
 o=im.offset(a);print(hex(a),struct.unpack('<d',im.b[o:o+8])[0])
