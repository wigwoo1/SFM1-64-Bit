from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll');sha=hashlib.sha256(im.b).hexdigest();assert sha=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
rows=json.loads(Path('outputs/sfm64-engine-shadow-manager-binding.json').read_text())['instructions']
routines=[('release',0x100d8220,0x100d8295),('unlink',0x100d7440,0x100d74f3),('resource',0x100d48c0,0x100d4911)]
starts={a for _,a,_ in routines};chunks=[];offset=0;header=['#pragma once'];locations={}
for name,a,z in routines:
 offset=(offset+15)&~15;locations[a]=offset
 selected=[r for r in rows if a<=int(r['address'],16)<z];covered=set();fixes=[];indirect=[]
 for r in selected:
  va=int(r['address'],16);b=bytes.fromhex(r['bytes']);op=r['instruction'];assert im.b[im.offset(va):im.offset(va)+len(b)]==b;covered.update(range(va,va+len(b)))
  if 'ds:[' in op:
   assert b[:2]==b'\x8b\x0d' and len(b)==6
   target=int.from_bytes(b[2:],'little');assert target in (0x106941d4,0x10401cd0)
   fixes.append({'offset':va-a+2,'target':hex(target),'kind':'absolute'})
  if op.startswith('call'):
   if b[0]==0xe8:
    target=va+5+int.from_bytes(b[1:],'little',signed=True);assert target in starts
    fixes.append({'offset':va-a+1,'target':hex(target),'kind':'relative'})
   else:
    assert b in (b'\xff\xd0',b'\xff\xd1',b'\xff\xd2');indirect.append(hex(va))
 assert covered==set(range(a,z)),name
 code=im.b[im.offset(a):im.offset(a)+z-a];header.append('inline constexpr unsigned char '+name+'GateBytes[]={'+','.join(hex(v) for v in code)+'};')
 chunks.append({'name':name,'start':hex(a),'size':len(code),'location':offset,'instructions':len(selected),'relocations':fixes,'indirect_calls':indirect});offset+=len(code)
cells={0x106941d4:(offset+3)&~3,0x10401cd0:((offset+3)&~3)+4};offset=max(cells.values())+4
header.append('inline void initializeModelRenderReleaseNative(unsigned char* code,uint32_t studioObject,uint32_t shadowObject){')
header.append(f'std::memcpy(code+{cells[0x106941d4]},&studioObject,4);')
header.append(f'std::memcpy(code+{cells[0x10401cd0]},&shadowObject,4);')
for r in chunks:
 header.append(f'std::memcpy(code+{r["location"]},{r["name"]}GateBytes,{r["size"]});')
 for fix in r['relocations']:
  at=r['location']+fix['offset'];target=int(fix['target'],16)
  if fix['kind']=='relative':header.append(f'{{int32_t displacement={locations[target]-(at+4)};std::memcpy(code+{at},&displacement,4);}}')
  elif fix['kind']=='absolute':header.append(f'{{uint32_t address=reinterpret_cast<uint32_t>(code+{cells[target]});std::memcpy(code+{at},&address,4);}}')
  else:raise AssertionError('Unexpected external relocation')
header+=['}',f'inline constexpr unsigned modelRenderReleaseAllocationSize={offset};']
Path('work/engine_model_render_release_native_init.h').write_text('\n'.join(header)+'\n')
report={'engine_sha256':sha,'routines':chunks,'bytes':sum(r['size'] for r in chunks),'allocation_size':offset,'external_services':'Owned Studio object slot34(record+8), Shadow object slot12(fullargument), owner+8 resource object slot8(resource,0,0,1); directcalls all copied local helpers. External services remain observers.','status':'Full original release, model pool unlink and record resource cleanup coverage and relocations verified; execution pending.','limitations':'Preparation only. Real Studio/shadow/resource ownership, fullrelease native parity and live renderer integration remain unverified.'}
Path('outputs/sfm64-engine-model-render-release-native-preparation.json').write_text(json.dumps(report,indent=2));print(json.dumps({k:report[k] for k in ['bytes','allocation_size','status']},indent=2))
