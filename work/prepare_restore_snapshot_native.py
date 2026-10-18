from pathlib import Path
import hashlib,json
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest()
assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
begin,end=0x1002114b,0x100211d4
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
assert im.b[im.offset(begin):im.offset(begin)+len(raw)]==raw
assert bytes.fromhex('b99e030000') in raw
assert bytes.fromhex('f3a5') in raw
Path('outputs/restore-snapshot-verified-bytes.bin').write_bytes(raw)
Path('work/restore_snapshot_native_init.h').write_text('#pragma once\nstatic const unsigned restoreSnapshotByteCount='+str(len(raw))+';\nstatic const unsigned restoreSnapshotFileOffset='+str(im.offset(begin))+';\n')
report={'sha256':sha,'begin':hex(begin),'end':hex(end),'bytes':len(raw),'instructions':rows,
 'copy':{'source_owner_offset':'0x1198','destination_owner_offset':'0x2010','dwords':0x39e,'bytes':0x39e*4},
 'preserved_destination_offsets':['0x2858','0x285c','0x2860','0x2864','0x2868','0x286c'],
 'entry_contract':'EBX points to complete owner; EBP points to writable fixture frame with locals -48h through -38h; local -40h holds readable subobject receiver/vtable; DF must be clear. Span includes controlled vtable slot183 call with argument1 after owner30d8 byte reset; ends before original epilogue. Fixture must preserve nonvolatile registers and supply exit return.',
 'limits':'Preparation and static byte verification only; native execution, x64 comparison and full owner integration pending. Raw 32-bit owner layout must not be reused for x64 pointer fields.'}
Path('outputs/sfm64-restore-snapshot-native-preparation.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'prepared_restore_snapshot_bytes={len(raw)} copy_bytes={0x39e*4} preserved_pointer_slots=6')
