from pathlib import Path
import json,sys,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll');source=json.loads(Path('outputs/sfm64-studio-model-release-dispatch.json').read_text());assert hashlib.sha256(im.b).hexdigest()==source['sha256']
header=['#pragma once'];fixes=[]
for name,a,z,location in [('Delete',0x100073d0,0x10007495,0),('Remove',0x100040c0,0x1000412d,256)]:
 rows=[r for r in source['instructions'] if a<=int(r['address'],16)<z];covered=set()
 for r in rows:
  va=int(r['address'],16);b=bytes.fromhex(r['bytes']);assert im.b[im.offset(va):im.offset(va)+len(b)]==b;covered.update(range(va,va+len(b)))
  if b[0]==0xe8:
   target=va+5+int.from_bytes(b[1:],'little',signed=True);assert target in (0x100040c0,0x10001940);fixes.append((location+va-a+1,target))
  elif 'ds:[' in r['instruction']:
   assert b[:2]==b'\x8b\x0d' and int.from_bytes(b[2:],'little')==0x10070118;fixes.append((location+va-a+2,None))
 assert covered==set(range(a,z));code=im.b[im.offset(a):im.offset(a)+z-a];header.append('inline constexpr unsigned char studioEntry'+name+'NativeBytes[]={'+','.join(hex(x) for x in code)+'};')
header+=['inline void initializeStudioEntryDeleteNative(unsigned char* code,uint32_t allocatorVariable,uint32_t freeAllocation){','std::memcpy(code,studioEntryDeleteNativeBytes,sizeof(studioEntryDeleteNativeBytes));','std::memcpy(code+256,studioEntryRemoveNativeBytes,sizeof(studioEntryRemoveNativeBytes));','std::memcpy(code+512,&allocatorVariable,4);']
for at,target in fixes:
 if target is None:header.append(f'{{uint32_t value=reinterpret_cast<uint32_t>(code+512);std::memcpy(code+{at},&value,4);}}')
 elif target==0x100040c0:header.append(f'{{int32_t value=256-{at+4};std::memcpy(code+{at},&value,4);}}')
 else:header.append(f'{{uint32_t value=freeAllocation-reinterpret_cast<uint32_t>(code+{at+4});std::memcpy(code+{at},&value,4);}}')
header+=['}'];Path('work/studio_entry_delete_native_init.h').write_text('\n'.join(header)+'\n');Path('outputs/sfm64-studio-entry-delete-native-preparation.json').write_text(json.dumps({'studio_sha256':source['sha256'],'bytes':306,'relocations':len(fixes),'services':'Owned allocator slot5/freeAllocation; original12byte list cleanup retained.'},indent=2));print('prepared306 bytes')
