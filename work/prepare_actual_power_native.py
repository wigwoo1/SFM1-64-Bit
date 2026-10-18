from pathlib import Path
import json,struct
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
r=json.loads(Path('outputs/sfm64-optimized-power-reachable.json').read_text());assert r['sha256']==sha
opt=im.u32(60)+24;rv,size=struct.unpack_from('<II',im.b,opt+136);cursor=im.offset(im.base+rv);limit=cursor+size;rel=[]
while cursor<limit:
 page,length=struct.unpack_from('<II',im.b,cursor);assert length>=8 and cursor+length<=limit
 for o in range(cursor+8,cursor+length,2):
  e=im.u16(o);assert e>>12 in (0,3)
  if e>>12==3:rel.append(im.base+page+(e&4095))
 cursor+=length
assert cursor==limit
h='#pragma once\n';metadata=[]
for i,body in enumerate(r['routines']):
 begin=int(body['start'],16);end=int(body['end'],16)
 raw=im.b[im.offset(begin):im.offset(begin)+end-begin]
 fix=[(a-begin,im.word(a)) for a in rel if begin<=a<end]
 assert all(0x10153a00<=v<0x10159100 for _,v in fix)
 h+='static const unsigned char powerBytes'+str(i)+'[]={'+','.join(map(str,raw))+'};\n'
 h+='static const unsigned powerFixups'+str(i)+'[][2]={'+','.join('{'+str(o)+','+str(v)+'}' for o,v in fix)+'};\n'
 h+='static const unsigned powerFileOffset'+str(i)+'='+str(im.offset(begin))+';\n'
 metadata.append(dict(start=hex(begin),bytes=len(raw),fixups=fix))
data=im.b[im.offset(0x10153a00):im.offset(0x10153a00)+0x5700];assert len(data)==0x5700
h+='static const unsigned char powerData[]={'+','.join(map(str,data))+'};\nstatic const unsigned powerDataFileOffset='+str(im.offset(0x10153a00))+';\n'
Path('work/actual_power_native_init.h').write_text(h)
Path('outputs/sfm64-actual-power-native-preparation.json').write_text(json.dumps(dict(sha256=sha,bodies=metadata,data_start='0x10153a00',data_bytes=len(data),limits='Reachable instruction closure has no calls; internal code offsets retained, PE data fixups constrained to copied aligned lookup region. Native execution not yet proved.'),indent=2)+'\n')
print('prepared_actual_power_code_bytes='+str(sum(x['bytes'] for x in metadata))+' data_bytes='+str(len(data)))
