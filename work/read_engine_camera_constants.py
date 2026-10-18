from pe_image import Image
import struct
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\sdktools\python\2.7\win32\Lib\site-packages\vs\datamodel_pyd.pyd')
for va in [0x104E8C0C,0x104E3C04]:
 print(hex(va),struct.unpack_from('<f',im.b,im.offset(va))[0])
lines=open('work/datamodel-pyd-camera-disassembly.txt').read().splitlines()
start=next(i for i,line in enumerate(lines) if line.startswith('  1005B370:'))
print('\n'.join(lines[start:start+28]))
