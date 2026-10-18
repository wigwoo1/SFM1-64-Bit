from pe_image import Image
import struct
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');o=im.offset(0x10A99650);print('mode13 constant',struct.unpack('<f',im.b[o:o+4])[0])
