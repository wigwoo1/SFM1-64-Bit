from pathlib import Path
import hashlib,json,struct
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest()
assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
nodes={};table_source=None
for path,key in [('outputs/sfm64-transition-handler-table-recovery.json','handlers'),('outputs/sfm64-transition-wrapper-helper-recovery.json','handlers'),('outputs/sfm64-restore-commit-setters-native-preparation.json','setters'),('outputs/sfm64-transition-byte-dispatch-native-preparation.json',None)]:
 source=json.loads(Path(path).read_text());assert source['sha256']==sha
 if 'tables' in source and table_source is None:table_source=source['tables']
 for h in source[key] if key else [source]:
  begin=int(h['begin'],16);assert begin not in nodes;nodes[begin]=h
assert len(nodes)==22
opt=im.u32(60)+24;rv,size=im.u32(opt+136),im.u32(opt+140)
pos=im.offset(im.base+rv);limit=pos+size;relocations=[]
while pos<limit:
 page,length=im.u32(pos),im.u32(pos+4);assert length>=8 and length%2==0 and pos+length<=limit
 for at in range(pos+8,pos+length,2):
  entry=im.u16(at)
  if entry>>12==3:relocations.append(im.base+page+(entry&4095))
 pos+=length
assert pos==limit
allowed_globals={0x10183fc4:'cache_receiver_cell',0x1019092c:'capability_receiver_cell',0x1016dd08:'backend_receiver_cell',0x1016e13c:'ordinary_handler_table',0x1016e170:'packed_handler_table'}
blob=bytearray();reports=[];placements={}
for begin,h in sorted(nodes.items()):
 while len(blob)%16:blob.append(0xcc)
 placements[begin]=len(blob);end=int(h['end'],16);covered={};calls=[];starts={int(r['address'],16) for r in h['instructions']}
 for row in h['instructions']:
  a=int(row['address'],16);b=bytes.fromhex(row['bytes']);assert im.b[im.offset(a):im.offset(a)+len(b)]==b
  for n,v in enumerate(b):assert a+n not in covered;covered[a+n]=v
  if b[0]==0xe8:
   target=a+5+struct.unpack('<i',b[1:])[0];assert target in nodes or target in (0x1000df50,0x10002da0);calls.append({'offset':a-begin,'target':hex(target),'binding':'internal' if target in nodes else 'getter' if target==0x1000df50 else 'refill'})
  if b[0]==0xeb or 0x70<=b[0]<=0x7f:assert a+2+struct.unpack('<b',b[1:2])[0] in starts
  elif b[0]==0x0f and len(b)>1 and 0x80<=b[1]<=0x8f:assert a+6+struct.unpack('<i',b[2:6])[0] in starts
 assert set(covered)==set(range(begin,end))
 raw=bytes(covered[a] for a in range(begin,end));blob.extend(raw);fixups=[]
 for a in relocations:
  if begin<=a<end:
   assert a+4<=end;target=im.word(a);assert target in allowed_globals;fixups.append({'offset':a-begin,'target':hex(target),'binding':allowed_globals[target]})
 reports.append({'begin':hex(begin),'end':hex(end),'placement':placements[begin],'bytes':len(raw),'calls':calls,'fixups':fixups})
tables={}
for name,start,count in [('ordinary',0x1016e13c,13),('packed',0x1016e170,2)]:
 values=[im.word(start+4*i) for i in range(count)];assert [hex(v) for v in values]==table_source[name];assert all(v in placements for v in values)
 tables[name]=[{'original':hex(v),'placement':placements[v]} for v in values]
assert tables['ordinary'][9]==tables['ordinary'][11]
for h in reports:
 for call in h['calls']:
  if call['binding']=='internal':call['target_placement']=placements[int(call['target'],16)]
Path('outputs/transition-composed-original.bin').write_bytes(blob)
out={'sha256':sha,'blob_sha256':hashlib.sha256(blob).hexdigest(),'entry_placement':placements[0x1002f2e0],'bodies':reports,'tables':tables,'bindings':allowed_globals,'limits':'Unpatched original bytes plus checked binding plan. Must verify runtime bytes, patch relative calls and global operands, supply controlled services, protect RX, then execute. No combined execution claimed.'}
Path('outputs/sfm64-transition-composed-native-preparation.json').write_text(json.dumps(out,indent=2)+'\n')
print(f'composed_transition_bodies={len(reports)} original_bytes={sum(h["bytes"] for h in reports)} blob_bytes={len(blob)} calls={sum(len(h["calls"]) for h in reports)} fixups={sum(len(h["fixups"]) for h in reports)}')
