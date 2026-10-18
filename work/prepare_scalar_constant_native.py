from pathlib import Path
import hashlib,json,struct
from pe_image import Image
image=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
report=json.loads(Path('outputs/sfm64-constant-subobject-methods.json').read_text())
assert hashlib.sha256(image.b).hexdigest()==report['dll_sha256']
opt=image.u32(60)+24;rva,size=struct.unpack_from('<II',image.b,opt+136)
cursor=image.offset(image.base+rva);limit=cursor+size;relocations=[]
while cursor<limit:
 page,length=struct.unpack_from('<II',image.b,cursor);assert length>=8 and cursor+length<=limit
 for pos in range(cursor+8,cursor+length,2):
  entry=image.u16(pos)
  if entry>>12:relocations.append(image.base+page+(entry&4095))
 cursor+=length
header='#pragma once\n';prepared=[]
for index,method in enumerate(report['methods']):
 start=int(method['target'],16);end=int(method['end_exclusive'],16)
 assert not any(start<=a<end for a in relocations)
 raw=image.b[image.offset(start):image.offset(start)+end-start];covered={}
 calls=[]
 for row in method['instructions']:
  address=int(row['address'],16)
  covered.update({address+i:v for i,v in enumerate(bytes.fromhex(row['bytes']))})
  if row['instruction'].startswith('call        100'):
   target=int(row['instruction'].split()[1],16);offset=address-start
   assert target in (0x10002da0,0x100df030)
   assert raw[offset]==0xe8 and address+5+struct.unpack_from('<i',raw,offset+1)[0]==target
   calls.append((offset,target))
 assert raw==bytes(covered[a] for a in range(start,end))
 header+=f'static const unsigned char scalarBytes{index}[]={{'+','.join(map(str,raw))+'};\n'
 header+=f'static const unsigned scalarCalls{index}[][2]={{'+','.join('{'+str(a)+','+str(t)+'}' for a,t in calls)+'};\n'
 prepared.append(dict(slot=method['slot'],bytes=len(raw),calls=calls,file_offset=image.offset(start)))
header+='struct ScalarNativeBody{const unsigned char* bytes;unsigned size,fileOffset;const unsigned (*calls)[2];unsigned callCount;};\n'
header+='static const ScalarNativeBody scalarBodies[]={'+','.join('{scalarBytes'+str(i)+','+str(m['bytes'])+','+str(m['file_offset'])+',scalarCalls'+str(i)+','+str(len(m['calls']))+'}' for i,m in enumerate(prepared))+'};\n'
Path('work/scalar_constant_native_init.h').write_text(header)
Path('outputs/sfm64-scalar-constant-native-preparation.json').write_text(json.dumps(dict(dll_sha256=report['dll_sha256'],methods=prepared,native_executed=False),indent=2)+'\n')
print('scalar_native_prepared_methods=4 bytes='+str(sum(m['bytes'] for m in prepared)))
