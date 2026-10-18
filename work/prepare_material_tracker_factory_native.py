from pathlib import Path
import json,struct
exec(Path('work/recover_tracker_material_system_factory.py').read_text().split('connect=region')[0])
r=json.loads(Path('outputs/sfm64-tracker-material-system-factory.json').read_text());assert r['sha256']==sha
bodies=[r['wrapper'],region(0x1006e300,0x1006e37e),r['local_factory_getter'],region(0x1006fd90,0x1006fdfe)]
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
   target=a+5+struct.unpack_from('<i',b,1)[0];assert target in (0x1006e300,0x10070260);calls.append((a-begin,target))
  elif b[0]==0xeb or 0x70<=b[0]<=0x7f:assert a+2+struct.unpack_from('<b',b,1)[0] in boundaries
 fixups=[(a-begin,im.word(a)) for a in relocations if begin<=a<limit]
 assert all(v in {0x10096888,0x10096870,0x10096860,0x101255c4,0x101255fc,0x100fc7b4,0x10108960,0x1006fd90,0x10135c00} for o,v in fixups)
 header+='static const unsigned char materialTrackerBytes'+str(i)+'[]={'+','.join(map(str,raw))+'};\n'
 for name,entries in [('Fixups',fixups),('Calls',calls)]:header+='static const unsigned materialTracker'+name+str(i)+'[][2]={'+','.join('{'+str(o)+','+str(v)+'}' for o,v in (entries or [(0,0)]))+'};\n'
 metadata.append(dict(file_offset=im.offset(begin),size=len(raw),fixups=fixups,calls=calls))
header+='struct MaterialTrackerBody{const unsigned char* bytes;unsigned size,fileOffset;const unsigned (*fixups)[2];unsigned fixupCount;const unsigned (*calls)[2];unsigned callCount;};\n'
header+='static const MaterialTrackerBody materialTrackerBodies[]={'+','.join('{materialTrackerBytes'+str(i)+','+str(m['size'])+','+str(m['file_offset'])+',materialTrackerFixups'+str(i)+','+str(len(m['fixups']))+',materialTrackerCalls'+str(i)+','+str(len(m['calls']))+'}' for i,m in enumerate(metadata))+'};\n'
Path('work/material_tracker_factory_native_init.h').write_text(header)
Path('outputs/sfm64-material-tracker-factory-native-preparation.json').write_text(json.dumps(dict(sha256=sha,bodies=metadata,name_compare='ASCIIcaseinsensitive fordirectnamedinterfaces; registryexactcase-sensitive',native_executed=False),indent=2)+'\n')
print('verified_material_tracker_factory_graph_bytes='+str(sum(m['size'] for m in metadata)))
