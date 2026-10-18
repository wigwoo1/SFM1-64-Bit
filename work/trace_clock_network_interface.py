from pathlib import Path
from collections import deque
from pe_image import Image
import re,json,hashlib,struct
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
history=deque(maxlen=24);lines=[];refs=[];remaining=0
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  if '106941C4h' in line:
   refs.append(line.strip());lines.extend(history);remaining=20
  if remaining:lines.append(line);remaining-=1
  history.append(line)
checked=0
for line in set(lines):
 m=re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if m:
  b=bytes.fromhex(m[2]);o=im.offset(int(m[1],16));assert im.b[o:o+len(b)]==b;checked+=1
report=dict(binary_sha256=hashlib.sha256(im.b).hexdigest(),checked_unique_instruction_prefixes=checked,reference_count=len(refs),writers=[x for x in refs if re.search(r'mov\s+dword ptr ds:\[106941C4h\],',x)])
raw_refs=[];start=0
while True:
 o=im.b.find(struct.pack('<I',0x106941c4),start)
 if o<0:break
 start=o+1
 for r,s,raw in im.sections:
  if raw<=o<raw+s:raw_refs.append(hex(im.base+r+o-raw));break
report['raw_address_references']=raw_refs
try:report['initial_pointer']=hex(im.word(0x106941c4))
except ValueError:report['initial_pointer']='Unbacked runtime storage'
Path('outputs/sfm64-clock-network-interface-contexts.txt').write_text(''.join(lines))
Path('outputs/sfm64-clock-network-interface-audit.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
