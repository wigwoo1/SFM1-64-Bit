from pathlib import Path
import sys,re,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
start,end=0x100093f0,0x1000956c
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

report={'sha256':hashlib.sha256(im.b).hexdigest(),'checked_code_bytes':len(raw),'instructions':instructions,'routine':hex(start),'flow':['serviceglobal1016dd08slot274 writes stack76byteinstance region andtwo metadataoutputs.','meshslot37 writes two locals within instance region (offset44/48).','service260 returnsmaterial; material46 writesRGB and44 returnsalpha throughx87. Optional arg1 float4 copied thencomponentwiseSSEmultiplied bymaterialRGBA; resultinstanceoffset60..72.','mesh32 resultpassedtoservice277; mesh1 returnsformatlow/high; compressionbool derivedlowbit400.','service253 receives(mesh,1,instanceAddress,compression,metadata1,metadata2); functionreturns.'], 'limitations':'Static fullroutinechecked. Instance publicABI/metadata types/servicebinding/nativeexecution/fullGPUsubmission unfinished.'}
Path('outputs/sfm64-dynamic-mesh-submit-recovery.json').write_text(json.dumps(report,indent=2))
print('checked',len(raw),'bytes',instructions,'instructions')
