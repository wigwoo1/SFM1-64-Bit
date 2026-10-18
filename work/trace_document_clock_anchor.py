from pathlib import Path
from pe_image import Image
from collections import deque
import re,json,hashlib,struct
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
history=deque(maxlen=14);lines=[];refs=[];remaining=0
with Path('work/ifm-disassembly.txt').open() as f:
 for line in f:
  if '+00000430h]' in line:
   refs.append(line.strip());lines.extend(history);remaining=15
  if remaining:lines.append(line);remaining-=1
  history.append(line)
checked=0
for line in set(lines):
 m=re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if m:
  b=bytes.fromhex(m[2]);o=im.offset(int(m[1],16));assert im.b[o:o+len(b)]==b;checked+=1
method_refs=[];start=0
while True:
 o=im.b.find(struct.pack('<I',0x1029a950),start)
 if o<0:break
 start=o+1
 for r,s,raw in im.sections:
  if raw<=o<raw+s:method_refs.append(hex(im.base+r+o-raw));break
report=dict(binary_sha256=hashlib.sha256(im.b).hexdigest(),checked_unique_instruction_prefixes=checked,references=refs,getter_raw_address_references=method_refs,limitations='Offset matches need owner proof; register/aliased anchor stores can evade exact-offset search.')
table=0x10ae7158;col=im.word(table-4)
report['constructor_owner']=dict(vtable=hex(table),type=im.string(im.word(col+12)+8),offset=im.word(col+4),store_instruction='0x102b9bc9',anchor_store='0x102b9e1c')
ranges=[(0x102b9b90,0x102b9e23),(0x102adc6f,0x102adc99),(0x102b6433,0x102b645d),(0x102b5d36,0x102b5d6c),(0x102b6073,0x102b609c)]
decoded=[];pending=False
with Path('work/ifm-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if m:
   va=int(m[1],16)
   if va>=max(z for a,z in ranges):break
   pending=any(a<=va<z for a,z in ranges)
   if pending:decoded.append([va,bytes.fromhex(m[2]),line.strip()])
  elif pending:
   c=re.fullmatch(r'\s+((?:[0-9A-F]{2} ?)+)\s*',line)
   if c:decoded[-1][1]+=bytes.fromhex(c[1])
full=[]
for va,b,line in decoded:
 o=im.offset(va);assert im.b[o:o+len(b)]==b
 full.append(dict(address=hex(va),bytes=b.hex(),instruction=line))
report['checked_complete_lifecycle_instructions']=len(full)
Path('outputs/sfm64-document-clock-lifecycle-instructions.json').write_text(json.dumps(full,indent=2))
Path('outputs/sfm64-document-clock-anchor-contexts.txt').write_text(''.join(lines))
Path('outputs/sfm64-document-clock-anchor-audit.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
