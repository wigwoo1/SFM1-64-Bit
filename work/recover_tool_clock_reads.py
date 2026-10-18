from pathlib import Path
from pe_image import Image
from collections import deque
import re,json,hashlib,struct
engine=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
slots={i:engine.word(0x103666c4+4*i) for i in range(44,51)}
history=deque(maxlen=10);lines=[];remaining=0;refs=[]
with Path('work/ifm-disassembly.txt').open() as f:
 for line in f:
  if '10FD4398h' in line:
   refs.append(line.strip());lines.extend(history);remaining=20
  if remaining:lines.append(line);remaining-=1
  history.append(line)
checked=0
for line in set(lines):
 m=re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if m:
  b=bytes.fromhex(m[2]);o=im.offset(int(m[1],16));assert im.b[o:o+len(b)]==b;checked+=1
Path('outputs/sfm64-ifm-tool-clock-reference-contexts.txt').write_text(''.join(lines))
selected=[]
for source,image,ranges in [('work/engine-disassembly.txt',engine,[(0x101e5750,0x101e57c4)]),('work/ifm-disassembly.txt',im,[(0x100801c0,0x10080221),(0x102b0680,0x102b079b),(0x10335830,0x10335834),(0x1075f0f0,0x1075f11f),(0x10002830,0x100028b5),(0x10275640,0x10275693),(0x1028bee0,0x1028bf31),(0x10298ad0,0x10298b31),(0x1029a950,0x1029aa1d),(0x102b4e7e,0x102b4edc),(0x1097b020,0x1097b087),(0x1097b170,0x1097b1e0),(0x1097b900,0x1097b917)])]:
 decoded=[];pending=False
 with Path(source).open() as f:
  for line in f:
   m=re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
   if m:
    va=int(m[1],16)
    if va>=max(z for a,z in ranges):break
    pending=any(a<=va<z for a,z in ranges)
    if pending:decoded.append([va,bytes.fromhex(m[2]),line.strip()])
   elif pending and decoded:
    c=re.fullmatch(r'\s+((?:[0-9A-F]{2} ?)+)\s*',line)
    if c:decoded[-1][1]+=bytes.fromhex(c[1])
 for va,b,line in decoded:
  o=image.offset(va);assert image.b[o:o+len(b)]==b;selected.append(dict(source=source,address=hex(va),bytes=b.hex(),instruction=line))
report=dict(engine_sha256=hashlib.sha256(engine.b).hexdigest(),ifm_sha256=hashlib.sha256(im.b).hexdigest(),slots={str(i):hex(p) for i,p in slots.items()},checked_unique_instruction_prefixes=checked,reference_count=len(refs),limitations='Global interface contexts only; classify dispatch receiver and slot before assigning clock semantics.')
report['fallback_attribute']=im.string(0x10acfb18)
report['fallback_default_float']=struct.unpack('<f',struct.pack('<I',im.word(0x10ab2848)))[0]
report['fallback_settings_path']=[im.string(p) for p in (0x10a8a6c4,0x10ae44a8)]
report['fallback_owner_handle_offset']='0x80'
Path('outputs/sfm64-tool-clock-read-audit.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
Path('outputs/sfm64-tool-clock-read-methods.json').write_text(json.dumps(selected,indent=2))
print('verified_complete_instructions',len(selected))


