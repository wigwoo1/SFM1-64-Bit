from pathlib import Path
import hashlib,json,struct
from pe_image import Image

image=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
expected=json.loads(Path('outputs/sfm64-owner-reset-cleanup-recovery.json').read_text())['sha256']
assert hashlib.sha256(image.b).hexdigest()==expected
start,end=0x100df030,0x100df391
raw=image.b[image.offset(start):image.offset(start)+end-start]
anchors={
 0x100df035:'8b750c8b4d108b7d08',
 0x100df042:'03c63bfe76083bf80f82a0010000',
 0x100df050:'81f980000000',
 0x100df058:'833d0c611b1000',
 0x100df06f:'e99b810000',
 0x100df087:'f3a5',
 0x100df1f0:'8d7431fc8d7c39fc',
 0x100df20b:'fdf3a5fc',
 0x100df34c:'8b45085e5fc9c3',
}
for address,value in anchors.items():
 data=bytes.fromhex(value);assert raw[address-start:address-start+len(data)]==data,hex(address)
tables=[]
for address,count,role in [(0x100df0b4,4,'forward alignment'),(0x100df134,8,'forward short DWORD count'),(0x100df1a0,4,'forward byte tail'),(0x100df240,4,'backward alignment'),(0x100df2d0,8,'backward short DWORD count (indexed negatively from 100df2ec)'),(0x100df33c,4,'backward byte tail')]:
 first=1 if role.endswith('alignment') else 0
 targets=[image.word(address+4*i) for i in range(first,count)]
 assert all(start<=target<end for target in targets)
 tables.append(dict(address=hex(address),role=role,first_valid_index=first,targets=[hex(t) for t in targets]))
out=dict(dll_sha256=expected,start=hex(start),end_exclusive=hex(end),bytes=len(raw),body_sha256=hashlib.sha256(raw).hexdigest(),verified_anchors={hex(a):v for a,v in anchors.items()},jump_tables=tables,contract='cdecl(destination, source, uint32 byte count); returns original destination. Overlap destination>source and destination<source+count selects backward copy; otherwise forward. Therefore memcpy-only replacement would lose supported overlap behavior.',optimized_path=dict(minimum_bytes=128,feature_global='0x101b610c',requires_equal_alignment_mod16=True,target='0x100e720f',verified=False),limits='Installed hash and selected anchors/tables verified. Tables are data, not linear-disassembly instructions. Complete reachable decoding, optimized target audit and original/x64 execution comparison pending; no equivalence claim.')
Path('outputs/sfm64-constant-copy-helper-recovery.json').write_text(json.dumps(out,indent=2)+'\n')
optional=image.u32(60)+24
rva,size=struct.unpack_from('<II',image.b,optional+136)
cursor=image.offset(image.base+rva);limit=cursor+size;fixups=[]
while cursor<limit:
 page,length=struct.unpack_from('<II',image.b,cursor)
 assert length>=8 and cursor+length<=limit
 for position in range(cursor+8,cursor+length,2):
  entry=image.u16(position);address=image.base+page+(entry&4095)
  if start<=address<end and entry>>12:
   assert entry>>12==3 and address+4<=end
   target=image.word(address)
   assert start<=target<end or target==0x101b610c,hex(target)
   fixups.append((address-start,target))
 cursor+=length
assert fixups
header='#pragma once\nstatic const unsigned char copyHelperBytes[]={'+','.join(map(str,raw))+'};\n'
header+='static const unsigned copyHelperFileOffset='+str(image.offset(start))+';\n'
header+='static const unsigned copyHelperFixups[][2]={'+','.join('{'+str(a)+','+str(t)+'}' for a,t in fixups)+'};\n'
Path('work/constant_copy_helper_native_init.h').write_text(header)
print(f'copy_helper_span_bytes={len(raw)} anchors={len(anchors)} tables={len(tables)} verified')
