from pathlib import Path
import json,sys,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll');source=json.loads(Path('outputs/sfm64-studio-model-release-dispatch.json').read_text());assert hashlib.sha256(im.b).hexdigest()==source['sha256']
routines=[('Add',0x10055370,0x1005537e,0),('Release',0x10055380,0x100553ac,32),('Gate',0x10059e20,0x10059e23,96),('Thunk',0x10055750,0x10055758,112),('Delete',0x100559f0,0x10055a0c,128)];header=['#pragma once'];fixes=[];total=0
for name,a,z,location in routines:
 rows=[r for r in source['instructions'] if a<=int(r['address'],16)<z];covered=set()
 for r in rows:
  va=int(r['address'],16);b=bytes.fromhex(r['bytes']);assert im.b[im.offset(va):im.offset(va)+len(b)]==b;covered.update(range(va,va+len(b)));assert 'ds:[' not in r['instruction']
  if b[0] in (0xe8,0xe9):
   target=va+5+int.from_bytes(b[1:],'little',signed=True);assert target in (0x100559f0,0x10001940);fixes.append((location+va-a+1,target))
 assert covered==set(range(a,z));total+=z-a;code=im.b[im.offset(a):im.offset(a)+z-a];header.append('inline constexpr unsigned char studioQueued'+name+'NativeBytes[]={'+','.join(hex(x) for x in code)+'};')
header.append('inline void initializeStudioQueuedReferenceNative(unsigned char* code,uint32_t freeObject){')
for name,a,z,location in routines:header.append(f'std::memcpy(code+{location},studioQueued{name}NativeBytes,sizeof(studioQueued{name}NativeBytes));')
for at,target in fixes:
 value=f'128-{at+4}' if target==0x100559f0 else f'freeObject-reinterpret_cast<uint32_t>(code+{at+4})'
 header.append(f'{{uint32_t value={value};std::memcpy(code+{at},&value,4);}}')
header.append('}');Path('work/studio_queued_reference_native_init.h').write_text('\n'.join(header)+'\n');Path('outputs/sfm64-studio-queued-reference-native-preparation.json').write_text(json.dumps({'studio_sha256':source['sha256'],'bytes':total,'relocations':len(fixes),'layout':routines},indent=2));print('prepared',total,'reference bytes')
