from pathlib import Path
import struct,json,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
start,end=0x1000fc90,0x1000fe10
raw=im.b[im.offset(start):im.offset(start)+end-start]
lookup=im.b[im.offset(0x100d4ef0):im.offset(0x100d4ef0)+71]
optional=im.u32(60)+24;rva,size=struct.unpack_from('<II',im.b,optional+96+40)
cursor=im.offset(im.base+rva);limit=cursor+size;fixups=[]
while cursor<limit:
 page,length=struct.unpack_from('<II',im.b,cursor)
 if not length:break
 for offset in range(cursor+8,cursor+length,2):
  entry=im.u16(offset);address=im.base+page+(entry&4095)
  if entry>>12==3 and start<=address<end:fixups.append((address-start,im.word(address)))
 cursor+=length
allowed={0x1016d130,0x100fc764,0x100fa5f8,0x100fc760}
assert {target for _,target in fixups}==allowed
calls=[]
for address in (0x1000fd31,0x1000fd43,0x1000fd55):
 offset=address-start;assert raw[offset]==0xe8
 assert address+5+struct.unpack_from('<i',raw,offset+1)[0]==0x100d4ef0
 calls.append(offset)
header='#pragma once\nstatic constexpr unsigned char nativeColorConversionBytes[]={'+','.join(map(str,raw))+'};\n'
header+='static constexpr unsigned nativeColorConversionFileOffset='+str(im.offset(start))+';\n'
header+='static constexpr unsigned nativeColorConversionFixups[][2]={'+','.join('{'+str(a)+','+str(t)+'}' for a,t in fixups)+'};\n'
header+='static constexpr unsigned nativeColorConversionCalls[]={'+','.join(map(str,calls))+'};\n'
Path('work/color_conversion_native_init.h').write_text(header)
report={'installed_sha256':hashlib.sha256(im.b).hexdigest(),'body_bytes':len(raw),'absolute_fixups':[{'offset':a,'original_target':hex(t)} for a,t in fixups],'lookup_calls':calls,'limits':'Preparation only. Indirect service calls must point to a controlled fake object/vtable; original lookup must be relocated to the native-generated table. Native full-conversion execution not yet performed.'}
Path('outputs/sfm64-color-conversion-native-preparation.json').write_text(json.dumps(report,indent=2)+'\n')
print('conversion_bytes='+str(len(raw))+' absolute_fixups='+str(len(fixups))+' lookup_calls='+str(len(calls)))
