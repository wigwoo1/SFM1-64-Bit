from pathlib import Path
import json,hashlib,struct
from pe_image import Image
image=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
report=json.loads(Path('outputs/sfm64-owner-reset-cleanup-recovery.json').read_text());assert report['sha256']==hashlib.sha256(image.b).hexdigest()
start,end=0x1002b300,0x1002b3c0;raw=image.b[image.offset(start):image.offset(start)+end-start]
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
assert not calls
optional=image.u32(60)+24;rva,size=struct.unpack_from('<II',image.b,optional+136);cursor=image.offset(image.base+rva);limit=cursor+size;fixups=[]
while cursor<limit:
 page,length=struct.unpack_from('<II',image.b,cursor);assert length>=8 and length%2==0 and cursor+length<=limit
 for position in range(cursor+8,cursor+length,2):
  entry=image.u16(position);address=image.base+page+(entry&4095)
  if start<=address<end and entry>>12:
   assert entry>>12==3 and address+4<=end;fixups.append({'offset':address-start,'target':image.word(address)})
 cursor+=length
header='#pragma once\nstatic const unsigned char nativeQueryCleanupBytes[]={'+','.join(map(str,raw))+'};\nstatic const unsigned nativeQueryCleanupOffset='+str(image.offset(start))+';\n'
for name,entries in [('Fixups',fixups)]:header+='static const unsigned nativeQueryCleanup'+name+'[][2]={'+','.join('{'+str(entry['offset'])+','+str(entry['target'])+'}' for entry in entries)+'};\n'
Path('work/query_cleanup_native_init.h').write_text(header)
Path('outputs/sfm64-query-cleanup-native-preparation.json').write_text(json.dumps({'dll_sha256':report['sha256'],'bytes':len(raw),'fixups':fixups,'calls':calls,'indirect_calls':indirect,'native_executed':False,'limits':'All original external services must be privately rebound before execution; no real GPU Reset/flush/logging allowed in controlled oracle.'},indent=2)+'\n')
print('query_cleanup_prepared_bytes='+str(len(raw))+' fixups='+str(len(fixups))+' direct_calls='+str(len(calls))+' indirect_calls='+str(len(indirect)))

