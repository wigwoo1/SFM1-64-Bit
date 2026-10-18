from pathlib import Path
from pe_image import Image
import struct,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll');target=0x10057340
matches=[]
for rva,size,raw in image.sections:
 for offset in range(raw,raw+size-3,4):
  if image.u32(offset)!=target:continue
  address=image.base+rva+offset-raw
  surrounding=[]
  for delta in range(-12,13):
   p=offset+delta*4
   if raw<=p<=raw+size-4:surrounding.append(dict(relative_slot=delta,address=hex(address+delta*4),pointer=hex(image.u32(p))))
  matches.append(dict(address=hex(address),surrounding=surrounding))
assert matches
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),target=hex(target),aligned_pointer_occurrences=matches,limitations='Aligned file-backed pointer scan only; candidate vtable identity and slot boundaries require independent constructor/RTTI/interface evidence.')
Path('outputs/sfm64-flex-caller-pointer-locations.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
for match in matches:print(json.dumps(match))
