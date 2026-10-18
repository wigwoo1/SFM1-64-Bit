from pathlib import Path
import re,struct
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\bin\client.dll')
lines=[]
with Path('work/native-client-scale-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}):',line)
  if m:
   va=int(m[1],16)
   if 0x1000c690<=va<0x1000c7b0:lines.append(line)
   if va>=0x1000c7b0:break
print(''.join(lines));Path('outputs/sfm64-registered-animation-proxy-factory.txt').write_text(''.join(lines))
