from pathlib import Path
import re,json,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll')
assert im.word(0x1009c9b4+3*4)==0x100375f0
regions=[(0x100375f0,0x10037651)]
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
report=dict(sha256=hashlib.sha256(im.b).hexdigest(),regions=results,secondary_vtable='0x1009c9b4',slot=3,flow=['Hardware configuration1013487c slot17 signed>=92 enables ShaderShadow global1011b0a4 slots21/20 with(15,1).','Selects entry using secondarythis+86 byte index and pointersecondary+80 toshaderstate whose+24 is44byte-entry table. Entry pointer retained across ShaderAPI78 capture callback.','Stores low16bit handle atentry+4+liveCount*2 and increments live32bit count.'],limitations='Static exact body checked. Logical four-handle adapter requires livecount<4; native unchecked capacity/otherentryfields/realcapture/public ABI/native execution/GPU unverified.')
Path('outputs/sfm64-shader-snapshot-capture-recovery.json').write_text(json.dumps(report,indent=2))
print('checked_snapshot_capture_bytes='+str(sum(r['bytes'] for r in results)))
