from pe_image import Image
import struct
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');o=im.offset(0x10AA53C8);print('mode12 epsilon',hex(im.word(0x10AA53C8)),struct.unpack('<f',im.b[o:o+4])[0])
