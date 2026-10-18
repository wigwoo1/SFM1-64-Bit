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
   if va>=0x10090270:break
   if 0x10090140<=va:
    expected=bytes.fromhex(m[2]);o=im.offset(va)
    assert im.b[o:o+len(expected)]==expected,hex(va)
    checked+=1;lines.append(line)
Path('outputs/sfm64-client-clock-accessors.txt').write_text(''.join(lines))
Path('outputs/sfm64-client-clock-accessors-audit.json').write_text(json.dumps(dict(binary_sha256=hashlib.sha256(im.b).hexdigest(),checked_instruction_prefixes=checked),indent=2))
print(''.join(lines));print('checked',checked)
