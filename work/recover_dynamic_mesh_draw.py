from pathlib import Path
import sys,re,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
start,end=0x10009fd0,0x1000a1d8
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

assert im.b[im.offset(0x10006bfd):im.offset(0x10006bfd)+3]==bytes.fromhex('897748')
assert im.word(0x100fbafc+10*4)==0x10006be0
report={'sha256':hashlib.sha256(im.b).hexdigest(),'checked_code_bytes':len(raw),'instructions':instructions,'routine':hex(start),'notes':['Draw1000a1f0 forwards(0,first,count). Global10183b58slot23 permitsdraw orownslot24 handlesempty.','Primitivefield48 confirmed bysetter10006be0 storinginput afterservicepermission;0/10 fullrangecount usesvertices instead ofindices.','Submittedflag74 setbeforegeometrygates. Overrideflags75/76 alterbasevertex/baseindex; prepare10009b40 failure skipspublication.','Fullrange first-1/count0 resolvesbaseindex andoverrideindexcount/ownindices orvertices(primitive0/10). Partialrangeaddsbaseindex modulo32.','Publishes globals1017f734basevertex/1017f758vertexcount/1017f754pointertolocalfirst-count/1017f75c1; calls100093f0 withoriginalarg1 then clearsrangepointer.'],'limitations':'Staticbodychecked; callbacks/shaderglobals/nativeexecution/GPU submission unfinished.'}
Path('outputs/sfm64-dynamic-mesh-draw-recovery.json').write_text(json.dumps(report,indent=2))
print('checked',len(raw),'bytes',instructions,'instructions')
