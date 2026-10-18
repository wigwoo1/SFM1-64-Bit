from pathlib import Path
import re,json,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll')
assert im.word(0x10095d74+71*4)==0x10004ae0
assert im.word(0x10095a9c+71*4)==0x1000a910
regions=[(0x10004ae0,0x10004b5a),(0x1000a910,0x1000a921),(0x100052b0,0x100052d3)]
lines=Path('work/materialsystem-disassembly.txt').read_text().splitlines()
results=[]
for start,end in regions:
 covered={};address=None;instructions=0
 for line in lines:
  m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)',line)
  if m:
   address=int(m[1],16);raw=bytes.fromhex(m[2])
   if start<=address<end:instructions+=1
  elif address is not None and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):raw=bytes.fromhex(line)
  else:continue
  for b in raw:
   if start<=address<end:covered[address]=b
   address+=1
 assert len(covered)==end-start
 raw=bytes(covered[a] for a in range(start,end))
 assert raw==im.b[im.offset(start):im.offset(start)+len(raw)]
 results.append(dict(start=hex(start),end=hex(end),bytes=len(raw),instructions=instructions))
report=dict(sha256=hashlib.sha256(im.b).hexdigest(),regions=results,material_vtable='0x10095d74',queue_friendly_vtable='0x10095a9c',draw_slot=71,notes=['Queue-friendly slot71 tail-forwards through materialThis+4 to underlying material slot71.','CMaterial shader+14 null invokes missing-shader diagnostic; primary parameter atmaterial+24 null or entry0null treated as flags0, otherwise entry0+8 bit4 suppresses draw.','Calls shader slot0, then rereads live material fields and XOR-decodes word+78 with global101059f8; shader-system global101017f8 slot18 receives(shader,parameters,material+30,compression,decodedword,alphaLowByte!=0,mode).','Material slot83 ignores arguments and returns bit17 from parameter entry2 flags, or0 if absent.'],limitations='Static bytes checked. Decoded-word type and public ABI, live shader-system binding, original execution parity, shader callback semantics and GPU draw remain unverified.')
Path('outputs/sfm64-material-draw-recovery.json').write_text(json.dumps(report,indent=2))
print('checked_material_draw_bytes='+str(sum(r['bytes'] for r in results)))
