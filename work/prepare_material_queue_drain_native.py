from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll');source=json.loads(Path('outputs/sfm64-material-call-queue-recovery.json').read_text());assert hashlib.sha256(im.b).hexdigest()==source['materialsystem_sha256']
a,z=0x100219a0,0x100219dd;covered=set();fixes=[]
for r in source['instructions']:
 va=int(r['address'],16)
 if a<=va<z:
  b=bytes.fromhex(r['bytes']);assert im.b[im.offset(va):im.offset(va)+len(b)]==b;covered.update(range(va,va+len(b)));assert 'ds:[' not in r['instruction']
  if b[0]==0xe8:assert va+5+int.from_bytes(b[1:],'little',signed=True)==0x10073980;fixes.append(va-a+1)
assert covered==set(range(a,z)) and len(fixes)==1;code=im.b[im.offset(a):im.offset(a)+z-a];at=fixes[0]
Path('work/material_queue_drain_native_init.h').write_text('#pragma once\ninline constexpr unsigned char materialQueueDrainNativeBytes[]={'+','.join(hex(x) for x in code)+'};\ninline void initializeMaterialQueueDrainNative(unsigned char* code,uint32_t reset){std::memcpy(code,materialQueueDrainNativeBytes,sizeof(materialQueueDrainNativeBytes));uint32_t displacement=reset-reinterpret_cast<uint32_t>(code+'+str(at+4)+');std::memcpy(code+'+str(at)+',&displacement,4);}\n');print('prepared61 drain bytes')
