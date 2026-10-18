from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll');sha=hashlib.sha256(im.b).hexdigest();assert sha=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
a,z=0x100d32e0,0x100d3385;coverage=set();count=0;fixes=[];calls=[]
for r in json.loads(Path('outputs/sfm64-engine-shadow-manager-binding.json').read_text())['instructions']:
 va=int(r['address'],16)
 if not a<=va<z:continue
 b=bytes.fromhex(r['bytes']);op=r['instruction'];assert im.b[im.offset(va):im.offset(va)+len(b)]==b
 if 'ds:[' in op:
  assert va==0x100d3359 and b==bytes.fromhex('8B 0D E8 A4 31 10');fixes.append(va-a+2)
 if op.startswith('call'):
  assert b in (b'\xff\xd0',b'\xff\xd2');calls.append(hex(va))
 coverage.update(range(va,va+len(b)));count+=1
assert coverage==set(range(a,z)) and fixes==[123] and calls==['0x100d336c','0x100d337b']
code=im.b[im.offset(a):im.offset(a)+z-a]
header=['#pragma once','inline constexpr unsigned char modelRenderGrowNativeBytes[]={'+','.join(hex(v) for v in code)+'};',
 'inline void initializeModelRenderGrowNative(unsigned char* code,uint32_t allocatorVariableAddress){',
 'std::memcpy(code,modelRenderGrowNativeBytes,sizeof(modelRenderGrowNativeBytes));',
 'std::memcpy(code+192,&allocatorVariableAddress,4);',
 '{uint32_t address=reinterpret_cast<uint32_t>(code+192);std::memcpy(code+123,&address,4);}',
 '}','inline constexpr unsigned modelRenderGrowNativeAllocationSize=208;']
Path('work/engine_model_render_grow_native_init.h').write_text('\n'.join(header)+'\n')
report={'engine_sha256':sha,'bytes':len(code),'instructions':count,'relocations':[{'offset':123,'original_iat':'0x1031a4e8','replacement':'Owned IAT cell atcode+192 contains address of owned allocator-pointer variable.'}],
 'allocator_contract':'Original reads IATcell -> allocator-pointer variable -> allocatorobject -> vtable. Slot1 malloc(bytes) whenoldstorage null, slot3 realloc(oldstorage,bytes) otherwise. Observer must use x86 thiscall-compatible services; byte count capacity*264.',
 'status':'Complete byte coverage and one absolute relocation verified; execution pending.',
 'limitations':'Allocator will be an observation service; no original heap/runtime allocator parity claim.'}
Path('outputs/sfm64-engine-model-render-grow-native-preparation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
