from pe_image import Image
import struct
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');o=im.offset(0x10A8AEF8);print(struct.unpack('<f',im.b[o:o+4])[0])
