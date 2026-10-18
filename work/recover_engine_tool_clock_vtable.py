from pe_image import Image
from pathlib import Path
import json
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
for count in range(100):
 base=0x1036678c-count*4
 try:
  col=im.word(base-4)
  if im.word(col)==0 and im.word(col+8)==0:
   name=im.string(im.word(col+12)+8)
   if name.startswith('.?AV'):print('vtable',hex(base),'setter slot',count,'COL',hex(col),'offset',im.word(col+4),name);Path('outputs/sfm64-engine-tool-clock-vtable.json').write_text(json.dumps(dict(vtable=hex(base),setter_slot=count,slot_byte_offset=hex(count*4),type=name),indent=2));break
 except (ValueError,UnicodeDecodeError):pass
