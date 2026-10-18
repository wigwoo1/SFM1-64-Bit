from pathlib import Path
import struct,json
from pe_image import Image
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
def address(o):return next((image.base+rva+o-raw for rva,size,raw in image.sections if raw<=o<raw+size),None)
def occurrences(word):
 needle=struct.pack('<I',word);p=0
 while True:
  p=image.b.find(needle,p)
  if p<0:return
  yield p;p+=1
rows=[];pos=0
while True:
 pos=image.b.find(b'.?AVCDmeFilmClip@@\0',pos)
 if pos<0:break
 td=address(pos-8);pos+=1
 for ref in occurrences(td):
  col=address(ref-12)
  if not col:continue
  try:
   if image.word(col)!=0 or image.word(col+4)>0x10000:continue
   for locatorref in occurrences(col):
    vt=address(locatorref+4)
    if not vt:continue
    fn=image.word(vt+0x5c)
    if not (image.base<fn<image.base+0xa70000):continue
    rows.append(dict(type_descriptor=hex(td),locator=hex(col),vtable=hex(vt),object_offset=image.word(col+4),film_track_slot=hex(fn)))
  except ValueError:continue
Path('outputs/sfm64-film-clip-vtables.json').write_text(json.dumps(rows,indent=2),encoding='utf-8')
print(json.dumps(rows,indent=2))
