from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tier0.dll')
o=im.u32(60)+24; d=im.base+im.u32(o+96)
n=im.base+im.word(d+32); a=im.base+im.word(d+36); f=im.base+im.word(d+28)
for i in range(im.word(d+24)):
 name=im.string(im.base+im.word(n+4*i))
 if name in ['ThreadInMainThread','Plat_FloatTime']:
  target=im.base+im.word(f+4*im.u16(im.offset(a+2*i)))
  print(name,hex(target))
