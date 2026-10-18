from pathlib import Path
from pe_image import Image
import re,json
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
ranges=[(0x100832e0,0x10083420),(0x10083730,0x10083770),(0x100929b0,0x10092a80)]
lines=[];checked=0
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if m:
   va=int(m[1],16)
   if va>=ranges[-1][1]:break
   if any(a<=va<b for a,b in ranges):
    expected=bytes.fromhex(m[2]);o=im.offset(va)
    assert im.b[o:o+len(expected)]==expected
    checked+=1;lines.append(line)
Path('outputs/sfm64-split-screen-client-creation.txt').write_text(''.join(lines))
Path('outputs/sfm64-split-screen-client-creation-audit.json').write_text(json.dumps(dict(checked_instruction_prefixes=checked,allocation_size=hex(0x18fa0),embedded_client_displacement=8,constructor='0x100929b0',manager_slots=4),indent=2))
print(''.join(lines));print('checked',checked)
