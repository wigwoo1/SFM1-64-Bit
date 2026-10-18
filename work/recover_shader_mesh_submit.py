from pathlib import Path
import sys,re,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
start,end=0x10025b50,0x10025da2
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

assert im.word(0x100fcedc+253*4)==start
report={'sha256':hashlib.sha256(im.b).hexdigest(),'checked_code_bytes':len(raw),'instructions':instructions,'routine':hex(start),'interface_slot':253,'flow':[
'Hardware service10183b58 slot8 state byte+76 suppresses the entire submission before any state writes.',
'Publishes mesh/count/instance/metadata1/metadata2 at secondaryThis+11236..11252; clears flags11288/5532 and word5536.',
'Copies snapshot from6808..6836 to11256..11284, first word represented as byte(mode==1).',
'Nonnull mesh: slot1 format; slot32 signed count>0 enables service slot65 mode1/2 test, with repeated call for second comparison.',
'Material atsecondaryThis+11296 slot20 format bit100; mesh33 plus mode passed to material83; resulting byte passed to mesh30; service276 receives format words plus derived flags.',
'Instance alpha at72: unequal1 orunordered sets boolean. Calls completeobject helpers10016530/1001e950, conditional10013490 if service+5600 nonzero, then1001f820(0,0).',
'Material slot71 receives original compression argument, derived alpha boolean, derived mode boolean. Conditional10015230(snapshot,0) if callback-set flag11288.',
'Clears published mesh/count/instance/metadata1/metadata2 plus flags11288/5532; word5536 is not cleared again.'
], 'limitations':'Static checked recovery. Callback semantics, helper bodies, reentrancy, native execution and GPU submission remain unfinished.'}
Path('outputs/sfm64-shader-mesh-submit-recovery.json').write_text(json.dumps(report,indent=2))
print('checked',len(raw),'bytes',instructions,'instructions')
