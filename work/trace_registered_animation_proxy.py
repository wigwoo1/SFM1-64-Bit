from pe_image import Image
from pathlib import Path
import struct,json,re
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\bin\client.dll')
for va in [0x104878d2,0x104c0b84]:
 print(hex(va),im.b[im.offset(va)-24:im.offset(va)+48].hex())
terms=[b'.?AVCAnimatedTextureProxy@@',b'.?AVCBaseAnimatedTextureProxy@@']
for term in terms:
 o=im.b.find(term);print(term,o)
 if o>=0:
  for r,s,raw in im.sections:
   if raw<=o<raw+s:print('VA',hex(im.base+r+o-raw))
lines=[]
with Path('work/native-client-scale-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}):',line)
  if m:
   va=int(m[1],16)
   if va>=0x10487950:break
   if 0x10487880<=va:lines.append(line)
print(''.join(lines))
