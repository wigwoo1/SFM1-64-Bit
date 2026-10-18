from pathlib import Path
import hashlib,json
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest()
assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
begin,end=0x10021087,0x100211d4
audit=json.loads(Path('outputs/sfm64-owner-reset-cleanup-recovery.json').read_text())
covered={};rows=[]
for row in audit['instructions']:
 a=int(row['address'],16);raw=bytes.fromhex(row['bytes'])
 if begin<=a<end:
  assert im.b[im.offset(a):im.offset(a)+len(raw)]==raw
  rows.append(row)
  for i,b in enumerate(raw):
   assert a+i not in covered
   covered[a+i]=b
assert set(covered)==set(range(begin,end))
raw=bytes(covered[a] for a in range(begin,end))
assert raw==im.b[im.offset(begin):im.offset(begin)+len(raw)]
starts={int(r['address'],16) for r in rows};branches=[]
for row in rows:
 a=int(row['address'],16);b=bytes.fromhex(row['bytes'])
 if b[0] in range(0x70,0x80) or b[0]==0xeb:
  assert len(b)==2
  target=a+2+int.from_bytes(b[1:],'little',signed=True)
  assert target in starts or target==end
  branches.append({'address':hex(a),'target':hex(target)})
 assert b[0]!=0xe8, 'Unexpected direct call requires binding audit'
p=im.u32(60);opt=p+24;relocRva=im.u32(opt+96+5*8);relocSize=im.u32(opt+96+5*8+4)
offset=im.offset(im.base+relocRva);limit=offset+relocSize;fixups=[];scanned=0
while offset<limit:
 page,size=im.u32(offset),im.u32(offset+4);assert size>=8 and offset+size<=limit
 for pos in range(offset+8,offset+size,2):
  entry=im.u16(pos);kind=entry>>12
  if kind==3:
   scanned+=1;address=im.base+page+(entry&0xfff)
   if begin<=address<end:fixups.append(address)
 offset+=size
assert not fixups
Path('outputs/restore-tail-verified-bytes.bin').write_bytes(raw)
Path('work/restore_tail_native_init.h').write_text('#pragma once\nstatic const unsigned restoreTailByteCount='+str(len(raw))+';\nstatic const unsigned restoreTailFileOffset='+str(im.offset(begin))+';\n')
report={'sha256':sha,'begin':hex(begin),'end':hex(end),'bytes':len(raw),'instructions':rows,'branches':branches,'highlow_relocations_scanned':scanned,'span_fixups':fixups,
 'contract':'One contiguous original span: two slot36 gates/live-list dispatch/clear, state copy preserving six staging slots, restore forcebyte reset and slot183 completion(true). EBX completeowner, clearDF, writableEBP locals and local-40 receiver. Controlled vtable36/cdecl4arg callbacks/vtable183 must be bound; exit return supplied after span before originalepilogue.',
 'limits':'Static preparation only; combined execution comparison pending. Original full5719byte restore entry/body, native allocation and GPU are outside this span.'}
Path('outputs/sfm64-restore-tail-native-preparation.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'prepared_restore_tail_bytes={len(raw)} verified_branches={len(branches)} scanned_highlow={scanned} span_fixups=0')
