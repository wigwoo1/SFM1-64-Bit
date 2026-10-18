from pe_image import Image
import struct
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\sdktools\python\2.7\win32\Lib\site-packages\vs\valve_pyd.pyd')
for va,fmt in [(0x101E6648,'d'),(0x10385798,'d'),(0x101E63F8,'f'),(0x101E640C,'f')]:print(hex(va),struct.unpack_from('<'+fmt,im.b,im.offset(va))[0])
