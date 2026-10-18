import sys
sys.path.insert(0,'work')
from pe_image import Image
x=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
for va in [0x10A8F458,0x10AA7D8C,0x10AB60B8,0x10AB60C4,0x10FD43A0,0x10AB5F94]:
 try: print(hex(va),repr(x.string(va)))
 except Exception as e: print(hex(va),e)
