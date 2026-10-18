import sys
sys.path.insert(0,'work')
from pe_image import Image
x=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
base=0x10AB5F94
for off in range(0,0x110,4):
 va=base+off
 try: v=x.word(va)
 except: break
 try: s=x.string(v) if v>=x.base else ''
 except: s=''
 print(f'{off:03x} {va:08X} {v:08X} {s[:50]}')
