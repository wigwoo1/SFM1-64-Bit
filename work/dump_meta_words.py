import sys
sys.path.insert(0,'work')
from pe_image import Image
x=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
for va in range(0x10AA17D8,0x10AA18A0,4): print(hex(va),hex(x.word(va)))
