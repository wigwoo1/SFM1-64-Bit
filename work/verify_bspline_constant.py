from pe_image import Image
import struct
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');o=im.offset(0x10B84570);print('constant:',struct.unpack('<f',im.b[o:o+4])[0])
