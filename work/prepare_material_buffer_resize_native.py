from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll');source=json.loads(Path('outputs/sfm64-material-call-queue-recovery.json').read_text());assert hashlib.sha256(im.b).hexdigest()==source['materialsystem_sha256']
a,z=0x10073b20,0x10073bef;cells={0x100950f0:4096,0x100950ec:4100,0x100951e4:4104,0x100951ec:4108};covered=set();fixes=[]
for r in source['instructions']:
 va=int(r['address'],16)
 if a<=va<z:
  b=bytes.fromhex(r['bytes']);assert im.b[im.offset(va):im.offset(va)+len(b)]==b;covered.update(range(va,va+len(b)))
  if 'ds:[' in r['instruction']:
   assert len(b)==6;target=int.from_bytes(b[2:],'little');assert target in cells;fixes.append((va-a+2,cells[target]))
assert covered==set(range(a,z));code=im.b[im.offset(a):im.offset(a)+z-a]
header=['#pragma once','inline constexpr unsigned char materialBufferResizeNativeBytes[]={'+','.join(hex(x) for x in code)+'};','inline void initializeMaterialBufferResizeNative(unsigned char* code,uint32_t commit,uint32_t decommit,uint32_t warn,uint32_t allocatorVariable){','std::memcpy(code,materialBufferResizeNativeBytes,sizeof(materialBufferResizeNativeBytes));','std::memcpy(code+4096,&commit,4);std::memcpy(code+4100,&decommit,4);std::memcpy(code+4104,&warn,4);std::memcpy(code+4108,&allocatorVariable,4);']
for at,cell in fixes:header.append(f'{{uint32_t address=reinterpret_cast<uint32_t>(code+{cell});std::memcpy(code+{at},&address,4);}}')
header.append('}');Path('work/material_buffer_resize_native_init.h').write_text('\n'.join(header)+'\n');print('prepared207 resize bytes')
