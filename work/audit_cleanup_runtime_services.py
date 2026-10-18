from pathlib import Path
import hashlib,json,re,struct
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tier0.dll')
o=im.u32(60)+24; d=im.base+im.u32(o+96)
n=im.base+im.word(d+32); a=im.base+im.word(d+36); f=im.base+im.word(d+28)
exports={}
for i in range(im.word(d+24)):
 name=im.string(im.base+im.word(n+4*i))
 if name in ['ThreadInMainThread','Plat_FloatTime']:
  exports[name]=hex(im.base+im.word(f+4*im.u16(im.offset(a+2*i))))
assert exports=={'Plat_FloatTime':'0x1000c320','ThreadInMainThread':'0x10010000'}
ranges=[(0x1000c2b0,0x1000c2fb),(0x1000c320,0x1000c384),(0x10010000,0x10010012),(0x10010020,0x1001002c),(0x1002dfff,0x1002e00b)]
rows=[];pending=None
for line in Path('work/tier0-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if m:
  pending=None; address=int(m[1],16)
  if any(s<=address<e for s,e in ranges):
   pending={'address':hex(address),'bytes':bytes.fromhex(m[2]).hex(),'instruction':m[3].strip()};rows.append(pending)
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line).hex()
covered=set()
for row in rows:
 address=int(row['address'],16);raw=bytes.fromhex(row['bytes']);offset=im.offset(address)
 assert im.b[offset:offset+len(raw)]==raw
 covered.update(range(address,address+len(raw)))
assert covered=={v for s,e in ranges for v in range(s,e)}
bindings={};descriptor=im.base+im.u32(o+104)
while im.word(descriptor+12):
 module=im.string(im.base+im.word(descriptor+12));names=im.word(descriptor) or im.word(descriptor+16);cells=im.word(descriptor+16);i=0
 while im.word(im.base+names+4*i):
  entry=im.word(im.base+names+4*i);iat=im.base+cells+4*i
  if iat in [0x1002f040,0x1002f054,0x1002f058]:
   assert not entry&0x80000000
   bindings[hex(iat)]={'module':module,'name':im.string(im.base+entry+2)}
  i+=1
 descriptor+=20
assert bindings['0x1002f040']['name']=='GetCurrentThreadId'
assert bindings['0x1002f054']['name']=='QueryPerformanceCounter'
assert bindings['0x1002f058']['name']=='QueryPerformanceFrequency'
step=struct.unpack_from('<d',im.b,im.offset(0x1003a720))[0]
report={'dll_path':str(im.path) if hasattr(im,'path') else 'F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/bin/tier0.dll','sha256':hashlib.sha256(im.b).hexdigest(),'exports':exports,'verified_bytes':len(covered),'instructions':rows,'imports':bindings,'timer':{'normal_path':'signed 64-bit counter delta from captured baseline divided by signed 64-bit performance frequency; elapsed seconds','initialization':'guard byte set before QueryPerformanceFrequency, followed by QueryPerformanceCounter baseline capture; API returns unchecked','alternate_path':'when byte100786fc nonzero, add file-backed step to global double10078700 on each invocation','alternate_step':step,'cleanup_threshold_seconds_normal_path':2.0},'main_thread':{'comparison':'GetCurrentThreadId == stored DWORD1007888c','setter':'10010020 stores current thread ID; export name and actual caller lifecycle unverified','startup_write':'verified fragment1002dfff..1002e00b stores current thread ID; full startup routine/caller context unverified'},'limits':'Static byte/import/export evidence only. No original DLL executed. x87 precision, timer alternate-mode lifecycle, initialization concurrency and thread ownership in the full application remain unverified.'}
Path('outputs/sfm64-cleanup-runtime-services-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_bytes='+str(len(covered))+' alternate_step='+repr(step))
print(json.dumps(bindings))
