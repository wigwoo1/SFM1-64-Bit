from pathlib import Path
import re,json,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
regions=[(0x10002ce0,0x10002d2d),(0x10002d30,0x10002d99),(0x10002da0,0x10002dec),(0x10002df0,0x10002e4b),(0x10002e50,0x10002eb1),(0x100f91c0,0x100f91cd)]
lines=Path('work/shaderapidx9-disassembly.txt').read_text().splitlines()
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
report=dict(sha256=hashlib.sha256(im.b).hexdigest(),regions=results,flow=['Allocator scans500 blockpointers at1017e040 forheader0, stores callerheader andreturnsblock; fullscan calls10002da0 andSleep(0), retries.','Publication10002d30 usesringobject1017e8b8: signed produced-consumed>=500 waitsSleep(0); storesblock atwriteIndex then incrementsproduced; writeIndex=(writeIndex+1)%500.','10002df0 onlypublisheswhen globalproducer-consumer counters1017f188/1017f290 equal; existing nonemptyblock gets terminator0/header2/published, then acquiresheader1 block, cursorblock+4, available4095words.','10002e50 queueenabled flush: publishesnonemptyblock similarly, acquiresnewblock, thenwaitsforglobalcountersequal. Nativecountersplainmemory/noexplicitatomicinstruction here.'],limitations='Fullpublicationbodies staticallychecked. Consumer unresolved; allocator10002da0 helper/native concurrency/memoryordering/resourceownership/GPU unfinished.')
report['flow'].append('Exhaustion helper10002da0 returns without mutation when existing cursor equals block+4. Otherwise a nonempty block receives terminator0/header2 and is published; absent or published block is replaced by allocator(header1), cursor=block+4, available=4095. Allocator can recursively enter this helper when every block is busy; actual liveness requires unresolved consumer.')
report['flow'].append('Startup100f91c0 initializes produced1017f188 and consumed1017f290 to zero. These addresses equal ring1017e8b8+8d0 and +9d8 respectively.')
assert 0x1017e8b8+0x8d0==0x1017f188
assert 0x1017e8b8+0x9d8==0x1017f290
report['limitations']='Publication, exhaustion helper and counter initializer statically checked against installed bytes. Consumer unresolved; native concurrency, memory ordering, resource ownership and GPU execution unfinished.'
Path('outputs/sfm64-device-command-publication-recovery.json').write_text(json.dumps(report,indent=2))
print('checked_publication_bytes='+str(sum(r['bytes'] for r in results)))
