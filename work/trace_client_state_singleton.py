from pathlib import Path
from collections import deque
from pe_image import Image
import re,json,hashlib
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
history=deque(maxlen=10);lines=[];remaining=0;references=[]
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  if '103FFAA4h' in line or '103FFAA0h' in line:
   lines.extend(history);remaining=20;references.append(line.strip())
  if remaining:lines.append(line);remaining-=1
  history.append(line)
checked=0
for line in set(lines):
 m=re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if m:
  expected=bytes.fromhex(m[2]);o=im.offset(int(m[1],16))
  assert im.b[o:o+len(expected)]==expected
  checked+=1
Path('outputs/sfm64-client-state-singleton-references.txt').write_text(''.join(lines))
initial_words={hex(va):hex(im.word(va)) for va in (0x103ffaa0,0x103ffaa4,0x103ffaa8,0x103ffaac)}
vtable=im.word(0x103ffaa0);col=im.word(vtable-4)
manager=dict(vtable=hex(vtable),rtti_name=im.string(im.word(col+12)+8),complete_object_offset=im.word(col+4),slots=[hex(im.word(vtable+4*i)) for i in range(15)])
report=dict(binary_sha256=hashlib.sha256(im.b).hexdigest(),checked_unique_instruction_prefixes=checked,references=references,initial_words=initial_words,manager=manager)
Path('outputs/sfm64-client-state-singleton-audit.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2));print(''.join(lines)[-6500:])
