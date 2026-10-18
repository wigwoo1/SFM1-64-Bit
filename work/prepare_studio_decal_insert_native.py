from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll');sha=hashlib.sha256(im.b).hexdigest();assert sha==json.loads(Path('outputs/sfm64-studio-model-release-dispatch.json').read_text())['sha256']
rows=json.loads(Path('outputs/sfm64-studio-model-release-dispatch.json').read_text())['instructions']
routines=[('insert',0x100074a0,0x1000752f),('frontier',0x10003ed0,0x10003f4b)]
starts={a for _,a,_ in routines};chunks=[];offset=0;header=['#pragma once'];locations={}
for name,a,z in routines:
 offset=(offset+15)&~15;locations[a]=offset
 selected=[r for r in rows if a<=int(r['address'],16)<z];covered=set();fixes=[];indirect=[]
 for r in selected:
  va=int(r['address'],16);b=bytes.fromhex(r['bytes']);op=r['instruction'];assert im.b[im.offset(va):im.offset(va)+len(b)]==b;covered.update(range(va,va+len(b)))
  assert 'ds:[' not in op
  if op.startswith('call'):
   if b[0]==0xe8:
    target=va+5+int.from_bytes(b[1:],'little',signed=True);assert target in starts
    fixes.append({'offset':va-a+1,'target':hex(target),'kind':'relative'})
   else:
    assert b in (b'\xff\xd0',b'\xff\xd1',b'\xff\xd2');indirect.append(hex(va))
 assert covered==set(range(a,z)),name
 code=im.b[im.offset(a):im.offset(a)+z-a];header.append('inline constexpr unsigned char '+name+'GateBytes[]={'+','.join(hex(v) for v in code)+'};')
 chunks.append({'name':name,'start':hex(a),'size':len(code),'location':offset,'instructions':len(selected),'relocations':fixes,'indirect_calls':indirect});offset+=len(code)
header.append('inline void initializeStudioDecalInsertNative(unsigned char* code){')
for r in chunks:
 header.append(f'std::memcpy(code+{r["location"]},{r["name"]}GateBytes,{r["size"]});')
 for fix in r['relocations']:
  at=r['location']+fix['offset'];target=int(fix['target'],16)
  if fix['kind']=='relative':header.append(f'{{int32_t displacement={locations[target]-(at+4)};std::memcpy(code+{at},&displacement,4);}}')
  else:raise AssertionError('Unexpected external relocation')
header+=['}',f'inline constexpr unsigned studioDecalInsertAllocationSize={offset};']
Path('work/studio_decal_insert_native_init.h').write_text('\n'.join(header)+'\n')
report={'studio_sha256':sha,'routines':chunks,'bytes':sum(r['size'] for r in chunks),'allocation_size':offset,'external_services':'No external services. Original insert invokes copied frontier classifier; x86 fixture must use raw20byte nodes and blockheader8, full pointer links, frontierblock/index.','status':'Full original decal pointer insert and frontier helper coverage verified; execution pending.','limitations':'Preparation only. Native insertion/frontier parity, signed32 address-boundary behavior, realheap ownership and renderer integration remain unverified.'}
Path('outputs/sfm64-studio-decal-insert-native-preparation.json').write_text(json.dumps(report,indent=2));print(json.dumps({k:report[k] for k in ['bytes','allocation_size','status']},indent=2))
