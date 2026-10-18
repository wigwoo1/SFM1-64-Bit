from pathlib import Path
import re,json,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll')
assert im.word(0x1009c39c+6*4)==0x100652c0
assert im.word(0x1009c39c+7*4)==0x100653c0
assert im.word(0x1009c39c+14*4)==0x100537a0
regions=[(0x100652c0,0x1006534b),(0x100653c0,0x1006549b),(0x10064ab0,0x10064b73)]
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
report=dict(sha256=hashlib.sha256(im.b).hexdigest(),regions=results,vertex_lit_generic_vtable='0x1009c39c',slots={'6':'0x100652c0','7':'0x100653c0','14':'0x100537a0'},flow=['Shadow setup10064ab0: calls shadow0 first, then reads primary parameter flags once; mask8000 calls slots3(0),2(0); mask10000 calls4(1),2(0); mask2000 calls15(0); mask400 calls1(1); mask10000000 calls14(2,1); mask20000000 calls24(1). Reloads live shadow pointer for each call.', 'Slot6 publishes params/shadow/api/full snapshot index/context-data pointer, clears pass counter, conditionally resets shadow, invokes virtual14 then clears all six globals.','Slot7 publishes currentAPI; shadow path reads parameter2 flags bits10/13 to output2/16; runtime path uses API17/19 lowbyte booleans.','API63 signed>=1 adds4; parameter flags bits9 AND11 adds8. Shadow bits20/21 add32/64; runtime API29(0) bits0/1 add32/64. Clears currentAPI on exit.'],limitations='Static bytes checked and logical x64 snapshot selector implemented; original execution oracle, generic draw14 integration, shared-global concurrency and GPU remain unverified.')
Path('outputs/sfm64-base-shader-draw-selection-recovery.json').write_text(json.dumps(report,indent=2))
print('checked_base_shader_bytes='+str(sum(r['bytes'] for r in results)))
