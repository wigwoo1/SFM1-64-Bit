from pathlib import Path
from pe_image import Image
from collections import deque
import re,json,hashlib
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
history=deque(maxlen=18);lines=[];remaining=0;refs=[]
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  if any(x in line for x in ('104B60C0h','104B5898h','103FE6B4h')):
   refs.append(line.strip());lines.extend(history);remaining=20
  if remaining:lines.append(line);remaining-=1
  history.append(line)
checked=0
for line in set(lines):
 m=re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if m:
  b=bytes.fromhex(m[2]);o=im.offset(int(m[1],16));assert im.b[o:o+len(b)]==b;checked+=1
Path('outputs/sfm64-clock-predicate-interface-contexts.txt').write_text(''.join(lines))
object_address=im.word(0x103fe6b4)
default_object=dict(address=hex(object_address),note='Runtime-initialized object; vtable requires constructor proof')
report=dict(binary_sha256=hashlib.sha256(im.b).hexdigest(),checked_unique_instruction_prefixes=checked,reference_count=len(refs),writers=[x for x in refs if re.search(r'mov\s+dword ptr ds:\[104B60C0h\],',x)],references=refs,default_object=default_object)
Path('outputs/sfm64-clock-predicate-interface-audit.json').write_text(json.dumps(report,indent=2));print(json.dumps({k:v for k,v in report.items() if k!='references'},indent=2))
