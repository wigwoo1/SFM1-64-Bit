from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll')
labels=[]
for match in re.finditer(rb'[\x20-\x7e]{5,}\x00',image.b):
 text=match[0][:-1].decode('ascii')
 if not re.search(r'flex|skin|software|morph',text,re.I):continue
 offset=match.start();address=None
 for rva,size,raw in image.sections:
  if raw<=offset<raw+size:address=image.base+rva+offset-raw;break
 if address is not None:labels.append(dict(address=hex(address),text=text,references=[]))
by_address={int(item['address'],16):item for item in labels}
for line in Path('work/studiorender-disassembly.txt').read_text(encoding='utf-8').splitlines():
 match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if not match:continue
 for operand in re.findall(r'\b(1[0-9A-F]{7})h\b',line):
  target=int(operand,16)
  if target not in by_address:continue
  address=int(match[1],16);prefix=bytes.fromhex(match[2]);offset=image.offset(address)
  assert image.b[offset:offset+len(prefix)]==prefix
  by_address[target]['references'].append(line.strip())
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),labels=labels,scope='ASCII label scan and verified disassembler instruction-prefix references; labels alone do not establish function behavior; continuation bytes and runtime paths unverified')
Path('outputs/sfm64-native-software-flex-labels.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
for label in labels:
 if label['references']:print(label['address'],label['text'],len(label['references']))
