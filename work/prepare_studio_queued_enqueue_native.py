from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll');source=json.loads(Path('outputs/sfm64-studio-model-release-dispatch.json').read_text());assert hashlib.sha256(im.b).hexdigest()==source['sha256']
a,z=0x100572e0,0x1005733a;covered=set();fixes=[]
for r in source['instructions']:
 va=int(r['address'],16)
 if a<=va<z:
  b=bytes.fromhex(r['bytes']);assert im.b[im.offset(va):im.offset(va)+len(b)]==b;covered.update(range(va,va+len(b)));assert 'ds:[' not in r['instruction']
  if b[0]==0xe8:assert va+5+int.from_bytes(b[1:],'little',signed=True)==0x10001860;fixes.append(va-a+1)
assert covered==set(range(a,z)) and len(fixes)==1;code=im.b[im.offset(a):im.offset(a)+z-a];at=fixes[0]
header=['#pragma once','inline constexpr unsigned char studioQueuedEnqueueNativeBytes[]={'+','.join(hex(x) for x in code)+'};','inline void initializeStudioQueuedEnqueueNative(unsigned char* code,uint32_t allocate){','std::memcpy(code,studioQueuedEnqueueNativeBytes,sizeof(studioQueuedEnqueueNativeBytes));',f'uint32_t displacement=allocate-reinterpret_cast<uint32_t>(code+{at+4});std::memcpy(code+{at},&displacement,4);','}']
Path('work/studio_queued_enqueue_native_init.h').write_text('\n'.join(header)+'\n');print('prepared90 enqueue bytes')
