from pathlib import Path
import hashlib,json,struct
from pe_image import Image
image=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
report=json.loads(Path('outputs/sfm64-shader-restore-unbind-recovery.json').read_text())
assert hashlib.sha256(image.b).hexdigest()==report['dll_sha256']
opt=image.u32(60)+24;rva,size=struct.unpack_from('<II',image.b,opt+136);cursor=image.offset(image.base+rva);limit=cursor+size;relocations=[]
while cursor<limit:
 page,length=struct.unpack_from('<II',image.b,cursor);assert length>=8 and cursor+length<=limit
 for pos in range(cursor+8,cursor+length,2):
  entry=image.u16(pos)
  if entry>>12:assert entry>>12==3;relocations.append(image.base+page+(entry&4095))
 cursor+=length
header='#pragma once\n';prepared=[]
for index,region in enumerate(report['regions'][2:]):
 start=int(region['start'],16);end=int(region['end_exclusive'],16);raw=image.b[image.offset(start):image.offset(start)+end-start];covered={};calls=[]
 for row in region['instructions']:
  address=int(row['address'],16);covered.update({address+i:v for i,v in enumerate(bytes.fromhex(row['bytes']))})
  if row['instruction'].startswith('call        100'):
   target=int(row['instruction'].split()[1],16);offset=address-start
   assert target in (0x100df012,0x1000df50,0x10002da0)
   assert raw[offset]==0xe8 and address+5+struct.unpack_from('<i',raw,offset+1)[0]==target
   calls.append((offset,target))
 assert raw==bytes(covered[a] for a in range(start,end))
 fixups=[(a-start,image.word(a)) for a in relocations if start<=a<end]
 allowed={0x100fa224,0x100fa258,0x10184218 if index==0 else 0x10184220,0x10184214 if index==0 else 0x1018421c,0x100ff400 if index==0 else 0x100ff418,0x100f9e00 if index==0 else 0x100f9e10}
 assert {t for _,t in fixups}==allowed
 header+=f'static const unsigned char shaderBindingBytes{index}[]={{'+','.join(map(str,raw))+'};\n'
 for name,entries in [('Calls',calls),('Fixups',fixups)]:header+=f'static const unsigned shaderBinding{name}{index}[][2]={{'+','.join('{'+str(a)+','+str(t)+'}' for a,t in entries)+'};\n'
 prepared.append(dict(bytes=len(raw),file_offset=image.offset(start),calls=calls,fixups=fixups))
header+='struct ShaderBindingNativeBody{const unsigned char* bytes;unsigned size,fileOffset;const unsigned (*calls)[2];unsigned callCount;const unsigned (*fixups)[2];unsigned fixupCount;};\n'
header+='static const ShaderBindingNativeBody shaderBindingBodies[]={'+','.join('{shaderBindingBytes'+str(i)+','+str(m['bytes'])+','+str(m['file_offset'])+',shaderBindingCalls'+str(i)+','+str(len(m['calls']))+',shaderBindingFixups'+str(i)+','+str(len(m['fixups']))+'}' for i,m in enumerate(prepared))+'};\n'
Path('work/shader_binding_native_init.h').write_text(header)
Path('outputs/sfm64-shader-binding-native-preparation.json').write_text(json.dumps(dict(dll_sha256=report['dll_sha256'],methods=prepared,native_executed=False,limits='Controlled initialized-counter path only; lazy imports/exit callbacks must be remapped and trapped.'),indent=2)+'\n')
print('shader_binding_prepared_bytes='+str(sum(m['bytes'] for m in prepared)))
