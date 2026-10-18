from pathlib import Path
from pe_image import Image
import re,json,hashlib
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
lines=[];checked=0
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if m:
   va=int(m[1],16)
   if va>=0x10143c40:break
   if va>=0x10143b10:
    b=bytes.fromhex(m[2]);o=im.offset(va);assert im.b[o:o+len(b)]==b
    checked+=1;lines.append(line)
strings=[]
for line in lines:
 m=re.search(r'push\s+([0-9A-F]{8})h',line)
 if m:
  try:strings.append(dict(address=m[1],text=im.string(int(m[1],16))))
  except (ValueError,UnicodeDecodeError):pass
Path('outputs/sfm64-clock-adjustment-state.txt').write_text(''.join(lines))
Path('outputs/sfm64-clock-adjustment-state-audit.json').write_text(json.dumps(dict(binary_sha256=hashlib.sha256(im.b).hexdigest(),checked_instruction_prefixes=checked,strings=strings),indent=2));print(''.join(lines));print('checked',checked)
