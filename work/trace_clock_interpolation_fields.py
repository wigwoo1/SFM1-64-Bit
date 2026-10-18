from pathlib import Path
from collections import deque
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
history=deque(maxlen=10);lines=[];remaining=0;refs=[]
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  if any(x in line for x in ('+00004A34h]','+00004A38h]','+00004A2Ch]',
                           '106460F4h','10644EA4h','106460E8h','106460F0h')):
   refs.append(line.strip());lines.extend(history);remaining=18
  if remaining:lines.append(line);remaining-=1
  history.append(line)
checked=0
for line in set(lines):
 m=re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if m:
  expected=bytes.fromhex(m[2]);o=image.offset(int(m[1],16))
  assert image.b[o:o+len(expected)]==expected
  checked+=1
Path('outputs/sfm64-clock-interpolation-field-contexts.txt').write_text(''.join(lines))
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),checked_unique_instruction_prefixes=checked,references=refs,limitations='Offsets only; classify this view before assigning semantics')
Path('outputs/sfm64-clock-interpolation-field-audit.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
