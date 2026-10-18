import sys, struct
sys.path.insert(0,'work')
from pe_image import Image
x=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
for va in [0x10E7D34C,0x10AA17D8,0x10AA193C,0x10AA18A0,0x10E83614]:
 print('\n===',hex(va),'===')
 try:
  o=x.offset(va)
  for i in range(0,128,16):
   b=x.b[o+i:o+i+16]
   print(hex(va+i),b.hex(' '),repr(b))
 except Exception as e: print(e)
