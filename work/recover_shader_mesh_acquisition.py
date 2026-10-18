from pathlib import Path
import sys,re,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
start,end=0x1000ea70,0x1000eae1
covered={};address=None;instructions=0
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)',line)
 if m:
  address=int(m[1],16)
  if start<=address<end:instructions+=1
  raw=bytes.fromhex(m[2])
 elif address is not None and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):raw=bytes.fromhex(line)
 else:continue
 if start<=address<end:
  for b in raw:
   if address<end:covered[address]=b
   address+=1
 else:address+=len(raw)
assert len(covered)==end-start,(len(covered),end-start)
raw=bytes(covered[a] for a in range(start,end))
assert raw==im.b[im.offset(start):im.offset(start)+len(raw)]

assert [im.word(0x100fcedc+4*i) for i in [85,86,180]]==[0x1000ea80,0x1000eab0,0x1000ea70]
report={'sha256':hashlib.sha256(im.b).hexdigest(),'checked_code_bytes':len(raw),'instructions':instructions,'shader_slots':{'85':'0x1000ea80','86':'0x1000eab0','180':'0x1000ea70'},'mesh_manager_slots':{'9':hex(im.word(0x100fbce4+9*4)),'28':hex(im.word(0x100fbce4+28*4))},'arguments':'Shader85 inserts two zero format words aftercount before forwarding allsevenargs to mesh-manager slot9. Shader86 forwardssevenargs unchanged. Shader180 tailcalls mesh-manager slot28. All obtainstaticmeshmanager via10005bc0.','limitations':'Static checked binding only; mesh-manager implementations/runtime/GPU unverified.'}
Path('outputs/sfm64-shader-mesh-acquisition-recovery.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
