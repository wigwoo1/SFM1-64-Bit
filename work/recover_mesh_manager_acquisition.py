from pathlib import Path
import sys,re,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
start,end=0x1000a750,0x1000a8e2
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

report={'sha256':hashlib.sha256(im.b).hexdigest(),'checked_code_bytes':len(raw),'instructions':instructions,'routine':'0x1000a750','default_mesh_offset':712,'default_mesh_vtable':'0x100fbafc','default_mesh_methods':{str(i):hex(im.word(0x100fbafc+4*i)) for i in [12,17,20,35,36]},'notes':['Overrides force buffered-byte0; otherwise use suppliedbufferedlowbyte. Changing away from oldbufferedstate withpreviousmesh triggersShaderglobal1016dd08slot84 andclearsmanager+2a8.','Shader service slot258 selectsmanager+418 versusdefault+2c8. Bufferedstate usesmanager+290 and cachesselectedunderlyingmesh+2a8, callingservice84 whenidentitychanges.','Withoutvertexoverride, querymaterialslot20 format. Implicitformat clearscompression400; explicitformat retained. Clearweightcountbits7000; maxsupplied/informatweightcount positive forces2080(two weights+indices).','Meshslot35 setsformat,slot36 bindsmaterial. Defaultmesh override vertex/index handles come fromoverrideslots39/40; writesflagsmesh+75/+76.'],'limitations':'Full acquisitioncode checked but serviceimplementations/embeddedconstructor binding/nativeexecution/GPU integration pending.'}
Path('outputs/sfm64-mesh-manager-acquisition-recovery.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
