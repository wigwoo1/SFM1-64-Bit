from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
lines=[];checked=0
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if m:
   va=int(m[1],16)
   if va>=0x101805de:break
   if 0x10143c40<=va<0x10143d90 or 0x10180260<=va:
    expected=bytes.fromhex(m[2]);o=image.offset(va)
    assert image.b[o:o+len(expected)]==expected
    checked+=1;lines.append(line)
Path('outputs/sfm64-host-frame-duration-disassembly.txt').write_text(''.join(lines))
Path('outputs/sfm64-host-frame-duration-audit.json').write_text(json.dumps(dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),checked_instruction_prefixes=checked),indent=2))
print(''.join(lines));print('checked',checked)
