from pathlib import Path
import re,json,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll')
assert im.word(0x1009c9b4+4*4)==0x10037660
regions=[(0x10037660,0x100376e1)]
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
report=dict(sha256=hashlib.sha256(im.b).hexdigest(),regions=results,secondary_vtable='0x1009c9b4',slot=4,flow=['Caches signed snapshot passcount before callbacks. Draw lowbyte nonzero calls ShaderAPI94(instanceword,livepass,count).','Always calls ShaderAPI202, then increments live8bit pass withwrap. If signed count exceeds newpass, reloads snapshot/table and calls ShaderAPI93(next16bit snapshot handle).'],limitations='Exact static bytes checked; logical adapter only, no original execution, instanceword type/public ABI/GPU verified.')
Path('outputs/sfm64-shader-runtime-pass-recovery.json').write_text(json.dumps(report,indent=2))
print('checked_runtime_pass_bytes='+str(sum(r['bytes'] for r in results)))
