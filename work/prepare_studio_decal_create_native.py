from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll');sha=hashlib.sha256(im.b).hexdigest();source=json.loads(Path('outputs/sfm64-studio-model-release-dispatch.json').read_text());assert sha==source['sha256']
a,z=0x10008a20,0x10008b59;rows=[r for r in source['instructions'] if a<=int(r['address'],16)<z];covered=set();fixes=[]
services={0x10006e80:'allocateNode',0x100074a0:'insertNode',0x10001860:'allocateBytes'}
imports={0x10070038:'threadIdentity',0x10070104:'contendedLock'}
for r in rows:
 va=int(r['address'],16);b=bytes.fromhex(r['bytes']);assert im.b[im.offset(va):im.offset(va)+len(b)]==b;covered.update(range(va,va+len(b)))
 if b[0]==0xe8:
  target=va+5+int.from_bytes(b[1:],'little',signed=True);assert target in services;fixes.append({'offset':va-a+1,'kind':'relative','service':services[target],'original':hex(target)})
 elif 'ds:[' in r['instruction']:
  assert b[:2]==b'\xff\x15' and len(b)==6;target=int.from_bytes(b[2:],'little');assert target in imports;fixes.append({'offset':va-a+2,'kind':'absolute','service':imports[target],'original':hex(target)})
assert covered==set(range(a,z));code=im.b[im.offset(a):im.offset(a)+z-a];cells={'threadIdentity':316,'contendedLock':320}
header=['#pragma once','inline constexpr unsigned char studioDecalCreateNativeBytes[]={'+','.join(hex(x) for x in code)+'};','inline void initializeStudioDecalCreateNative(unsigned char* code,uint32_t allocateNode,uint32_t insertNode,uint32_t allocateBytes,uint32_t threadIdentity,uint32_t contendedLock){','std::memcpy(code,studioDecalCreateNativeBytes,sizeof(studioDecalCreateNativeBytes));']
for name,at in cells.items():header.append(f'std::memcpy(code+{at},&{name},4);')
for fix in fixes:
 at=fix['offset'];name=fix['service']
 if fix['kind']=='relative':header.append(f'{{uint32_t displacement={name}-reinterpret_cast<uint32_t>(code+{at+4});std::memcpy(code+{at},&displacement,4);}}')
 else:header.append(f'{{uint32_t address=reinterpret_cast<uint32_t>(code+{cells[name]});std::memcpy(code+{at},&address,4);}}')
header+=['}','inline constexpr unsigned studioDecalCreateNativeAllocationSize=324;'];Path('work/studio_decal_create_native_init.h').write_text('\n'.join(header)+'\n')
report={'studio_sha256':sha,'method':hex(a),'bytes':len(code),'checked_instructions':len(rows),'relocations':fixes,'services':'Owned thiscall-compatible node allocator/initflag0, insert(pool,beforeNull,node); cdecl byte allocator(size); cdecl threadIdentity(); thiscall-compatible contendedLock(lock,threadId,0). Original atomic acquisition/depth/release retained.','status':'Complete caller byte coverage/relocations verified; execution pending.','limitations':'Original allocator/insert/heap services replaced by observers; cannot establish realheap ownership/concurrency or full renderer from caller parity.'};Path('outputs/sfm64-studio-decal-create-native-preparation.json').write_text(json.dumps(report,indent=2));print('prepared',len(code),'bytes',len(fixes),'service relocations')
