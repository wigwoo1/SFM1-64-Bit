from pathlib import Path
from pe_image import Image
import re,json,hashlib,struct
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');selected=[];pending=None
def save(item):
 if item is None:return
 address,data,line=item
 if not 0x10240630<=address<0x10240950:return
 offset=image.offset(address);assert image.b[offset:offset+len(data)]==data
 selected.append(dict(address=hex(address),bytes=data.hex(),instruction=line))
with Path('work/ifm-disassembly.txt').open(encoding='utf-8') as stream:
 for line in stream:
  match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if match:
   save(pending);pending=[int(match[1],16),bytes.fromhex(match[2]),line.strip()]
   if pending[0]>=0x10240950:break
  elif pending:
   tail=re.fullmatch(r'\s+((?:[0-9A-F]{2} ?)+)\s*',line)
   if tail:pending[1]+=bytes.fromhex(tail[1])
constants=[]
for address,kind in [(0x10A8A56C,'f'),(0x10ADB8A0,'d'),(0x10ADB89C,'f')]:
 offset=image.offset(address);constants.append(dict(address=hex(address),bytes=image.b[offset:offset+struct.calcsize(kind)].hex(),value=struct.unpack_from('<'+kind,image.b,offset)[0]))
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),routine='0x10240630',history_pointer='renderable_this+0x18',controller_pointer='renderable_this+0xa8',controller_count='renderable_this+0xb4',coefficient_time_callback='global 10FD4398, method byte offset C0h',constants=constants,verified_complete_instructions=selected,limitations='Static function evidence; time callback identity, history initialization, live dispatch and x87 coefficient precision still unverified. Bounded extraction may include following padding.')
Path('outputs/sfm64-ifm-puppet-weight-recovery.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('Verified puppet weight instructions:',len(selected),'constants:',constants)
