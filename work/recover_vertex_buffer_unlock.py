from pathlib import Path
import sys,re,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
start,end=0x10005a60,0x10005b86
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

report={'sha256':hashlib.sha256(im.b).hexdigest(),'checked_code_bytes':len(raw),'instructions':instructions,'routine':hex(start),'notes':'Flagbyte20bit1/nonnullresource gateall. Cursorbytes8 rounds upward by signeddivide(stride16); bytecount=count*stride modulo32. Flagbit0 choosescommand29extended(resource,metadata36..44,bytes) vs18simple(resource). DirectCOMslot12 negative reports3f4/3da. Finalcursor=roundedoldcursor+bytecount andclearsbit1; nullresource retainsstate.','limitations':'Staticcodechecked; commandcapacity/helper/COM runtime notverified.'}
Path('outputs/sfm64-vertex-buffer-unlock-recovery.json').write_text(json.dumps(report,indent=2))
print('checked',len(raw),'bytes',instructions,'instructions')
