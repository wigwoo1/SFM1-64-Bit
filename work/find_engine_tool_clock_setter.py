from pe_image import Image
from pathlib import Path
import struct,re,json
root=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
def refs(im,va):
 result=[];start=0
 while True:
  o=im.b.find(struct.pack('<I',va),start)
  if o<0:return result
  start=o+1
  for r,s,raw in im.sections:
   if raw<=o<raw+s:result.append(im.base+r+o-raw);break
engine=Image(root/'bin/engine.dll');print('setter vtable refs',[hex(x) for x in refs(engine,0x101e57b0)])
im=Image(root/'bin/tools/ifm.dll');hits=refs(im,0x10b42bec);print('tool interface name refs',[hex(x) for x in hits]);lines=[]
with Path('work/ifm-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}):',line)
  if m and any(x-40<=int(m[1],16)<=x+64 for x in hits):lines.append(line)
Path('outputs/sfm64-ifm-engine-tool-interface-connect.txt').write_text(''.join(lines));print(''.join(lines))
Path('outputs/sfm64-engine-tool-clock-setter-reference-audit.json').write_text(json.dumps(dict(setter='0x101e57b0',engine_refs=[hex(x) for x in refs(engine,0x101e57b0)],ifm_interface_string='0x10b42bec',ifm_refs=[hex(x) for x in hits]),indent=2))
