from pathlib import Path
from pe_image import Image
import json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\bin\client.dll');matches=[]
for rva,size,raw in image.sections:
 for offset in range(raw,raw+size-3,4):
  if image.u32(offset)!=0x100A0540:continue
  entry=image.base+rva+offset-raw
  for slot in range(80):
   table=entry-slot*4
   try:
    locator=image.word(table-4)
    if image.word(locator)!=0:continue
    descriptor=image.word(locator+12);name=image.string(descriptor+8)
    if not name.startswith('.?AV'):continue
    matches.append(dict(entry=hex(entry),vtable=hex(table),slot=slot,class_name=name,object_offset=image.word(locator+4)))
   except (ValueError,UnicodeDecodeError):pass
assert matches
offset=image.offset(0x100A054A);assert image.b[offset:offset+3]==bytes.fromhex('8B 75 08')
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),routine='0x100a0540',first_argument_load='0x100a054a: mov esi,[ebp+8]',matches=matches,limitations='Static RTTI and argument origin. SDK names support stage identity; live engine call and exact frame scheduling remain unverified.')
Path('outputs/sfm64-client-frame-stage-table.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(matches)
