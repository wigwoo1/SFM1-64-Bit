from pathlib import Path
import json,hashlib,sys
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
sha=hashlib.sha256(im.b).hexdigest()
assert sha=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
a,z=0x1024a9c0,0x1024ad5f
rows=[r for r in json.loads(Path('outputs/sfm64-engine-shadow-manager-binding.json').read_text())['instructions'] if a<=int(r['address'],16)<z]
constants={0x1031cb50:0x3f000000,0x1031bdec:0x3f800000,0x1031dcf8:0x40000000}
for address,bits in constants.items():assert im.word(address)==bits
covered=set();fixes=[]
for r in rows:
 va=int(r['address'],16);b=bytes.fromhex(r['bytes']);op=r['instruction']
 assert im.b[im.offset(va):im.offset(va)+len(b)]==b
 covered.update(range(va,va+len(b)))
 assert not op.startswith('call'),op
 if 'ds:[' in op:
  target=int(op.split('ds:[')[1].split('h')[0],16)
  assert target in constants and int.from_bytes(b[-4:],'little')==target
  fixes.append({'offset':va-a+len(b)-4,'target':hex(target)})
assert covered==set(range(a,z))
code=im.b[im.offset(a):im.offset(a)+z-a]
assert len(fixes)==5 and code[-1]==0xc3
header=['#pragma once','inline constexpr unsigned char sphereNativeBytes[]={'+','.join(hex(v) for v in code)+'};','inline void initializeProjectionSphereNative(unsigned char* code){','std::memcpy(code,sphereNativeBytes,sizeof(sphereNativeBytes));']
locations={target:1024+i*16 for i,target in enumerate(constants)}
for target,bits in constants.items():header.append(f'{{uint32_t bits={bits}u;std::memcpy(code+{locations[target]},&bits,4);}}')
for fix in fixes:header.append(f'{{uint32_t address=reinterpret_cast<uint32_t>(code+{locations[int(fix["target"],16)]});std::memcpy(code+{fix["offset"]},&address,4);}}')
header+=['}','inline constexpr unsigned sphereNativeAllocationSize=1072;']
Path('work/engine_projection_sphere_native_bytes.h').write_text('\n'.join(header)+'\n')
report={'engine_sha256':sha,'original_start':hex(a),'size':len(code),'instruction_count':len(rows),'relocations':fixes,'calling_convention':'cdecl(matrix, center, radius); stack arguments at saved EBX+8,+c,+10; plain ret','status':'Complete byte coverage and constant relocation verified; execution pending','limitations':'No native parity claim; generated initializer is for an isolated x86 harness only.'}
Path('outputs/sfm64-engine-projection-sphere-native-preparation.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
