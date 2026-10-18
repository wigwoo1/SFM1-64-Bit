from pathlib import Path
import json,hashlib,struct
from pe_image import Image
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
report=json.loads(Path('outputs/sfm64-device-event-query-recovery.json').read_text());assert report['dll_sha256']==hashlib.sha256(image.b).hexdigest()
start,end=0x100296f0,0x100297bd;covered={}
for row in report['instructions']:
 address=int(row['address'],16)
 if start<=address<end:
  for i,value in enumerate(bytes.fromhex(row['bytes'])):covered[address+i]=value
raw=bytes(covered[address] for address in range(start,end));assert image.b[image.offset(start):image.offset(start)+len(raw)]==raw
assert struct.unpack_from('<I',raw,15)[0]==0x100fdc9c
for offset,target in [(5,0x100288d0),(51,0x100df3a0)]:assert raw[offset]==0xe8 and start+offset+5+struct.unpack_from('<i',raw,offset+1)[0]==target
Path('work/device_owner_constructor_native_init.h').write_text('#pragma once\nstatic const unsigned char nativeOwnerConstructorBytes[]={'+','.join(map(str,raw))+'};\nstatic const unsigned nativeOwnerConstructorOffset='+str(image.offset(start))+';\n')
print('device_owner_constructor_prepared_bytes='+str(len(raw)))
