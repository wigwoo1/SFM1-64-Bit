from pathlib import Path
import hashlib,json,re
from pe_image import Image
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
start,end=0x1002a610,0x1002a667;constructorStart,constructorEnd=0x100296f0,0x100297bd;rows=[];pending=None
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 match=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if match:
  pending=None;address=int(match[1],16)
  if start<=address<end or constructorStart<=address<constructorEnd:pending={'address':hex(address),'bytes':bytes.fromhex(match[2]).hex(),'instruction':match[3].strip()};rows.append(pending)
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line).hex()
covered={}
for row in rows:
 address=int(row['address'],16);raw=bytes.fromhex(row['bytes']);offset=image.offset(address)
 assert image.b[offset:offset+len(raw)]==raw
 for i,value in enumerate(raw):covered[address+i]=value
assert set(covered)==set(range(start,end))|set(range(constructorStart,constructorEnd))
raw=bytes(covered[address] for address in range(start,end))
Path('work/device_event_query_native_init.h').write_text('#pragma once\nstatic const unsigned char nativeEventQueryBytes[]={'+','.join(map(str,raw))+'};\nstatic const unsigned nativeEventQueryOffset='+str(image.offset(start))+';\n')
report={'dll_sha256':hashlib.sha256(image.b).hexdigest(),'start':hex(start),'end':hex(end),'verified_bytes':len(raw),'instructions':rows,'flow':'If owner+6c differs fromffffffff, do nothing. Otherwise call device COM slot118 with type8 and pointer to initially-null local. Nonnegative HRESULT plus nonnull output calls output slot2 Release, then sets owner+6c=1. Failure or null sets owner+6c=0.','identity':'D3D9 CreateQuery type8 event query inferred from interface provenance, COM slot and documented signature','queue_consumer_identified':False,'native_executed':False,'limits':'Full selected helper bytes verified; COM callback execution, cache initialization/invalidation and queue activation remain unverified.'}
assert image.b[image.offset(0x10029769):image.offset(0x10029769)+6]==bytes.fromhex('83c8ff89466c')
assert image.word(0x100296ff)==0x100fdc9c
report['cache_constructor']={'start':hex(constructorStart),'end':hex(constructorEnd),'verified_bytes':constructorEnd-constructorStart,'vtable':'100fdc9c installed100296fd','initialization':'10029769 EAX=-1;1002976c stores owner+6c','base_constructor':'100288d0 called before initialization','limits':'Static constructor bytes only; base construction, runtime owner identity and cache invalidation/reset behavior unverified. Other objects also have +6c fields.'}
Path('outputs/sfm64-device-event-query-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('device_event_query_verified_bytes='+str(len(raw)))
