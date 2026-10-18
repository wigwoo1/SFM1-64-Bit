from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll');sha=hashlib.sha256(im.b).hexdigest();assert sha=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
rows=json.loads(Path('outputs/sfm64-engine-shadow-manager-binding.json').read_text())['instructions']
routines=[('gate',0x10109b80,0x10109c4b),('sphere',0x1014e1f0,0x1014e2ae),('planes',0x10246af0,0x10246bbe),('accessor',0x10104a00,0x10104a28)]
starts={a for _,a,_ in routines};chunks=[];offset=0;header=['#pragma once'];locations={}
for name,a,z in routines:
 offset=(offset+15)&~15;locations[a]=offset
 selected=[r for r in rows if a<=int(r['address'],16)<z];covered=set();fixes=[];indirect=[]
 for r in selected:
  va=int(r['address'],16);b=bytes.fromhex(r['bytes']);op=r['instruction'];assert im.b[im.offset(va):im.offset(va)+len(b)]==b;covered.update(range(va,va+len(b)))
  assert 'ds:[' not in op
  if op.startswith('call'):
   if b[0]==0xe8:
    target=va+5+int.from_bytes(b[1:],'little',signed=True);assert target in starts or target==0x10109a60
    fixes.append({'offset':va-a+1,'target':hex(target),'kind':'association_service' if target==0x10109a60 else 'relative'})
   else:
    assert b in (b'\xff\xd0',b'\xff\xd2');indirect.append(hex(va))
 assert covered==set(range(a,z)),name
 code=im.b[im.offset(a):im.offset(a)+z-a];header.append('inline constexpr unsigned char '+name+'GateBytes[]={'+','.join(hex(v) for v in code)+'};')
 chunks.append({'name':name,'start':hex(a),'size':len(code),'location':offset,'instructions':len(selected),'relocations':fixes,'indirect_calls':indirect});offset+=len(code)
header.append('inline void initializeProjectionReceiverGateNative(unsigned char* code,uint32_t associationAddress){')
for r in chunks:
 header.append(f'std::memcpy(code+{r["location"]},{r["name"]}GateBytes,{r["size"]});')
 for fix in r['relocations']:
  at=r['location']+fix['offset'];target=int(fix['target'],16)
  if fix['kind']=='relative':header.append(f'{{int32_t displacement={locations[target]-(at+4)};std::memcpy(code+{at},&displacement,4);}}')
  else:header.append(f'{{uint32_t displacement=associationAddress-reinterpret_cast<uint32_t>(code+{at+4});std::memcpy(code+{at},&displacement,4);}}')
header+=['}',f'inline constexpr unsigned projectionReceiverGateAllocationSize={offset};',f'inline constexpr unsigned projectionReceiverGateAccessorOffset={locations[0x10104a00]};']
Path('work/engine_projection_receiver_gate_native_init.h').write_text('\n'.join(header)+'\n')
report={'engine_sha256':sha,'routines':chunks,'bytes':sum(r['size'] for r in chunks),'allocation_size':offset,'external_services':'Association10109a60 must be replaced by an owned thiscall observation service; full allocator/resource body intentionally excluded. Object slots3(bounds)/5(record) require owned services; owner slot7 can use extracted original accessor.','status':'Full original gate and spatial/accessor helper byte coverage verified; execution pending.','limitations':'Preparation only, not full-gate native parity or resource association validation.'}
Path('outputs/sfm64-engine-projection-receiver-gate-native-preparation.json').write_text(json.dumps(report,indent=2));print(json.dumps({k:report[k] for k in ['bytes','allocation_size','status']},indent=2))
