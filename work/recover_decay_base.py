from pe_image import Image
import struct
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');o=im.offset(0x10A8E010);print('decay base',hex(im.word(0x10A8E010)),struct.unpack('<f',im.b[o:o+4])[0])
