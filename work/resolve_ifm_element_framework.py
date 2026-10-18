from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');words={}
for rva,size,raw in image.sections:
 for offset in range(raw,raw+size-3,4):words.setdefault(image.u32(offset),[]).append(image.base+rva+offset-raw)
tables=[]
for entry in words.get(0x1038F090,[]):
 for slot in range(15):
  table=entry-slot*4
  try:
   locator=image.word(table-4)
   if image.word(locator)!=0:continue
   descriptor=image.word(locator+12);name=image.string(descriptor+8)
   if not name.startswith('.?AV'):continue
   tables.append(dict(name=name,table=hex(table),query_slot=slot,object_offset=image.word(locator+4),operate_slot11=hex(image.word(table+44)),table_pointers=[hex(v) for v in words.get(table,[])]))
  except (ValueError,UnicodeDecodeError):pass
global_refs=[]
for address in words.get(0x10E9CCF8,[]):
 global_refs.append(dict(address=hex(address),neighbor_words=[hex(image.word(address+offset)) for offset in range(-8,13,4)]))
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),query_interface='0x1038f090',interface_name=image.string(0x10B39338),candidate_tables=tables,aligned_global_address_references=global_refs,limitations='Static RTTI candidate resolution and aligned references. Operate name, global assignment, object lifecycle and runtime dispatch require independent verification.')
Path('outputs/sfm64-ifm-element-framework-candidates.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
