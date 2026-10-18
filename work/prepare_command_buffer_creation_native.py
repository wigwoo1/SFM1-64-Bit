from pathlib import Path
import struct,json,hashlib
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
report=json.loads(Path('outputs/sfm64-command-buffer-creation-recovery.json').read_text())
sha=hashlib.sha256(im.b).hexdigest();assert sha==report['sha256']
opt=im.u32(60)+24;rva,size=struct.unpack_from('<II',im.b,opt+136);cursor=im.offset(im.base+rva);end=cursor+size;relocations=[]
while cursor<end:
 page,length=struct.unpack_from('<II',im.b,cursor);assert length>=8 and length%2==0 and cursor+length<=end
 for position in range(cursor+8,cursor+length,2):
  entry=im.u16(position);kind=entry>>12
  assert kind in (0,3)
  if kind==3:relocations.append(im.base+page+(entry&4095))
 cursor+=length
assert cursor==end
allowed_calls=[{0x100d7470,0x100d7640,0x10002e50,0x100df5ad},{0x10002e50,0x100df012}]
allowed_fixups=[{0x1017c370,0x100fb030,0x100fb020,0x100fb000,0x100fa224,0x100fa258,0x10190934},{0x100fb554,0x100fa244,0x1017fe94,0x100fa224,0x100fb534,0x100fa258,0x100f9b10,0x1017fe90}]
header='#pragma once\n';methods=[]
for index,region in enumerate(report['regions']):
 begin=int(region['start'],16);limit=int(region['end'],16);raw=im.b[im.offset(begin):im.offset(begin)+limit-begin]
 covered={};calls=[];branches=[];indirect=[];boundaries={int(row['address'],16) for row in region['instructions']}
 for row in region['instructions']:
  a=int(row['address'],16);b=bytes.fromhex(row['bytes']);assert im.b[im.offset(a):im.offset(a)+len(b)]==b
  for n,value in enumerate(b):assert a+n not in covered;covered[a+n]=value
  if b[0]==0xe8:
   target=a+5+struct.unpack_from('<i',b,1)[0];assert target in allowed_calls[index];calls.append((a-begin,target))
  elif row['instruction'].startswith('call'):indirect.append(row)
  if b[0]==0xeb or 0x70<=b[0]<=0x7f:target=a+2+struct.unpack_from('<b',b,1)[0];assert target in boundaries;branches.append((a,target))
  elif b[0]==0x0f and 0x80<=b[1]<=0x8f:target=a+6+struct.unpack_from('<i',b,2)[0];assert target in boundaries;branches.append((a,target))
 assert raw==bytes(covered[a] for a in range(begin,limit))
 fixups=[(a-begin,im.word(a)) for a in relocations if begin<=a<limit]
 assert {value for offset,value in fixups}==allowed_fixups[index]
 for offset,value in fixups:assert offset+4<=len(raw)
 header+=f'static const unsigned char bufferCreationBytes{index}[]={{'+','.join(map(str,raw))+'};\n'
 for name,entries in [('Calls',calls),('Fixups',fixups)]:header+=f'static const unsigned bufferCreation{name}{index}[][2]={{'+','.join('{'+str(a)+','+str(t)+'}' for a,t in entries)+'};\n'
 methods.append(dict(start=region['start'],size=len(raw),file_offset=im.offset(begin),calls=calls,fixups=fixups,internal_branches=branches,indirect_calls=indirect))
header+='struct BufferCreationNativeBody{const unsigned char* bytes;unsigned size,fileOffset;const unsigned (*calls)[2];unsigned callCount;const unsigned (*fixups)[2];unsigned fixupCount;};\n'
header+='static const BufferCreationNativeBody bufferCreationBodies[]={'+','.join('{bufferCreationBytes'+str(i)+','+str(m['size'])+','+str(m['file_offset'])+',bufferCreationCalls'+str(i)+','+str(len(m['calls']))+',bufferCreationFixups'+str(i)+','+str(len(m['fixups']))+'}' for i,m in enumerate(methods))+'};\n'
Path('work/command_buffer_creation_native_init.h').write_text(header)
Path('outputs/sfm64-command-buffer-creation-native-preparation.json').write_text(json.dumps(dict(sha256=sha,methods=methods,native_executed=False,contract='Remap every direct call and absolute relocation. Vertex thiscall receiver size>=44, nine stack args, cookie checker/formatting/counter/service registration and COM slots26/5. Index thiscall receiver size>=28, four stack args, initialized lazy-counter path or explicit lazy-service remapping, logging/exit callback and COM slots27/5. Flush callback receives command argument; device callbacks may mutate output receiver and command device. Returned EAX is receiver, not HRESULT.',limits='Preparation only; imports/strings/globals must never retain installed addresses in executable fixture. Full resource ownership, original execution, GPU and async consumer remain unfinished.'),indent=2)+'\n')
print('buffer_creation_prepared_bytes='+str(sum(m['size'] for m in methods))+' direct_calls='+str(sum(len(m['calls']) for m in methods))+' absolute_fixups='+str(sum(len(m['fixups']) for m in methods)))
