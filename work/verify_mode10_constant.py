from pe_image import Image
import struct
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');o=im.offset(0x10B36620);print('mode10 cubic next coefficient',struct.unpack('<f',im.b[o:o+4])[0])
