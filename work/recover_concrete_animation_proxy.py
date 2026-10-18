from pathlib import Path
from pe_image import Image
import struct,json,re,hashlib,bisect
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\bin\client.dll')
vtable=0x104c02e4;slots=[im.word(vtable+i*4) for i in range(7)];refs=[];needle=struct.pack('<I',0x1063ce48);start=0
while True:
 o=im.b.find(needle,start)
 if o<0:break
 start=o+1
 for r,s,raw in im.sections:
  if raw<=o<raw+s:refs.append(im.base+r+o-raw);break
report=dict(binary_sha256=hashlib.sha256(im.b).hexdigest(),registration='0x104878c0',registered_name=im.string(0x104c0300),factory='0x1000c6d0',factory_object_size=20,concrete_vtable=hex(vtable),slots=[hex(x) for x in slots],start_time_slot5=hex(slots[5]),clock_pointer_xrefs=[hex(x) for x in refs])
refs.sort();lines=[]
with Path('work/native-client-scale-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}):',line)
  if m:
   va=int(m[1],16)
   if slots[6]<=va<slots[6]+32 or 0x1000c700<=va<0x1000c705 or (bisect.bisect_left(refs,va-16)<len(refs) and refs[bisect.bisect_left(refs,va-16)]<=va+12):lines.append(line)
Path('outputs/sfm64-concrete-animation-proxy-recovery.json').write_text(json.dumps(report,indent=2));Path('outputs/sfm64-animation-clock-pointer-xrefs.txt').write_text(''.join(lines));print(json.dumps(report,indent=2));print(''.join(lines)[:5000])
