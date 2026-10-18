from pe_image import Image
from pathlib import Path
import struct,json,hashlib
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\networksystem.dll')
def va(o):
 for r,s,raw in im.sections:
  if raw<=o<raw+s:return im.base+r+o-raw
 raise ValueError(o)
def refs(p):
 out=[];start=0
 while True:
  o=im.b.find(struct.pack('<I',p),start)
  if o<0:return out
  start=o+1;out.append(va(o))
name=va(im.b.index(b'NetworkSystemVersion001\0'))
hits=refs(name);rows=[]
for p in hits:
 rows.append(dict(reference=hex(p),nearby=[hex(im.word(x)) for x in range(p-8,p+16,4)]))
report=dict(binary_sha256=hashlib.sha256(im.b).hexdigest(),interface_name=hex(name),references=rows)
Path('outputs/sfm64-networksystem-registration.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
