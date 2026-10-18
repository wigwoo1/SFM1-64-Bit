from pathlib import Path
from pe_image import Image
import re,json,struct,hashlib
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll');lines=[];checked=0
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if m:
   va=int(m[1],16)
   if va>=0x10143990:break
   if va>=0x10143790:
    b=bytes.fromhex(m[2]);o=im.offset(va);assert im.b[o:o+len(b)]==b
    checked+=1;lines.append(line)
constants={}
for va in (0x1031df40,0x1031cbb0,0x1033af88,0x1031dcc0,0x1031e234):
 word=im.word(va);constants[hex(va)]=dict(word=hex(word),value=struct.unpack('<f',struct.pack('<I',word))[0])
Path('outputs/sfm64-clock-drift-helpers.txt').write_text(''.join(lines))
Path('outputs/sfm64-clock-drift-helpers-audit.json').write_text(json.dumps(dict(binary_sha256=hashlib.sha256(im.b).hexdigest(),checked_instruction_prefixes=checked,constants=constants),indent=2))
print(''.join(lines));print('checked',checked)
