import sys,struct
sys.path.insert(0,'work')
from pe_image import Image
x=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
for va in [0x10AB5F74,0x10AB5F94,0x10E2242C,0x10E8361C,0x10E7D34C]:
 print('\n',hex(va))
 for i in range(-8,33,4):
  try: v=x.word(va+i)
  except Exception: continue
  try: s=x.string(v) if v>=x.base else ''
  except: s=''
  print(hex(va+i),hex(v),s[:60])
