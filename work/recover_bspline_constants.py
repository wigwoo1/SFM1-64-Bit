from pe_image import Image
import struct
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
for a in [0x10B8456C,0x10AAFA48]:
 o=im.offset(a);print(hex(a),struct.unpack('<f',im.b[o:o+4])[0])
