from pathlib import Path
from pe_image import Image
import re,json,hashlib,struct
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
ranges=[(0x100832d0,0x10083300),(0x1008f7e0,0x1008f840),
        (0x10143990,0x101439d0),(0x1017da60,0x1017daa0),
        (0x101ebd20,0x101ebd50)]
lines=[];checked=0
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if m:
   va=int(m[1],16)
   if va>=ranges[-1][1]:break
   if any(a<=va<b for a,b in ranges):
    expected=bytes.fromhex(m[2]);offset=image.offset(va)
    assert image.b[offset:offset+len(expected)]==expected,hex(va)
    lines.append(line);checked+=1
word=image.word(0x1033662c)
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),checked_instruction_prefixes=checked,
 current_time_subtraction_constant=dict(address='0x1033662c',word=hex(word),value=struct.unpack('<f',struct.pack('<I',word))[0]))
Path('outputs/sfm64-clock-predicates.txt').write_text(''.join(lines))
Path('outputs/sfm64-clock-predicates-audit.json').write_text(json.dumps(report,indent=2))
print(''.join(lines));print(json.dumps(report,indent=2))
