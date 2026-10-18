import sys
sys.path.insert(0,'work')
from pe_image import Image
x=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
for va in [0x10DDB3EC,0x10B42BEC,0x10DDB3D8,0x10DDB3C4]:
 try: print(hex(va),repr(x.string(va)))
 except Exception as e: print(e)
