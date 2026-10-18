from pathlib import Path
import hashlib,json,re,struct
from pe_image import Image
image=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
ranges=[(0x10028a80,0x10028aa1),(0x10028bf0,0x10028c1c),(0x10028c40,0x10028c9d),(0x1002ef20,0x1002efb0),(0x10028ab0,0x10028b12),(0x10028bd0,0x10028be3)];rows=[];pending=None
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 match=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if match:
  pending=None;address=int(match[1],16)
  if any(start<=address<end for start,end in ranges):pending={'address':hex(address),'bytes':bytes.fromhex(match[2]).hex(),'instruction':match[3].strip()};rows.append(pending)
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line).hex()
covered=set()
for row in rows:
 address=int(row['address'],16);raw=bytes.fromhex(row['bytes']);offset=image.offset(address);assert image.b[offset:offset+len(raw)]==raw;covered.update(range(address,address+len(raw)))
assert covered=={address for start,end in ranges for address in range(start,end)}
raw=image.b[image.offset(0x10028a80):image.offset(0x10028a80)+33]
Path('work/manager_cleanup_native_init.h').write_text('#pragma once\nstatic const unsigned char cleanupBytes[]={'+','.join(map(str,raw))+'};\nstatic const unsigned cleanupOffset='+str(image.offset(0x10028a80))+';\n')
vtable=[{'slot':slot,'target':hex(image.word(0x100fdb8c+slot*4))} for slot in range(24)]
registration_slots=[row['slot'] for row in vtable if row['target']=='0x10028c40']
assert registration_slots==[16]
Path('outputs/sfm64-manager-cleanup-recovery.json').write_text(json.dumps({'sha256':hashlib.sha256(image.b).hexdigest(),'verified_bytes':len(covered),'instructions':rows,'base_vtable_prefix_words_bounds_unverified':vtable,'registration_slots':registration_slots,'flow':'Capture signed count owner+24 once; if positive, reload callback-array pointer owner+18 each iteration and invoke no-argument callback.','constructor':'10028bf0 installs100fdb8c, clears eleven DWORDs owner+4 through+2c, including registry pointer/count.','registration':'10028c40 appends incoming callback to owner+18 vector: captured index owner+24, capacity owner+1c, growth1002ef20 on vector when count+1 exceeds capacity; increments count, updates vector cached base owner+28, stores callback at base+index*4.','limits':'Constructor/registration recovered statically only; vector growth, callback registration callers/identities, allocator/owner lifetime and concurrency remain unresolved. Dispatcher has separate controlled execution evidence.','native_executed':False},indent=2)+'\n')
print('manager_cleanup_verified_bytes='+str(len(covered)))
growth=image.b[image.offset(0x1002ef20):image.offset(0x1002ef20)+144]
assert growth[96:98]==bytes.fromhex('8b0d') and struct.unpack_from('<I',growth,98)[0]==0x100fa238
Path('work/manager_growth_native_init.h').write_text('#pragma once\nstatic const unsigned char growthBytes[]={'+','.join(map(str,growth))+'};\nstatic const unsigned growthOffset='+str(image.offset(0x1002ef20))+';\n')
append=image.b[image.offset(0x10028c40):image.offset(0x10028c40)+93]
for offset,target in [(29,0x1002ef20),(65,0x100d73c0)]:
 assert append[offset]==0xe8 and 0x10028c40+offset+5+struct.unpack_from('<i',append,offset+1)[0]==target
Path('work/manager_append_native_init.h').write_text('#pragma once\nstatic const unsigned char appendBytes[]={'+','.join(map(str,append))+'};\nstatic const unsigned appendOffset='+str(image.offset(0x10028c40))+';\n')



remove=image.b[image.offset(0x10028ab0):image.offset(0x10028ab0)+98]
wrapper=image.b[image.offset(0x10028bd0):image.offset(0x10028bd0)+19]
assert remove[78]==0xe8 and 0x10028ab0+83+struct.unpack_from('<i',remove,79)[0]==0x100d73c0
assert wrapper[10]==0xe8 and 0x10028bd0+15+struct.unpack_from('<i',wrapper,11)[0]==0x10028ab0
Path('work/manager_remove_native_init.h').write_text('#pragma once\nstatic const unsigned char removeBytes[]={'+','.join(map(str,remove))+'};\nstatic const unsigned removeOffset='+str(image.offset(0x10028ab0))+';\nstatic const unsigned char removeWrapperBytes[]={'+','.join(map(str,wrapper))+'};\nstatic const unsigned removeWrapperOffset='+str(image.offset(0x10028bd0))+';\n')
