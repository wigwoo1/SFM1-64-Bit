from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll');source=json.loads(Path('outputs/sfm64-studio-model-release-dispatch.json').read_text());sha=hashlib.sha256(im.b).hexdigest();assert sha==source['sha256']
a,z=0x10008b60,0x10008d99;rows=[r for r in source['instructions'] if a<=int(r['address'],16)<z];covered=set();fixes=[]
services={0x10007530:'associations',0x10006a30:'unlinkIndex',0x10004050:'remove24',0x10003fe0:'remove48',0x100073d0:'releaseArray',0x10003ed0:'classify'}
absolute={0x1008d5e8:('counter',4096),0x10070118:('allocatorVariable',4100),0x10070038:('threadIdentity',4104),0x10070104:('contendedLock',4108)}
for r in rows:
 va=int(r['address'],16);b=bytes.fromhex(r['bytes']);assert im.b[im.offset(va):im.offset(va)+len(b)]==b;covered.update(range(va,va+len(b)))
 if b[0]==0xe8:
  target=va+5+int.from_bytes(b[1:],'little',signed=True);assert target in services;fixes.append({'offset':va-a+1,'kind':'relative','service':services[target],'original':hex(target)})
 elif 'ds:[' in r['instruction']:
  assert len(b)==6 and b[:2] in [b'\x29\x05',b'\x8b\x0d',b'\xff\x15'];target=int.from_bytes(b[2:],'little');assert target in absolute;name,cell=absolute[target];fixes.append({'offset':va-a+2,'kind':'absolute','service':name,'cell':cell,'original':hex(target)})
assert covered==set(range(a,z));code=im.b[im.offset(a):im.offset(a)+z-a];arguments=list(services.values())+[name for name,_ in absolute.values()]
header=['#pragma once','inline constexpr unsigned char studioDecalReleaseNativeBytes[]={'+','.join(hex(x) for x in code)+'};','inline void initializeStudioDecalReleaseNative(unsigned char* code,'+','.join('uint32_t '+name for name in arguments)+'){','std::memcpy(code,studioDecalReleaseNativeBytes,sizeof(studioDecalReleaseNativeBytes));']
for name,cell in absolute.values():header.append(f'std::memcpy(code+{cell},&{name},4);')
for fix in fixes:
 at=fix['offset'];name=fix['service']
 if fix['kind']=='relative':header.append(f'{{uint32_t displacement={name}-reinterpret_cast<uint32_t>(code+{at+4});std::memcpy(code+{at},&displacement,4);}}')
 else:header.append(f'{{uint32_t address=reinterpret_cast<uint32_t>(code+{fix["cell"]});std::memcpy(code+{at},&address,4);}}')
header+=['}','inline constexpr unsigned studioDecalReleaseNativeAllocationSize=8192;','inline constexpr unsigned studioDecalReleaseNativeCounterOffset=4096;'];Path('work/studio_decal_release_native_init.h').write_text('\n'.join(header)+'\n')
report={'studio_sha256':sha,'bytes':len(code),'checked_instructions':len(rows),'relocations':fixes,'services':'Owned thiscall-compatible association/indexunlink/remove24/remove48/releaseArray/classifier; original caller list unlink/freepush and atomic lock retained. Allocator IAT replacementcell must contain address of owned allocator-pointer variable; allocator slot5 free. Thread/lock imports owned. Counter cell mutable and must stay writable at code+4096; first page executable/read-only, second page read/write.','status':'Complete caller coverage/relocations verified; execution pending.','limitations':'Original local helpers replaced by observer services; empty-chain execution cannot prove populated54byte pool lifecycle/heap/concurrency or renderer.'};Path('outputs/sfm64-studio-decal-release-native-preparation.json').write_text(json.dumps(report,indent=2));print('prepared',len(code),'bytes',len(fixes),'relocations')

# Preserve a reproducible byte-checked oracle for the nested index unlink.
unlinkStart,unlinkEnd=0x10006a30,0x10006ab9
unlinkRows=[r for r in source['instructions'] if unlinkStart<=int(r['address'],16)<unlinkEnd];covered=set()
for r in unlinkRows:
 va=int(r['address'],16);b=bytes.fromhex(r['bytes']);assert im.b[im.offset(va):im.offset(va)+len(b)]==b;covered.update(range(va,va+len(b)));assert 'call' not in r['instruction'] and 'ds:[' not in r['instruction']
assert covered==set(range(unlinkStart,unlinkEnd))
unlinkBytes=im.b[im.offset(unlinkStart):im.offset(unlinkStart)+unlinkEnd-unlinkStart]
Path('work/studio_decal_index_unlink_native_init.h').write_text('#pragma once\ninline constexpr unsigned char studioDecalIndexUnlinkNativeBytes[]={'+','.join(hex(x) for x in unlinkBytes)+'};\n')
