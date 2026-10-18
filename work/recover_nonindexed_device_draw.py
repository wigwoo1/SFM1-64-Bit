from pathlib import Path
import re,json,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
regions=[(0x1000bc0f,0x1000bc77)]
lines=Path('work/shaderapidx9-disassembly.txt').read_text().splitlines()
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
report=dict(sha256=hashlib.sha256(im.b).hexdigest(),regions=results,flow=['Primitive0/10 dispatch branch reads meshword4c type; firstvertex is published1017f734 andprimitivecount isrange secondword unchanged.','Queueenabled emits4word packet(opcode3,type,first,count), advancing16bytes andcalls10002df0. Queuecapacitycheck reserves4words.','Direct branch usesCOMslot81 withsame3argumentwords. HRESULT notconsumed.'],limitations='Static nativebodychecked. x64D3D9calladaptercompiles; noGPUexecution/queueconsumer/resourcebinding proof.')
Path('outputs/sfm64-nonindexed-device-draw-recovery.json').write_text(json.dumps(report,indent=2))
print('checked_nonindexed_draw_bytes='+str(sum(r['bytes'] for r in results)))
