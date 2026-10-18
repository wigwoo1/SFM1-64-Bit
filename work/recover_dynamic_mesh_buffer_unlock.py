from pathlib import Path
import sys,re,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
start,end=0x10006a90,0x10006ae1
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

assert im.word(0x100fbafc+7*4)==0x10006a90 and im.word(0x100fbac0+7*4)==0x10006ac0
report={'sha256':hashlib.sha256(im.b).hexdigest(),'checked_code_bytes':len(raw),'instructions':instructions,'vertex_unlock':'10006a90: primarymesh+56 nonzero calls10005a60(mesh+18,count); clearsbyte56 aftercallback. Descriptorargumentunused.','index_unlock':'10006ac0: secondarythis=root+4; byte53(root57) nonzero calls10005440(secondary+18=root1c,count), clearsbyte53aftercallback; descriptorunused.','limitations':'Complete wrapper bodies pluspadding checked. Lower buffer operations/nativeexecution/GPUintegration unfinished.'}
Path('outputs/sfm64-dynamic-mesh-buffer-unlock-recovery.json').write_text(json.dumps(report,indent=2))
print('checked',len(raw),'bytes',instructions,'instructions')
