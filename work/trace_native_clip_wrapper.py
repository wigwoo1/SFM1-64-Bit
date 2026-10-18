from pathlib import Path
from pe_image import Image
import re
path=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\sdktools\python\2.7\win32\Lib\site-packages\vs\datamodel_pyd.pyd')
image=Image(path)
lines=[]
for line in Path('work/datamodel-pyd-camera-disassembly.txt').read_text().splitlines():
 match=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2} )+)',line)
 if not match:continue
 address=int(match[1],16)
 if not any(lo<=address<hi for lo,hi in [(0x102f6300,0x102f6400),(0x100af660,0x100af700),(0x100af3e0,0x100af540),(0x100aeb60,0x100aeb90),(0x1009f8c0,0x1009f95b)]):continue
 wire=bytes.fromhex(match[2]);offset=image.offset(address)
 if image.b[offset:offset+len(wire)]!=wire:raise RuntimeError('cached disassembly differs from installed binary')
 lines.append(line)
Path('outputs/sfm64-native-film-clip-wrapper.txt').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print('\n'.join(lines))
