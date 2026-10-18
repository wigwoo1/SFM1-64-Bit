from pathlib import Path
import sys,re,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
start,end=0x1000ffb0,0x100100a0
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

assert im.word(0x1016dd08)==0x10180730
slots={i:im.word(0x100fcedc+i*4) for i in [253,258,260,274,277]}
assert slots=={253:0x10025b50,258:0x10014090,260:0x10013fd0,274:0x1000ffb0,277:0x1000ea60}
report={'sha256':hashlib.sha256(im.b).hexdigest(),'checked_code_bytes':len(raw),'instructions':instructions,'service_global':'0x1016dd08','service_object':'0x10180730','service_vtable':'0x100fcedc','slots':{str(i):hex(a) for i,a in slots.items()},'instance_getter':'0x1000ffb0','instance_bytes':76,'notes':['Zeros76bytes theninstance+24=serviceThis+5084.','Ifservice+3552 nonzero andservice+5900 nonzero: instance+8=count+1, +16=service+1008; clearsservicecount3552. Otherwisequeriesmatrixservice+1004slot17 andhelper10057cba, copies48bytes into service+1008; instance+8=1/+16=thatmatrix.','Metadata1=serviceThis+4536; metadata2=serviceThis+4534 (two bytes earlier).','Pointers andoffsets relative ShaderAPI secondaryinterfaceThis, not completeobject.'],'limitations':'Staticbody/bindingchecked; matrixhelper/nativepublicx64instanceABI/nativeoracle/rendererGPUunfinished.'}
Path('outputs/sfm64-shader-instance-state-recovery.json').write_text(json.dumps(report,indent=2))
print('checked',len(raw),'bytes',instructions,'instructions; instance76bytes')
