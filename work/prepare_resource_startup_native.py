from pathlib import Path
import json,struct
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
r=json.loads(Path('outputs/sfm64-resource-startup-registration.json').read_text());assert r['sha256']==sha
bodies=r["regions"]
allowed_calls={int(body['start'],16) for body in bodies}
opt=im.u32(60)+24;relocations=[];rva,size=struct.unpack_from('<II',im.b,opt+136);cursor=im.offset(im.base+rva);end=cursor+size
while cursor<end:
 page,length=struct.unpack_from('<II',im.b,cursor);assert length>=8 and cursor+length<=end
 for o in range(cursor+8,cursor+length,2):
  entry=im.u16(o);assert entry>>12 in (0,3)
  if entry>>12==3:relocations.append(im.base+page+(entry&4095))
 cursor+=length
assert cursor==end
header='#pragma once\n';metadata=[]
for i,body in enumerate(bodies):
 begin=int(body['start'],16);limit=int(body['end'],16);raw=im.b[im.offset(begin):im.offset(begin)+limit-begin];assert raw==b''.join(bytes.fromhex(x['bytes']) for x in body['instructions'])
 boundaries={int(x['address'],16) for x in body['instructions']};calls=[]
 for x in body['instructions']:
  a=int(x['address'],16);b=bytes.fromhex(x['bytes'])
  if b[0] in (0xe8,0xe9):
   target=a+5+struct.unpack_from('<i',b,1)[0];assert target in allowed_calls;calls.append((a-begin,target))
  elif b[0]==0xeb or 0x70<=b[0]<=0x7f:assert a+2+struct.unpack_from('<b',b,1)[0] in boundaries
 fixups=[(a-begin,im.word(a)) for a in relocations if begin<=a<limit]
 assert all(v in {0x10190904,0x1019094c,0x101b11fd,0x101b11fc,0x101b1200,0x101b122c,0x101b124c} for o,v in fixups)
 header+='static const unsigned char resourceStartupBytes'+str(i)+'[]={'+','.join(map(str,raw))+'};\n'
 for name,entries in [('Fixups',fixups),('Calls',calls)]:header+='static const unsigned resourceStartup'+name+str(i)+'[][2]={'+','.join('{'+str(o)+','+str(v)+'}' for o,v in (entries or [(0,0)]))+'};\n'
 metadata.append(dict(original_start=hex(begin),private_offset=512*i,file_offset=im.offset(begin),size=len(raw),fixups=fixups,calls=calls))
header+='struct ResourceStartupBody{const unsigned char* bytes;unsigned size,fileOffset;const unsigned (*fixups)[2];unsigned fixupCount;const unsigned (*calls)[2];unsigned callCount;};\n'
header+='static const ResourceStartupBody resourceStartupBodies[]={'+','.join('{resourceStartupBytes'+str(i)+','+str(m['size'])+','+str(m['file_offset'])+',resourceStartupFixups'+str(i)+','+str(len(m['fixups']))+',resourceStartupCalls'+str(i)+','+str(len(m['calls']))+'}' for i,m in enumerate(metadata))+'};\n'
assert len(metadata)==3 and sum(m['size'] for m in metadata)==159
assert sum(len(m['fixups']) for m in metadata)==11
assert sum(len(m['calls']) for m in metadata)==2
assert max(m['private_offset']+m['size'] for m in metadata)<=8192
header+='static const unsigned resourceStartupAddressMap[][2]={'+','.join('{'+m['original_start']+','+str(m['private_offset'])+'}' for m in metadata)+'};\n'
Path('work/resource_startup_native_init.h').write_text(header)
Path('outputs/sfm64-resource-startup-native-preparation.json').write_text(json.dumps(dict(sha256=sha,bodies=metadata,ownership='Three verified resource startup bodies; internal call/tail targets restricted. External virtual providers controlled. Shared precache/resource registration flags, five group tokens and node+0c list links remapped.',native_executed=False),indent=2)+'\n')
print('verified_resource_startup_graph_bytes='+str(sum(m['size'] for m in metadata)))





