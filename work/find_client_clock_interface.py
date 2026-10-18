from pathlib import Path
from pe_image import Image
import struct,json
root=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game');im=Image(root/'tf/bin/client.dll')
def refs(im,value):
 hits=[];needle=struct.pack('<I',value);start=0
 while True:
  o=im.b.find(needle,start)
  if o<0:return hits
  start=o+1
  for r,s,raw in im.sections:
   if raw<=o<raw+s:hits.append(im.base+r+o-raw);break
hits=refs(im,0x10099860);print('init refs',[hex(x) for x in hits])
for hit in hits:
 print(hex(hit),[hex(im.word(hit+i*4)) for i in range(-3,10)])
for rel in ['tf/bin/client.dll','bin/engine.dll','bin/tools/ifm.dll']:
 p=root/rel;data=p.read_bytes();print(rel)
 import re
 print([x.decode('ascii') for x in re.findall(rb'VClient[0-9]{3}',data)])
Path('outputs/sfm64-client-clock-init-reference-audit.json').write_text(json.dumps(dict(init='0x10099860',references=[hex(x) for x in hits]),indent=2))
