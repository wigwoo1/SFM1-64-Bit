from pathlib import Path
import re,json,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll')
regions=[(0x10065a70,0x10065b4e)]
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
interfaces={hex(a):im.string(a) for a in [0x1009e3c4,0x1009eabc,0x1009eaac]}
report=dict(sha256=hashlib.sha256(im.b).hexdigest(),regions=results,interfaces=interfaces,flow=['Shadow path checks configuration byte66 and primaryparameter flagmask2; if permitted calls shadow2(1), shadow8(0).','Calls service100db9e0 slot3 before checking current pass instance pointer. Missing instance plus secondaryparameter flagmask40000 or2 sets globals100db960 and100bd148 then calls10065980.','Increments live pass counter after callbacks. Shadow path ignores supplied draw argument.','Runtime path reads current pass instanceword4, defaults0 for absentinstance; service slot4(instanceword4,suppliedDrawArgument); increments live counter after callback.'],limitations='Static checked body. Instanceword4 type, service binding, shadow instance creation and original execution/GPU remain unfinished.')
Path('outputs/sfm64-base-shader-pass-recovery.json').write_text(json.dumps(report,indent=2))
print('checked_shader_pass_bytes='+str(sum(r['bytes'] for r in results)),interfaces)
