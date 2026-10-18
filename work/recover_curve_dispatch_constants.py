from pe_image import Image
from pathlib import Path
import struct
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
out=[f'{i}: {im.word(0x10066CF8+i*4):08X}' for i in range(16)]
for addr in [0x10A8BAA4,0x10A8C508]:
 off=im.offset(addr);out.append(f'{addr:08X}: {struct.unpack("<f",im.b[off:off+4])[0]}')
for addr in [0x10A99898,0x10A8BBA0,0x10A99890,0x10A8AB28]:
 off=im.offset(addr);out.append(f'{addr:08X}: {struct.unpack("<d",im.b[off:off+8])[0]}')
Path('work/ifm-curve-dispatch-map.txt').write_text('\n'.join(out)+'\n');print('\n'.join(out))
