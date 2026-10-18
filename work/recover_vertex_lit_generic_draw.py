from pathlib import Path
import re,json,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll')
assert im.word(0x1009c39c+14*4)==0x100537a0
regions=[(0x100537a0,0x10053a02)]+[(a,a+30) for a in [0x100856a0,0x100856c0,0x100850c0,0x100855a0,0x10085600,0x10085720]]
lines=Path('work/stdshader_dx9-disassembly.txt').read_text().splitlines()
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
registrations=[(0x100ca658,0x1008ddbc),(0x100cae80,0x1008cca0),(0x100caee0,0x1008ca54),(0x100cad48,0x10090c5c),(0x100caef8,0x10090cd0),(0x100caf28,0x1009b9b4)]
names={hex(index):im.string(name) for index,name in registrations}
report=dict(sha256=hashlib.sha256(im.b).hexdigest(),regions=results,parameter_indices=names,flow=['Primary draw routine calls main lighting helper1005a820 with info initialized by10052f10, or10065a70(0) if runtime overlay optimization100096f0 permits skipping.','Runtime/no shadow overlays use floating parameter gates; effect helpers and1001f590 require further recovery.','Each overlay branch reads enable parameter again after main helper; entire routine ends10053a02.'],limitations='Full concrete draw bytes and six parameter registrations checked. Complete effect helper semantics/native execution/public ABI/GPU remain unfinished.')
Path('outputs/sfm64-vertex-lit-generic-draw-recovery.json').write_text(json.dumps(report,indent=2))
print('checked_generic_draw_bytes='+str(sum(r['bytes'] for r in results)),names)
