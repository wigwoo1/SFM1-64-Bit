from pathlib import Path
from pe_image import Image
import re,json,struct,hashlib
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\networksystem.dll')
ranges=[(0x1000dab0,0x1000db64),(0x1000dc00,0x1000dc57),(0x1000dda6,0x1000ddb7),(0x1002e080,0x1002e09e)]
decoded=[]
for line in Path('work/networksystem-disassembly.txt').read_text().splitlines():
 m=re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if m:decoded.append([int(m[1],16),bytes.fromhex(m[2]),line.strip()])
 elif decoded:
  c=re.fullmatch(r'\s+((?:[0-9A-F]{2} ?)+)\s*',line)
  if c:decoded[-1][1]+=bytes.fromhex(c[1])
selected=[]
for va,b,line in decoded:
 if any(a<=va<z for a,z in ranges):
  o=im.offset(va);assert im.b[o:o+len(b)]==b;selected.append(dict(address=hex(va),bytes=b.hex(),instruction=line))
constants={}
for va,fmt,size in [(0x10030640,'<d',8),(0x10030368,'<f',4),(0x10030630,'<I',4)]:
 o=im.offset(va);constants[hex(va)]=dict(bytes=im.b[o:o+size].hex(),value=struct.unpack_from(fmt,im.b,o)[0])
report=dict(binary_sha256=hashlib.sha256(im.b).hexdigest(),convar=dict(parent='0x1004bd34',name=im.string(0x100317b4),default=im.string(0x1002ffd8),help=im.string(0x100317c0)),constants=constants,
 owner_link=[dict(slot=i,method=hex(im.word(0x10031504+4*i))) for i in range(44) if im.word(0x10031504+4*i)==0x1000dc00],
 verified_full_instructions=selected,limitations='Static update block and caller linkage; no live field lifecycle or update execution comparison.')
Path('outputs/sfm64-network-timing-update.json').write_text(json.dumps(report,indent=2))
print(json.dumps({k:v for k,v in report.items() if k!='verified_full_instructions'},indent=2));print('verified_full_instructions',len(selected))
