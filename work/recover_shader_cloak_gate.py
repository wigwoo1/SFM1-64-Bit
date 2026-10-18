from pathlib import Path
import re,json,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll')
regions=[(0x100096f0,0x100097c1)]
assert im.word(0x1008cf44)==0x3f800000
assert im.word(0x1008cff0)==0x3faccccd
assert im.word(0x1008cfec)==0x3ecccccd
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
report=dict(sha256=hashlib.sha256(im.b).hexdigest(),regions=results,constants={'one':'0x3f800000','scale':'0x3faccccd','threshold':'0x3ecccccd'},flow=['Negative factor index defaults0; otherwise parameter slot17 lowbyte defined boolean; true rereads live index thenfloat atparameter+12.','Clamps factor0..1, scalar SSE computes1-1.35*clampedFactor. Ordered greater1 returnsfalse, ordered less0 returnstrue, otherwise threshold>=remaining returnstrue. Unordered final compare returnsfalse.','Concrete shader100537a0 invokes only ifcloakpass enabled andshadownull; true skipsmainlighting via10065a70(0).'],limitations='Static bytes/constantschecked; logical x64 boolean candidate under masked FP exceptions. Original execution parity, exact exception behavior, material/GPU integration unfinished.')
Path('outputs/sfm64-shader-cloak-gate-recovery.json').write_text(json.dumps(report,indent=2))
print('checked_cloak_gate_bytes='+str(sum(r['bytes'] for r in results)))
