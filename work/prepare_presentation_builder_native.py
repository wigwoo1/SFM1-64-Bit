from pathlib import Path
import json,hashlib,struct
from pe_image import Image
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
report=json.loads(Path('outputs/sfm64-device-reset-path-recovery.json').read_text());assert report['dll_sha256']==hashlib.sha256(image.b).hexdigest()
start,end=0x10029960,0x10029cf1;raw=image.b[image.offset(start):image.offset(start)+end-start]
covered={};calls=[];indirect=[]
for row in report['instructions']:
 address=int(row['address'],16)
 if start<=address<end:
  for i,value in enumerate(bytes.fromhex(row['bytes'])):covered[address+i]=value
  instruction=row['instruction']
  if instruction.startswith('call'):
   operand=instruction.split(None,1)[1]
   if len(operand)==8 and all(c in '0123456789ABCDEF' for c in operand):
    target=int(operand,16);offset=address-start;assert raw[offset]==0xe8 and address+5+struct.unpack_from('<i',raw,offset+1)[0]==target;calls.append({'offset':offset,'target':target})
   else:indirect.append({'address':hex(address),'instruction':instruction})
assert raw==bytes(covered[address] for address in range(start,end))
assert len(calls)==9
branches=[]
for row in report['instructions']:
 address=int(row['address'],16)
 if start<=address<end and row['instruction'].startswith('j'):
  target=int(row['instruction'].split()[-1],16)
  assert start<=target<end
  assert any(int(other['address'],16)==target for other in report['instructions'])
  branches.append({'address':hex(address),'target':hex(target)})
optional=image.u32(60)+24;rva,size=struct.unpack_from('<II',image.b,optional+136);cursor=image.offset(image.base+rva);limit=cursor+size;fixups=[]
while cursor<limit:
 page,length=struct.unpack_from('<II',image.b,cursor);assert length>=8 and length%2==0 and cursor+length<=limit
 for position in range(cursor+8,cursor+length,2):
  entry=image.u16(position);address=image.base+page+(entry&4095)
  if start<=address<end and entry>>12:
   assert entry>>12==3 and address+4<=end;fixups.append({'offset':address-start,'target':image.word(address)})
 cursor+=length
header='#pragma once\nstatic const unsigned char nativePresentationBuilderBytes[]={'+','.join(map(str,raw))+'};\nstatic const unsigned nativePresentationBuilderOffset='+str(image.offset(start))+';\n'
for name,entries in [('Fixups',fixups),('Calls',calls)]:header+='static const unsigned nativePresentationBuilder'+name+'[][2]={'+','.join('{'+str(entry['offset'])+','+str(entry['target'])+'}' for entry in entries)+'};\n'
Path('work/presentation_builder_native_init.h').write_text(header)
dependencies={hex(call['target']) for call in calls}
strings={hex(entry['target']):image.string(entry['target']) for entry in fixups if entry['target'] in {0x100fdd80,0x100fdd74,0x100fdd68,0x100fdd58,0x100fdd44}}
Path('outputs/sfm64-presentation-builder-native-preparation.json').write_text(json.dumps({'dll_sha256':report['dll_sha256'],'bytes':len(raw),'fixups':fixups,'calls':calls,'indirect_calls':indirect,'internal_branches':branches,'direct_dependencies':sorted(dependencies),'referenced_strings':strings,'native_executed':False,'limits':'All original external services must be privately rebound before execution; display/material/convar callbacks and GPU multisample calls require controlled replacements. No original routine executed.'},indent=2)+'\n')
print('presentation_builder_prepared_bytes='+str(len(raw))+' fixups='+str(len(fixups))+' direct_calls='+str(len(calls))+' indirect_calls='+str(len(indirect)))

