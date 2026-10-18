from pe_image import Image
import struct
from pathlib import Path
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
lines=[]
for name in [b'subClipTrackGroup\0',b'tracks\0']:
 p=0
 while True:
  p=image.b.find(name,p)
  if p<0:break
  va=next((image.base+rva+p-raw for rva,size,raw in image.sections if raw<=p<raw+size),None);p+=1
  if not va:continue
  cursor=0
  while True:
   cursor=image.b.find(struct.pack('<I',va),cursor)
   if cursor<0:break
   ref=next((image.base+rva+cursor-raw for rva,size,raw in image.sections if raw<=cursor<raw+size),None);cursor+=1
   lines.append(f'{name!r} string={va:08X} reference={ref:08X}')
Path('outputs/sfm64-film-group-registration-locations.txt').write_text('\n'.join(lines)+'\n',encoding='utf-8');print('\n'.join(lines))
