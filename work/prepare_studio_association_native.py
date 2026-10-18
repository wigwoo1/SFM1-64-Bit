from pathlib import Path
import json,sys,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll');source=json.loads(Path('outputs/sfm64-studio-model-release-dispatch.json').read_text());assert hashlib.sha256(im.b).hexdigest()==source['sha256']
a,z=0x10007530,0x100075bf;rows=[r for r in source['instructions'] if a<=int(r['address'],16)<z];covered=set();fixes=[]
for r in rows:
 va=int(r['address'],16);b=bytes.fromhex(r['bytes']);assert im.b[im.offset(va):im.offset(va)+len(b)]==b;covered.update(range(va,va+len(b)));assert 'ds:[' not in r['instruction']
 if b[0]==0xe8:
  assert va+5+int.from_bytes(b[1:],'little',signed=True)==0x10003e50;fixes.append(va-a+1)
assert covered==set(range(a,z));assert len(fixes)==1
code=im.b[im.offset(a):im.offset(a)+z-a];at=fixes[0]
header=['#pragma once','inline constexpr unsigned char studioAssociationNativeBytes[]={'+','.join(hex(x) for x in code)+'};','inline void initializeStudioAssociationNative(unsigned char* code,uint32_t classify){','std::memcpy(code,studioAssociationNativeBytes,sizeof(studioAssociationNativeBytes));',f'uint32_t displacement=classify-reinterpret_cast<uint32_t>(code+{at+4});std::memcpy(code+{at},&displacement,4);','}']
frontierRows=[r for r in source['instructions'] if 0x10003e50<=int(r['address'],16)<0x10003ec7];covered=set()
for r in frontierRows:
 va=int(r['address'],16);b=bytes.fromhex(r['bytes']);assert im.b[im.offset(va):im.offset(va)+len(b)]==b;covered.update(range(va,va+len(b)));assert 'call' not in r['instruction'] and 'ds:[' not in r['instruction']
assert covered==set(range(0x10003e50,0x10003ec7))
frontier=im.b[im.offset(0x10003e50):im.offset(0x10003e50)+119]
header.append('inline constexpr unsigned char studioAssociationFrontierNativeBytes[]={'+','.join(hex(x) for x in frontier)+'};')
Path('work/studio_association_native_init.h').write_text('\n'.join(header)+'\n');Path('outputs/sfm64-studio-association-native-preparation.json').write_text(json.dumps({'studio_sha256':source['sha256'],'bytes':len(code),'checked_instructions':len(rows),'classifier_relocations':fixes,'frontier_bytes':119,'frontier_checked_instructions':len(frontierRows)},indent=2));print('prepared',len(code),'association bytes')
