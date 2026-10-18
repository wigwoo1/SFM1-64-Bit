from pathlib import Path
from collections import deque
from pe_image import Image
import re, json, hashlib
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
history=deque(maxlen=12); lines=[]; refs=[]; remaining=0
offsets=(0x8838,0x8839,0x883c,0x8a4c,0x8a4d,0x8a50)
patterns=tuple(f'+{n:08X}h]' for n in offsets)
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  if any(p in line for p in patterns):
   refs.append(line.strip()); lines.extend(history); remaining=16
  if remaining: lines.append(line); remaining-=1
  history.append(line)
checked=0
for line in set(lines):
 m=re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if m:
  b=bytes.fromhex(m[2]); o=im.offset(int(m[1],16)); assert im.b[o:o+len(b)]==b; checked+=1
report=dict(binary_sha256=hashlib.sha256(im.b).hexdigest(),checked_unique_instruction_prefixes=checked,
 references=refs,limitations='Offset matches require independent owner identification; static reads/writes do not prove live playback lifecycle.')
Path('outputs/sfm64-alternate-clock-field-contexts.txt').write_text(''.join(lines))
Path('outputs/sfm64-alternate-clock-field-audit.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
