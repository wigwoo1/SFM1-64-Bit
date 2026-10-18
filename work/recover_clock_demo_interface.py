from pathlib import Path
from pe_image import Image
import re,json
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll');vtable=0x10327104;col=im.word(vtable-4)
slots=[im.word(vtable+4*i) for i in range(13)]
ranges=[(0x10067000,0x10067090)]+[(slots[i],slots[i]+48) for i in (6,7,8,12)]
lines=[];checked=0
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if m:
   va=int(m[1],16)
   if va>=max(b for a,b in ranges):break
   if any(a<=va<b for a,b in ranges):
    b=bytes.fromhex(m[2]);o=im.offset(va);assert im.b[o:o+len(b)]==b;checked+=1;lines.append(line)
report=dict(vtable=hex(vtable),rtti=im.string(im.word(col+12)+8),complete_object_offset=im.word(col+4),slots=[hex(x) for x in slots],checked_instruction_prefixes=checked)
Path('outputs/sfm64-clock-demo-interface.json').write_text(json.dumps(report,indent=2));Path('outputs/sfm64-clock-demo-interface.txt').write_text(''.join(lines));print(json.dumps(report,indent=2));print(''.join(lines))
