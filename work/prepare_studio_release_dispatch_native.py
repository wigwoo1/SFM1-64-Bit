from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll');source=json.loads(Path('outputs/sfm64-studio-model-release-dispatch.json').read_text());assert hashlib.sha256(im.b).hexdigest()==source['sha256']
a,z=0x100580f0,0x1005816e;covered=set();fixes=[];cells={0x108734a8:4096,0x10872de4:4100,0x1008a41c:4104}
for r in source['instructions']:
 va=int(r['address'],16)
 if a<=va<z:
  b=bytes.fromhex(r['bytes']);assert im.b[im.offset(va):im.offset(va)+len(b)]==b;covered.update(range(va,va+len(b)))
  if b[0]==0xe8:
   target=va+5+int.from_bytes(b[1:],'little',signed=True);assert target in (0x100572e0,0x10008b60);fixes.append((va-a+1,'relative','enqueue' if target==0x100572e0 else 'cleanup'))
  elif 'ds:[' in r['instruction']:
   assert b[:2]==b'\x8b\x0d';target=int.from_bytes(b[2:],'little');assert target in cells;fixes.append((va-a+2,'cell',cells[target]))
  elif b[0]==0x68:
   assert int.from_bytes(b[1:],'little')==0x10008b60;fixes.append((va-a+1,'value','cleanup'))
assert covered==set(range(a,z));code=im.b[im.offset(a):im.offset(a)+z-a]
header=['#pragma once','inline constexpr unsigned char studioReleaseDispatchNativeBytes[]={'+','.join(hex(x) for x in code)+'};','inline void initializeStudioReleaseDispatchNative(unsigned char* code,uint32_t service,uint32_t config,uint32_t owner,uint32_t cleanup,uint32_t enqueue){','std::memcpy(code,studioReleaseDispatchNativeBytes,sizeof(studioReleaseDispatchNativeBytes));','std::memcpy(code+4096,&service,4);std::memcpy(code+4100,&config,4);std::memcpy(code+4104,&owner,4);']
for at,kind,value in fixes:
 if kind=='relative':expression=f'{value}-reinterpret_cast<uint32_t>(code+{at+4})'
 elif kind=='cell':expression=f'reinterpret_cast<uint32_t>(code+{value})'
 else:expression=value
 header.append(f'{{uint32_t value={expression};std::memcpy(code+{at},&value,4);}}')
header.append('}');Path('work/studio_release_dispatch_native_init.h').write_text('\n'.join(header)+'\n');print('prepared126 dispatch bytes')
