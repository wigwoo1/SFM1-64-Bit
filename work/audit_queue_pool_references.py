"""Inventory literal address references; does not substitute for alias analysis."""
import hashlib,json,struct
from pathlib import Path
from pe_image import Image
image=Image(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll'))
def references(address):
 result=[];pattern=struct.pack('<I',address);at=0
 while True:
  at=image.b.find(pattern,at)
  if at<0:break
  for rva,size,raw in image.sections:
   if raw<=at<raw+size:result.append(hex(image.base+rva+at-raw))
  at+=1
 return result
table=references(0x1017e040)
assert table==['0x10002cf5','0x10002d17','0x10002d24']
report={
 'binary_sha256':hashlib.sha256(image.b).hexdigest(),
 'literal_pool_table_references':table,
 'literal_ring_object_references':references(0x1017e8b8),
 'table_file_backed':False,
 'interpretation':'The three pool-table literal references belong to acquisition loads. No literal table-address initialization reference exists in this image. This is not proof of no initialization: computed addresses, runtime aliases, external writes and dormant branches remain possible.',
 'next_recovery':'Trace queue consumer and runtime enable/setup path instead of assigning unrelated nearby constructors to pool allocation.',
 'limits':'Literal byte-pattern inventory only, not exhaustive x86 dataflow or runtime execution. Does not prove actual allocation size or native queue feature availability.'}
Path('outputs/sfm64-queue-pool-reference-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print('literal_pool_table_references=3 initialization_write_reference=not_found')
