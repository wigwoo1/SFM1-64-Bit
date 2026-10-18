from pathlib import Path
from pe_image import Image
import struct,json,re,bisect
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll');o=im.b.index(b'VClient016\0');va=next(im.base+r+o-raw for r,s,raw in im.sections if raw<=o<raw+s);hits=[];start=0
while True:
 ref=im.b.find(struct.pack('<I',va),start)
 if ref<0:break
 start=ref+1
 for r,s,raw in im.sections:
  if raw<=ref<raw+s:hits.append(im.base+r+ref-raw);break
hits.sort();print('interface string',hex(va),'references',[hex(x) for x in hits]);lines=[]
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}):',line)
  if m:
   address=int(m[1],16);n=bisect.bisect_left(hits,address-160)
   if n<len(hits) and hits[n]<=address+64:lines.append(line)
Path('outputs/sfm64-engine-client-clock-interface-disassembly.txt').write_text(''.join(lines));print(''.join(lines))
