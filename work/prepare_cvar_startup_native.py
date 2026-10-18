from pathlib import Path
import json,struct
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
r=json.loads(Path('outputs/sfm64-cvar-startup-registration.json').read_text());assert r['sha256']==sha
bodies=[r['body']]
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
  if b[0]==0xe8:
   target=a+5+struct.unpack_from('<i',b,1)[0];assert target in ();calls.append((a-begin,target))
  elif b[0]==0x0f and 0x80<=b[1]<=0x8f:assert a+6+struct.unpack_from('<i',b,2)[0] in boundaries
  elif b[0]==0xeb or 0x70<=b[0]<=0x7f:assert a+2+struct.unpack_from('<b',b,1)[0] in boundaries
 fixups=[(a-begin,im.word(a)) for a in relocations if begin<=a<limit]
 assert all(v in {0x101908e4,0x1019fcf4,0x1019fcf0,0x1017b7f0,0x1019fcec,0x1017b868,0x1019fce8} for o,v in fixups)
 header+='static const unsigned char cvarStartupBytes'+str(i)+'[]={'+','.join(map(str,raw))+'};\n'
 for name,entries in [('Fixups',fixups),('Calls',calls)]:header+='static const unsigned cvarStartup'+name+str(i)+'[][2]={'+','.join('{'+str(o)+','+str(v)+'}' for o,v in (entries or [(0,0)]))+'};\n'
 metadata.append(dict(file_offset=im.offset(begin),size=len(raw),fixups=fixups,calls=calls))
header+='struct CvarStartupBody{const unsigned char* bytes;unsigned size,fileOffset;const unsigned (*fixups)[2];unsigned fixupCount;const unsigned (*calls)[2];unsigned callCount;};\n'
header+='static const CvarStartupBody cvarStartupBodies[]={'+','.join('{cvarStartupBytes'+str(i)+','+str(m['size'])+','+str(m['file_offset'])+',cvarStartupFixups'+str(i)+','+str(len(m['fixups']))+',cvarStartupCalls'+str(i)+','+str(len(m['calls']))+'}' for i,m in enumerate(metadata))+'};\n'
assert len(metadata)==1 and metadata[0]['size']==172
assert len(metadata[0]['calls'])==0
Path('work/cvar_startup_native_init.h').write_text(header)
Path('outputs/sfm64-cvar-startup-native-preparation.json').write_text(json.dumps(dict(sha256=sha,bodies=metadata,ownership='Original172byteCvar registration with no directcalls; restrictedprovider/registered/flags/handle/access/fallback/pendinghead relocations andinternalshort/nearbranches. NativeABI virtualcallbacks controlled.',native_executed=False),indent=2)+'\n')
print('verified_cvar_startup_graph_bytes='+str(sum(m['size'] for m in metadata)))





